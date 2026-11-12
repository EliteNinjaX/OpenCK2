#include "../open_launcher/asset_paths.h"
#include "map_view.h"
#include "assets.h"
#include "models.h"
#include "../open_launcher/install.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct layers {
    SDL_Surface *color, *water, *noise, *height, *normal, *terrain, *rivers, *trees, *atlas, *pine, *water_normal, *tree_tint, *underwater;
    SDL_Surface *reflection[6], *frame_diffuse;
    struct ck2_model frame_model;
    float *frame_depth;
};

struct rgb { int r, g, b; };

struct water_pixel {
    Uint32 screen, base;
    float x, y;
};

struct map_water {
    SDL_Surface *frame, *noise, *normal;
    SDL_Surface *reflection[6];
    float camera_x, camera_y, camera_height;
    struct water_pixel *pixels;
    size_t count;
    Uint64 last_update;
};

static Uint32 le32(const Uint8 *bytes)
{
    Uint32 value;
    memcpy(&value, bytes, sizeof value);
    return SDL_SwapLE32(value);
}

static struct rgb color565(unsigned value)
{
    return (struct rgb){(int)((value >> 11) * 255 / 31),
                        (int)(((value >> 5) & 63) * 255 / 63),
                        (int)((value & 31) * 255 / 31)};
}

static struct rgb mix(struct rgb a, struct rgb b, int weight, int total)
{
    const int rest = total - weight;
    return (struct rgb){(a.r * rest + b.r * weight) / total,
                        (a.g * rest + b.g * weight) / total,
                        (a.b * rest + b.b * weight) / total};
}

static Uint32 pack(struct rgb color)
{
    return 0xff000000u | ((Uint32)SDL_clamp(color.r, 0, 255) << 16) |
        ((Uint32)SDL_clamp(color.g, 0, 255) << 8) | (Uint32)SDL_clamp(color.b, 0, 255);
}

/* Terrain atlas alpha is unused by this pass. BC3 color blocks always use four colors. */
static SDL_Surface *load_atlas(const char *root)
{
    char *path = ck2_asset_path(root, "map/terrain/atlas0.dds");
    SDL_RWops *file = path ? SDL_RWFromFile(path, "rb") : NULL;
    free(path);
    if (!file)
        return NULL;
    SDL_Surface *atlas = NULL;
    Uint8 header[128];
    if (SDL_RWread(file, header, sizeof header, 1) != 1 ||
        memcmp(header, "DDS ", 4) || memcmp(header + 84, "DXT5", 4) ||
        le32(header + 12) != 2048 || le32(header + 16) != 2048) {
        SDL_SetError("Unsupported map terrain atlas");
        goto done;
    }

    atlas = SDL_CreateRGBSurfaceWithFormat(0, 2048, 2048, 32, SDL_PIXELFORMAT_BGRA32);
    if (!atlas)
        goto done;
    Uint8 blocks[512 * 16];
    for (int y = 0; y < atlas->h; y += 4) {
        if (SDL_RWread(file, blocks, sizeof blocks, 1) != 1) {
            SDL_FreeSurface(atlas);
            atlas = NULL;
            SDL_SetError("Truncated map terrain atlas");
            break;
        }
        for (int x = 0; x < 2048; x += 4) {
            const Uint8 *block = blocks + (x / 4) * 16;
            struct rgb palette[4] = {
                color565(block[8] | (unsigned)block[9] << 8),
                color565(block[10] | (unsigned)block[11] << 8),
                {0}, {0},
            };
            palette[2] = mix(palette[0], palette[1], 1, 3);
            palette[3] = mix(palette[0], palette[1], 2, 3);
            Uint32 indices = le32(block + 12);
            for (int row = 0; row < 4; ++row) {
                Uint8 *pixel = (Uint8 *)atlas->pixels + (y + row) * atlas->pitch + x * 4;
                for (int col = 0; col < 4; ++col, pixel += 4, indices >>= 2) {
                    const struct rgb color = palette[indices & 3];
                    pixel[0] = (Uint8)color.b;
                    pixel[1] = (Uint8)color.g;
                    pixel[2] = (Uint8)color.r;
                    pixel[3] = 255;
                }
            }
        }
    }

done:
    SDL_RWclose(file);
    return atlas;
}

static SDL_Surface *load_bitmap(const char *root, const char *name, bool indexed)
{
    char *path = ck2_asset_path(root, name);
    SDL_Surface *surface = path ? SDL_LoadBMP(path) : NULL;
    free(path);
    if (!surface)
        return NULL;
    if (indexed) {
        if (surface->format->format == SDL_PIXELFORMAT_INDEX8)
            return surface;
        SDL_FreeSurface(surface);
        SDL_SetError("Expected an indexed map bitmap: %s", name);
        return NULL;
    }
    SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_BGRA32, 0);
    SDL_FreeSurface(surface);
    return converted;
}

static void load_reflections(struct layers *layers, const char *root)
{
    char *path = ck2_asset_path(root, "map/terrain/reflection.dds");
    SDL_RWops *file = path ? SDL_RWFromFile(path, "rb") : NULL;
    free(path);
    if (!file)
        return;
    Uint8 header[128];
    if (SDL_RWread(file, header, sizeof header, 1) != 1 || memcmp(header, "DDS ", 4) ||
        memcmp(header + 84, "DXT1", 4) || (le32(header + 112) & 0xfe00u) != 0xfe00u)
        goto done;
    const unsigned size = le32(header + 16), mips = le32(header + 28);
    if (!size || size > 4096 || size != le32(header + 12) || !mips || mips > 13)
        goto done;
    const unsigned level = mips > 2 && size >= 128 ? 2 : 0;
    const unsigned side = size >> level;
    size_t chain = 0, offset = 0;
    for (unsigned mip = 0; mip < mips; ++mip) {
        const unsigned blocks = (SDL_max(1u, size >> mip) + 3) / 4;
        if (mip < level)
            offset += (size_t)blocks * blocks * 8;
        chain += (size_t)blocks * blocks * 8;
    }
    for (int face = 0; face < 6; ++face) {
        if (SDL_RWseek(file, 128 + (Sint64)(chain * face + offset), RW_SEEK_SET) < 0)
            break;
        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, (int)side, (int)side, 32,
                                                               SDL_PIXELFORMAT_BGRA32);
        if (!surface)
            break;
        bool valid = true;
        for (unsigned y = 0; y < side && valid; y += 4) {
            for (unsigned x = 0; x < side; x += 4) {
                Uint8 block[8];
                if (SDL_RWread(file, block, sizeof block, 1) != 1) {
                    valid = false;
                    break;
                }
                const unsigned a = block[0] | (unsigned)block[1] << 8;
                const unsigned b = block[2] | (unsigned)block[3] << 8;
                const struct rgb p = color565(a), q = color565(b);
                const struct rgb palette[] = {p, q, mix(p, q, 1, a > b ? 3 : 2),
                    a > b ? mix(p, q, 2, 3) : (struct rgb){0, 0, 0}};
                const Uint32 indices = le32(block + 4);
                for (unsigned dy = 0; dy < 4 && y + dy < side; ++dy) {
                    Uint32 *row = (Uint32 *)((Uint8 *)surface->pixels + (y + dy) * surface->pitch);
                    for (unsigned dx = 0; dx < 4 && x + dx < side; ++dx)
                        row[x + dx] = pack(palette[(indices >> (2 * (dy * 4 + dx))) & 3]);
                }
            }
        }
        if (valid)
            layers->reflection[face] = surface;
        else
            SDL_FreeSurface(surface);
    }
done:
    SDL_RWclose(file);
}

static int load_layers(struct layers *layers, const char *root)
{
    layers->color = ck2_load_dds_surface(root, "map/terrain/colormap.dds");
    layers->water = ck2_load_dds_surface(root, "map/terrain/colormap_water.dds");
    layers->noise = ck2_load_dds_surface(root, "map/terrain/noise-2d.dds");
    layers->height = load_bitmap(root, "map/topology.bmp", true);
    layers->normal = load_bitmap(root, "map/world_normal_height.bmp", false);
    layers->terrain = load_bitmap(root, "map/terrain.bmp", true);
    layers->rivers = load_bitmap(root, "map/rivers.bmp", true);
    layers->trees = load_bitmap(root, "map/trees.bmp", true);
    layers->atlas = load_atlas(root);
    layers->pine = ck2_load_dds_surface(root, "gfx/models/Mapitems/Pine_Diffuse.dds");
    layers->water_normal = load_bitmap(root, "map/terrain/waternormal.bmp", false);
    layers->tree_tint = load_bitmap(root, "map/terrain/Tree_tint.bmp", false);
    layers->underwater = ck2_load_dds_surface(root, "map/terrain/underwater_terrain.dds");
    load_reflections(layers, root);
    if (!layers->color || !layers->water || !layers->noise || !layers->height ||
        !layers->normal || !layers->terrain || !layers->rivers || !layers->trees || !layers->atlas || !layers->pine ||
        !layers->water_normal || !layers->tree_tint)
        return SDL_SetError("Could not load the local CK2 terrain layers");
    const SDL_Surface *aligned[] = {
        layers->water, layers->height, layers->normal, layers->terrain, layers->rivers
    };
    for (size_t i = 0; i < SDL_arraysize(aligned); ++i) {
        if (aligned[i]->w != layers->color->w || aligned[i]->h != layers->color->h)
            return SDL_SetError("CK2 terrain layers have different dimensions");
    }
    return 0;
}

static void free_layers(struct layers *layers)
{
    SDL_Surface *surfaces[] = {layers->color, layers->water, layers->noise, layers->height,
        layers->normal, layers->terrain, layers->rivers, layers->trees, layers->atlas, layers->pine, layers->water_normal, layers->tree_tint, layers->underwater, layers->frame_diffuse};
    for (size_t i = 0; i < SDL_arraysize(surfaces); ++i)
        SDL_FreeSurface(surfaces[i]);
    for (int i = 0; i < 6; ++i)
        SDL_FreeSurface(layers->reflection[i]);
    ck2_model_free(&layers->frame_model);
    free(layers->frame_depth);
}

static unsigned index_at(const SDL_Surface *surface, int x, int y)
{
    return ((const Uint8 *)surface->pixels)[(size_t)y * surface->pitch + x];
}

static struct rgb rgb_at(const SDL_Surface *surface, int x, int y)
{
    const Uint8 *pixel = (const Uint8 *)surface->pixels + (size_t)y * surface->pitch + x * 4;
    return (struct rgb){pixel[2], pixel[1], pixel[0]};
}

static struct rgb rgb_repeat(const SDL_Surface *surface, SDL_Rect region, float u, float v)
{
    u -= floorf(u / region.w) * region.w;
    v -= floorf(v / region.h) * region.h;
    const int x = (int)u, y = (int)v;
    const int next_x = x + 1 == region.w ? 0 : x + 1;
    const int next_y = y + 1 == region.h ? 0 : y + 1;
    const int fx = (int)((u - x) * 256), fy = (int)((v - y) * 256);
    const struct rgb top = mix(rgb_at(surface, region.x + x, region.y + y),
                               rgb_at(surface, region.x + next_x, region.y + y), fx, 256);
    const struct rgb bottom = mix(rgb_at(surface, region.x + x, region.y + next_y),
                                  rgb_at(surface, region.x + next_x, region.y + next_y), fx, 256);
    return mix(top, bottom, fy, 256);
}

static float elevation(const struct layers *layers, int x, int y)
{
    return SDL_max(0.0f, (index_at(layers->height, x, y) - 94.0f) * 0.40f);
}

static int overlay(int base, int tint)
{
    const int over = tint < 128 ? 2 * tint * base / 255
        : 255 - 2 * (255 - tint) * (255 - base) / 255;
    return (base + 3 * over) / 4;
}

static Uint32 hash_pixel(unsigned x, unsigned y)
{
    Uint32 value = x * 0x9e3779b1u + y * 0x85ebca77u;
    value ^= value >> 16;
    value *= 0x7feb352du;
    return value ^ (value >> 15);
}

static struct rgb shade_water(const struct layers *layers, int x, int y, float wx, float wy)
{
    const struct rgb tint = rgb_at(layers->water, x, y);
    const float height = (float)index_at(layers->height, x, y);
    const int shore = (int)(100 * SDL_clamp((height / 93.7f - .995f) * 50, 0.f, 1.f));
    const SDL_Surface *detail = layers->underwater;
    struct rgb ground = {66, 80, 99};
    if (detail)
        ground = mix(ground, rgb_repeat(detail, (SDL_Rect){0, 0, detail->w, detail->h}, wx * 2, wy * 2), 30, 100);
    struct rgb water = mix(ground, tint, 30, 100);
    return mix(water, rgb_at(layers->color, x, y), shore / 2, 100);
}

static struct rgb shade_land(const struct layers *layers, int x, int y, float wx, float wy)
{
    const struct rgb tint = rgb_at(layers->color, x, y);
    const unsigned tile = index_at(layers->terrain, x, y) & 15;
    const float repeat = 128.0f / (float)layers->color->h;
    const float u = wx * repeat, v = wy * repeat;
    const SDL_Rect cell = {(int)(tile % 4) * 512, (int)(tile / 4) * 512, 512, 512};
    const struct rgb detail = rgb_repeat(layers->atlas, cell, u * cell.w, v * cell.h);
    const struct rgb normal = rgb_at(layers->normal, x, y);
    const float lighting = SDL_clamp(0.96f + (128 - normal.r) * 0.0043f +
        (128 - normal.g) * 0.0035f + (normal.b - 255) * 0.0040f, 0.46f, 1.44f);
    struct rgb land = {(int)(overlay(detail.r, tint.r) * lighting),
                       (int)(overlay(detail.g, tint.g) * lighting),
                       (int)(overlay(detail.b, tint.b) * lighting)};

    const int luminance = (land.r * 3 + land.g * 6 + land.b) / 10;
    land = mix(land, (struct rgb){luminance + 10, luminance + 1, luminance - 3}, 2, 10);
    const unsigned river = index_at(layers->rivers, x, y);
    if (river < 254)
        land = mix(land, (struct rgb){72, 96, 108}, river < 7 ? 5 : 7, 10);
    return land;
}

struct camera {
    float x, y, height, forward_y, forward_height, focal;
};

/* World-space rectangle covered by the ground pixels that are on screen. */
struct map_bounds {
    int min_x, min_y, max_x, max_y;
};

static void bounds_add(struct map_bounds *bounds, int x, int y)
{
    bounds->min_x = SDL_min(bounds->min_x, x);
    bounds->min_y = SDL_min(bounds->min_y, y);
    bounds->max_x = SDL_max(bounds->max_x, x);
    bounds->max_y = SDL_max(bounds->max_y, y);
}

static struct camera camera_for(const struct map_view *view, bool campaign)
{
    /* CK2's final menu camera points from (790, 500, 600) to (790, 0, 1000).
       World Z increases north, while bitmap rows increase south. */
    const float altitude = campaign ? 790.0f / view->zoom : 500.0f;
    const float north = campaign ? 320.0f / view->zoom : 400.0f;
    const float scale = sqrtf(altitude * altitude + north * north);
    return (struct camera){campaign ? view->center_x : 790.0f,
                           campaign ? view->center_y + north : (float)view->world_height - 600,
                           altitude, -north / scale, -altitude / scale,
                           view->height * (campaign ? 1.11f : 1.24f)};
}

/* Flat fallback for installations without a supported frame actor. */
static Uint32 shade_map_frame(const struct map_view *view, const SDL_Surface *texture,
                              float x, float y)
{
    const float rail = 64.0f;
    const float horizontal = x < 0 ? -x : x - view->world_width;
    const float vertical = y < 0 ? -y : y - view->world_height;
    const float distance = SDL_max(horizontal, vertical);
    if (!texture || distance < 0 || distance >= rail)
        return 0xffa5a2ae;

    const float along = horizontal >= vertical ? y : x;
    const float repeat = along / (rail * texture->w / texture->h);
    const int u = (int)((repeat - floorf(repeat)) * texture->w);
    const int v = (int)((1.0f - distance / rail) * (texture->h - 1));
    return pack(rgb_at(texture, u, v));
}

static void render_row(const struct map_view *view, const struct layers *layers,
                        struct camera camera, int y, Uint32 *output, Uint32 *samples,
                        struct map_water *water, struct map_bounds *bounds, bool campaign)
{
    const float vertical = (view->height * 0.5f - (y + 0.5f)) / camera.focal;
    const float ray_y = camera.forward_y + vertical * camera.forward_height;
    const float ray_height = camera.forward_height - vertical * camera.forward_y;
    const float flat_distance = -camera.height / ray_height;

    for (int x = 0; x < view->width; ++x) {
        const float ray_x = (x + 0.5f - view->width * 0.5f) / camera.focal;
        float distance = flat_distance, wx = 0.0f, wy = 0.0f;
        int mx = 0, my = 0;
        bool on_map = false;
        /* Refine a ray-plane intersection with the local heightfield. */
        for (int sample = 0; sample < 4; ++sample) {
            wx = camera.x + ray_x * distance;
            wy = camera.y + ray_y * distance;
            on_map = wx >= 0.0f && wy >= 0.0f && wx < view->world_width && wy < view->world_height;
            if (!on_map)
                break;
            mx = (int)wx;
            my = (int)wy;
            distance = (elevation(layers, mx, my) - camera.height) / ray_height;
        }
        if (layers->frame_depth)
            layers->frame_depth[(size_t)y * view->width + x] =
                on_map ? 1.0f / distance : 0.0f;
        if (!on_map) {
            samples[x] = SDL_MAX_UINT32;
            output[x] = campaign
                ? shade_map_frame(view, layers->frame_depth ? NULL : layers->frame_diffuse,
                                  camera.x + ray_x * flat_distance, camera.y + ray_y * flat_distance)
                : 0xff3d4d60;
            continue;
        }

        samples[x] = (Uint32)((size_t)my * view->world_width + mx);
        bounds_add(bounds, mx, my);
        /* The water shader's 93.7 reference height also covers land where
           the river bitmap has no valid channel data, such as Iceland. */
        const bool ocean = index_at(layers->height, mx, my) < 94;
        output[x] = pack(ocean ? shade_water(layers, mx, my, wx, wy)
                               : shade_land(layers, mx, my, wx, wy));
        if (ocean)
            water->pixels[water->count++] = (struct water_pixel){
                (Uint32)((size_t)y * view->width + x), output[x], wx, wy
            };
    }
}

struct projected_vertex {
    float x, y, inverse_depth, u, v, light;
};

static float edge(struct projected_vertex a, struct projected_vertex b, float x, float y)
{
    return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
}

static void draw_frame_triangle(const struct map_view *view, struct projected_vertex a,
                                struct projected_vertex b, struct projected_vertex c)
{
    if (a.inverse_depth <= 0 || b.inverse_depth <= 0 || c.inverse_depth <= 0)
        return;
    const float area = edge(a, b, c.x, c.y);
    if (fabsf(area) < 0.001f)
        return;

    const struct layers *layers = view->layers;
    const SDL_Surface *texture = layers->frame_diffuse;
    const SDL_Rect tile = {0, 0, texture->w, texture->h};
    SDL_Surface *frame = view->water->frame;
    const int left = (int)SDL_clamp(floorf(SDL_min(a.x, SDL_min(b.x, c.x))), 0, frame->w);
    const int right = (int)SDL_clamp(ceilf(SDL_max(a.x, SDL_max(b.x, c.x))), 0, frame->w);
    const int top = (int)SDL_clamp(floorf(SDL_min(a.y, SDL_min(b.y, c.y))), 0, frame->h);
    const int bottom = (int)SDL_clamp(ceilf(SDL_max(a.y, SDL_max(b.y, c.y))), 0, frame->h);

    for (int y = top; y < bottom; ++y) {
        Uint32 *row = (Uint32 *)((Uint8 *)frame->pixels + (size_t)y * frame->pitch);
        for (int x = left; x < right; ++x) {
            const float wa = edge(b, c, x + 0.5f, y + 0.5f) / area;
            const float wb = edge(c, a, x + 0.5f, y + 0.5f) / area;
            const float wc = edge(a, b, x + 0.5f, y + 0.5f) / area;
            if (wa < 0 || wb < 0 || wc < 0)
                continue;
            const float pa = wa * a.inverse_depth, pb = wb * b.inverse_depth, pc = wc * c.inverse_depth;
            const float depth = pa + pb + pc;
            const size_t pixel = (size_t)y * frame->w + x;
            if (depth <= layers->frame_depth[pixel])
                continue;
            const float u = (pa * a.u + pb * b.u + pc * c.u) / depth;
            const float v = (pa * a.v + pb * b.v + pc * c.v) / depth;
            const float light = (pa * a.light + pb * b.light + pc * c.light) / depth;
            const struct rgb texel = rgb_repeat(texture, tile, u * tile.w, v * tile.h);
            row[x] = pack((struct rgb){(int)(texel.r * light), (int)(texel.g * light),
                                       (int)(texel.b * light)});
            layers->frame_depth[pixel] = depth;
            view->samples[pixel] = SDL_MAX_UINT32;
        }
    }
}

static void draw_map_frame(const struct map_view *view, struct camera camera)
{
    const struct layers *layers = view->layers;
    const struct ck2_model *model = &layers->frame_model;
    if (!layers->frame_depth)
        return;

    struct projected_vertex *points = malloc(model->vertex_count * sizeof *points);
    if (!points)
        return;
    /* Frame actors use roughly one model unit per 100 map texels. */
    const float scale = model->scale * 100.0f;
    for (size_t i = 0; i < model->vertex_count; ++i) {
        const struct ck2_model_vertex v = model->vertices[i];
        const float dy = view->world_height - v.z * scale - camera.y;
        const float dh = (v.y - 0.098f) * scale - camera.height;
        const float depth = dy * camera.forward_y + dh * camera.forward_height;
        if (depth < 1.0f) {
            points[i] = (struct projected_vertex){0};
            continue;
        }
        const float zoom = camera.focal / depth;
        points[i] = (struct projected_vertex){
            view->width * 0.5f + (v.x * scale - camera.x) * zoom,
            view->height * 0.5f - (dy * camera.forward_height - dh * camera.forward_y) * zoom,
            1.0f / depth, v.u, v.v, 0.65f + 0.35f * SDL_clamp(v.ny, 0.0f, 1.0f)
        };
    }
    for (size_t i = 0; i < model->index_count; i += 3)
        draw_frame_triangle(view, points[model->indices[i]], points[model->indices[i + 1]],
                            points[model->indices[i + 2]]);
    free(points);
}

static struct rgb unpack(Uint32 color)
{
    return (struct rgb){(int)((color >> 16) & 255), (int)((color >> 8) & 255),
                        (int)(color & 255)};
}

static void draw_tree(SDL_Surface *frame, const struct layers *layers, struct camera camera,
                      int x, int y, float height, Uint32 seed)
{
    const float dy = (float)y - camera.y;
    const float dh = height - camera.height;
    const float depth = dy * camera.forward_y + dh * camera.forward_height;
    if (depth < 1.0f)
        return;
    const float scale = camera.focal / depth;
    const float screen_y = frame->h * 0.5f -
        (dy * camera.forward_height - dh * camera.forward_y) * scale;
    const float screen_x = frame->w * 0.5f + ((float)x - camera.x) * scale;
    if (screen_x < -4 || screen_y < -4 || screen_x >= frame->w + 4 || screen_y >= frame->h + 4)
        return;
    const int center_x = (int)screen_x, bottom = (int)screen_y;
    const int size = SDL_clamp((int)(scale * 1.75f), 2, 5);
    const int tx = (int)((seed >> 8) % (unsigned)layers->pine->w);
    const int ty = (int)((seed >> 16) % (unsigned)layers->pine->h);
    const struct rgb tint = rgb_at(layers->tree_tint,
        x * layers->tree_tint->w / layers->color->w,
        y * layers->tree_tint->h / layers->color->h);
    const struct rgb foliage = mix(rgb_at(layers->pine, tx, ty), tint, 2, 10);
    for (int row = 0; row < size; ++row) {
        const int py = bottom - size + row + 1;
        if (py < 0 || py >= frame->h)
            continue;
        Uint32 *pixels = (Uint32 *)((Uint8 *)frame->pixels + (size_t)py * frame->pitch);
        const int radius = row * size / (2 * size - 1);
        for (int dx = -radius; dx <= radius; ++dx) {
            const int px = center_x + dx;
            if (px < 0 || px >= frame->w)
                continue;
            const int light = dx < 0 ? 11 : 2;
            const struct rgb leaf = {foliage.r / 2 + light, foliage.g / 2 + light + 8,
                                     foliage.b / 2 + light};
            pixels[px] = pack(mix(unpack(pixels[px]), leaf, row == 0 ? 5 : 7, 10));
        }
    }
}

static void draw_forest(SDL_Surface *frame, const struct layers *layers, struct camera camera,
                        struct map_bounds visible)
{
    const int width = layers->color->w, height = layers->color->h;
    if (visible.max_x < visible.min_x || visible.max_y < visible.min_y)
        return;
    /* Trees stand up to ~65 units above the ground they are drawn over, so a margin
       of 128 world pixels around the visible ground keeps every on-screen crown. */
    const int margin = 128;
    const int x_begin = SDL_max(0, visible.min_x - margin) / 3 * 3;
    const int y_begin = SDL_max(0, visible.min_y - margin) / 3 * 3;
    const int x_end = SDL_min(width - 3, visible.max_x + margin + 1);
    const int y_end = SDL_min(height - 3, visible.max_y + margin + 1);
    /* The tree definition bitmap marks forest cells. Scatter stable crowns inside those cells. */
    for (int y = y_begin; y < y_end; y += 3) {
        for (int x = x_begin; x < x_end; x += 3) {
            const Uint32 seed = hash_pixel((unsigned)x, (unsigned)y);
            if ((seed & 7) > 4)
                continue;
            const int tx = x + (int)((seed >> 3) % 3), ty = y + (int)((seed >> 5) % 3);
            if (!index_at(layers->trees, tx * layers->trees->w / width,
                          ty * layers->trees->h / height) || index_at(layers->rivers, tx, ty) != 255)
                continue;
            draw_tree(frame, layers, camera, tx, ty, elevation(layers, tx, ty), seed);
        }
    }
}

static struct rgb reflect_color(const struct map_water *water, float x, float y, float z)
{
    const float ax = fabsf(x), ay = fabsf(y), az = fabsf(z);
    float u, v, major;
    int face;
    if (ax >= ay && ax >= az) {
        face = x >= 0 ? 0 : 1;
        u = x >= 0 ? -z : z;
        v = -y;
        major = ax;
    } else if (ay >= az) {
        face = y >= 0 ? 2 : 3;
        u = x;
        v = y >= 0 ? z : -z;
        major = ay;
    } else {
        face = z >= 0 ? 4 : 5;
        u = z >= 0 ? x : -x;
        v = -y;
        major = az;
    }
    const SDL_Surface *surface = water->reflection[face];
    if (!surface || major < .001f)
        return (struct rgb){111, 128, 151};
    const int sx = (int)((u / major + 1) * .5f * (surface->w - 1));
    const int sy = (int)((v / major + 1) * .5f * (surface->h - 1));
    return rgb_at(surface, SDL_clamp(sx, 0, surface->w - 1), SDL_clamp(sy, 0, surface->h - 1));
}

enum { MAX_MAP_THREADS = 16 };

/* Runs `function(data)` on the calling thread and on one helper per extra CPU core.
   The function must pull its work from shared state in `data` (see the atomic counters). */
static void run_parallel(SDL_ThreadFunction function, void *data)
{
    SDL_Thread *threads[MAX_MAP_THREADS - 1];
    const int helpers = SDL_clamp(SDL_GetCPUCount() - 1, 0, MAX_MAP_THREADS - 1);
    int started = 0;
    while (started < helpers) {
        threads[started] = SDL_CreateThread(function, "map worker", data);
        if (!threads[started])
            break;
        ++started;
    }
    function(data);
    for (int i = 0; i < started; ++i)
        SDL_WaitThread(threads[i], NULL);
}

static void shade_wave_range(const struct map_view *view, int phase, size_t begin, size_t end)
{
    const struct map_water *water = view->water;
    const SDL_Surface *noise = water->noise, *normal = water->normal;
    for (size_t i = begin; i < end; ++i) {
        const struct water_pixel pixel = water->pixels[i];
        const float x = pixel.x, y = pixel.y;
        const SDL_Rect noise_tile = {0, 0, noise->w, noise->h};
        const struct rgb a = rgb_repeat(noise, noise_tile, x * 5 + y + phase,
                                        y * 5 + x / 2 + phase * 2);
        const struct rgb b = rgb_repeat(noise, noise_tile, x * 7 + y - phase * 3,
                                        y * 7 + x / 3 - phase);
        float nx = (a.r + b.r - 255) / 255.f;
        float ny = (a.b + b.b - 255) / 255.f + 1.5f;
        float nz = (a.g + b.g - 255) / 255.f;
        const float length = sqrtf(nx * nx + ny * ny + nz * nz);
        nx /= length;
        ny /= length;
        nz /= length;
        const float ex = x - water->camera_x, ey = -water->camera_height, ez = water->camera_y - y;
        const float dot = ex * nx + ey * ny + ez * nz;
        const float facing = SDL_clamp(-dot / sqrtf(ex * ex + ey * ey + ez * ez), 0.f, 1.f);
        const float f = 1 - .5f * facing;
        const float f2 = f * f, f4 = f2 * f2;
        const int fresnel = 20 + (int)(80 * f4 * f4 * f2);
        const struct rgb reflection = reflect_color(water, ex - 2 * dot * nx,
                                                    ey - 2 * dot * ny, ez - 2 * dot * nz);
        const struct rgb ripples = rgb_repeat(normal, (SDL_Rect){0, 0, normal->w, normal->h},
                                               x * 9 + y + phase * 3, y * 15 + x / 3 + phase);
        struct rgb color = mix(unpack(pixel.base), reflection, fresnel, 100);
        const int wave = (ripples.r - 128) / 13;
        color.r += wave;
        color.g += wave;
        color.b += wave;
        Uint32 *row = (Uint32 *)((Uint8 *)water->frame->pixels +
            (size_t)(pixel.screen / (unsigned)view->width) * water->frame->pitch);
        row[pixel.screen % (unsigned)view->width] = pack(color);
    }
}

enum { WAVE_CHUNK = 8192 };

struct wave_job {
    const struct map_view *view;
    int phase;
    size_t count;
    SDL_atomic_t next;
};

static int wave_worker(void *data)
{
    struct wave_job *job = data;
    const size_t chunks = (job->count + WAVE_CHUNK - 1) / WAVE_CHUNK;
    for (int chunk; (chunk = SDL_AtomicAdd(&job->next, 1)) >= 0 && (size_t)chunk < chunks;) {
        const size_t begin = (size_t)chunk * WAVE_CHUNK;
        shade_wave_range(job->view, job->phase, begin, SDL_min(job->count, begin + WAVE_CHUNK));
    }
    return 0;
}

static void shade_waves(struct map_view *view, Uint64 now)
{
    struct wave_job job = {.view = view, .phase = (int)((now / 65) % 4096), .count = view->water->count};
    if (job.count < 2 * WAVE_CHUNK)
        shade_wave_range(view, job.phase, 0, job.count);
    else
        run_parallel(wave_worker, &job);
}

bool map_view_animate(struct map_view *view, Uint64 now)
{
    if (!view->water || now - view->water->last_update < 65)
        return false;
    view->water->last_update = now;
    shade_waves(view, now);
    return SDL_UpdateTexture(view->texture, NULL, view->water->frame->pixels,
                             view->water->frame->pitch) == 0;
}

enum { ROW_BLOCK = 16 };

struct render_job {
    const struct map_view *view;
    const struct layers *layers;
    struct camera camera;
    bool campaign;
    int blocks;
    size_t *counts;
    struct map_bounds *bounds;
    SDL_atomic_t next;
};

/* Each block of rows keeps its water pixels in its own slice of water->pixels
   (a row has at most `width` of them), so workers never write to the same place. */
static int render_worker(void *data)
{
    struct render_job *job = data;
    const struct map_view *view = job->view;
    struct map_water *water = view->water;
    for (int block; (block = SDL_AtomicAdd(&job->next, 1)) >= 0 && block < job->blocks;) {
        const int first = block * ROW_BLOCK;
        const int last = SDL_min(view->height, first + ROW_BLOCK);
        struct map_water local = {.pixels = water->pixels + (size_t)first * view->width};
        struct map_bounds *bounds = &job->bounds[block];
        *bounds = (struct map_bounds){view->world_width, view->world_height, -1, -1};
        for (int y = first; y < last; ++y) {
            Uint32 *row = (Uint32 *)((Uint8 *)water->frame->pixels + (size_t)y * water->frame->pitch);
            render_row(view, job->layers, job->camera, y, row,
                       view->samples + (size_t)y * view->width, &local, bounds, job->campaign);
        }
        job->counts[block] = local.count;
    }
    return 0;
}

static int render_view(struct map_view *view, bool campaign)
{
    struct map_water *water = view->water;
    const struct layers *layers = view->layers;
    const struct camera camera = camera_for(view, campaign);
    water->camera_x = camera.x;
    water->camera_y = camera.y;
    water->camera_height = camera.height;
    water->count = 0;

    const int blocks = (view->height + ROW_BLOCK - 1) / ROW_BLOCK;
    size_t *counts = calloc((size_t)blocks, sizeof *counts);
    struct map_bounds *block_bounds = calloc((size_t)blocks, sizeof *block_bounds);
    if (!counts || !block_bounds) {
        free(counts);
        free(block_bounds);
        return SDL_OutOfMemory();
    }
    struct render_job job = {.view = view, .layers = layers, .camera = camera, .campaign = campaign,
                             .blocks = blocks, .counts = counts, .bounds = block_bounds};
    run_parallel(render_worker, &job);

    /* Gather the per-block water pixels and visible bounds. Blocks are in row order and
       each slice starts at or after the write position, so a forward move is safe. */
    size_t total = 0;
    struct map_bounds visible = {view->world_width, view->world_height, -1, -1};
    for (int block = 0; block < blocks; ++block) {
        struct water_pixel *slice = water->pixels + (size_t)block * ROW_BLOCK * view->width;
        if (slice != water->pixels + total)
            memmove(water->pixels + total, slice, counts[block] * sizeof *slice);
        total += counts[block];
        if (block_bounds[block].max_x >= 0) {
            bounds_add(&visible, block_bounds[block].min_x, block_bounds[block].min_y);
            bounds_add(&visible, block_bounds[block].max_x, block_bounds[block].max_y);
        }
    }
    water->count = total;
    free(counts);
    free(block_bounds);

    draw_forest(water->frame, layers, camera, visible);
    draw_map_frame(view, camera);
    size_t kept = 0;
    for (size_t i = 0; i < water->count; ++i) {
        const struct water_pixel pixel = water->pixels[i];
        const Uint32 *row = (const Uint32 *)((const Uint8 *)water->frame->pixels +
            (size_t)(pixel.screen / (unsigned)view->width) * water->frame->pitch);
        if (view->samples[pixel.screen] != SDL_MAX_UINT32 &&
            row[pixel.screen % (unsigned)view->width] == pixel.base)
            water->pixels[kept++] = pixel;
    }
    water->count = kept;
    shade_waves(view, SDL_GetTicks64());
    ++view->revision;
    return SDL_UpdateTexture(view->texture, NULL, water->frame->pixels, water->frame->pitch);
}

int map_view_build(struct map_view *view, SDL_Renderer *renderer, const char *root,
                   int width, int height, bool campaign)
{
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096)
        return SDL_SetError("Invalid map viewport dimensions");
    view->layers = calloc(1, sizeof *view->layers);
    view->water = calloc(1, sizeof *view->water);
    if (!view->layers || !view->water) {
        SDL_OutOfMemory();
        goto fail;
    }
    if (load_layers(view->layers, root) < 0)
        goto fail;
    view->width = width;
    view->height = height;
    view->world_width = view->layers->color->w;
    view->world_height = view->layers->color->h;
    if (campaign) {
        struct layers *layers = view->layers;
        char actor[32];
        SDL_snprintf(actor, sizeof actor, "frame%d", view->world_width);
        if (ck2_model_load(&layers->frame_model, root, actor) && *layers->frame_model.diffuse) {
            layers->frame_diffuse = ck2_load_dds_surface(root, layers->frame_model.diffuse);
            if (layers->frame_diffuse)
                layers->frame_depth = malloc((size_t)width * height * sizeof *layers->frame_depth);
        }
        if (!layers->frame_diffuse)
            layers->frame_diffuse = ck2_load_dds_surface(root, "gfx/models/Mapitems/Frame_Diffuse.dds");
    }
    view->zoom = 1;
    view->center_x = 790;
    view->center_y = (float)view->world_height - 1035;
    view->samples = malloc((size_t)width * height * sizeof *view->samples);
    view->water->pixels = malloc((size_t)width * height * sizeof *view->water->pixels);
    view->water->frame = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    view->water->noise = view->layers->noise;
    view->water->normal = view->layers->water_normal;
    memcpy(view->water->reflection, view->layers->reflection, sizeof view->water->reflection);
    view->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!view->samples || !view->water->pixels || !view->water->frame || !view->texture ||
        render_view(view, campaign) < 0)
        goto fail;
    if (!campaign) {
        view->layers->noise = view->layers->water_normal = NULL;
        memset(view->layers->reflection, 0, sizeof view->layers->reflection);
        free_layers(view->layers);
        free(view->layers);
        view->layers = NULL;
    }
    return 0;

fail:
    map_view_free(view);
    return -1;
}

bool map_view_move(struct map_view *view, float dx, float dy, int zoom_steps)
{
    if (!view->layers)
        return false;
    const float x = SDL_clamp(view->center_x + dx / view->zoom, 0.0f, (float)view->world_width);
    const float y = SDL_clamp(view->center_y + dy / view->zoom, 0.0f, (float)view->world_height);
    const float zoom = SDL_clamp(view->zoom * powf(1.24f, (float)zoom_steps), .31f, 3.0f);
    if (x == view->center_x && y == view->center_y && zoom == view->zoom)
        return false;
    view->center_x = x;
    view->center_y = y;
    view->zoom = zoom;
    return render_view(view, true) == 0;
}

void map_view_free(struct map_view *view)
{
    if (view->texture)
        SDL_DestroyTexture(view->texture);
    if (view->water) {
        SDL_FreeSurface(view->water->frame);
        if (!view->layers) {
            SDL_FreeSurface(view->water->noise);
            SDL_FreeSurface(view->water->normal);
            for (int i = 0; i < 6; ++i)
                SDL_FreeSurface(view->water->reflection[i]);
        }
        free(view->water->pixels);
        free(view->water);
    }
    if (view->layers) {
        free_layers(view->layers);
        free(view->layers);
    }
    free(view->samples);
    *view = (struct map_view){0};
}

bool map_view_pick(const struct map_view *view, int x, int y, SDL_Point *world)
{
    if (!view->samples || x < 0 || y < 0 || x >= view->width || y >= view->height)
        return false;
    const Uint32 index = view->samples[(size_t)y * view->width + x];
    if (index == SDL_MAX_UINT32)
        return false;
    world->x = (int)(index % (unsigned)view->world_width);
    world->y = (int)(index / (unsigned)view->world_width);
    return true;
}

/* Projects an authored settlement point onto the same heightfield rays used
 * by render_row(), rather than painting labels at a flat texture offset. */
bool map_view_project_ground(const struct map_view *view, float x, float y, SDL_Point *point)
{
    if (!view || !point || !view->layers || !view->width || !view->height ||
        x < 0 || y < 0 || x >= view->world_width || y >= view->world_height)
        return false;
    const struct camera camera = camera_for(view, true);
    const float dh = elevation(view->layers, (int)x, (int)y) - camera.height;
    const float dy = y - camera.y;
    const float depth = dy * camera.forward_y + dh * camera.forward_height;
    if (depth <= 1.0f)
        return false;
    const float factor = camera.focal / depth;
    const float px = view->width * 0.5f + (x - camera.x) * factor;
    const float py = view->height * 0.5f -
        (dy * camera.forward_height - dh * camera.forward_y) * factor;
    if (px < -64 || py < -64 || px >= view->width + 64 || py >= view->height + 64)
        return false;
    point->x = (int)lroundf(px);
    point->y = (int)lroundf(py);
    return true;
}
