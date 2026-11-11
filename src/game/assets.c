#include "assets.h"

#include <SDL_image.h>
#include <stdio.h>
#include <string.h>

static Uint32 read_u32(const Uint8 *bytes)
{
    Uint32 value;
    memcpy(&value, bytes, sizeof value);
    return SDL_SwapLE32(value);
}

static SDL_Surface *load_dds(const char *path)
{
    SDL_RWops *file = SDL_RWFromFile(path, "rb");
    if (!file)
        return NULL;

    SDL_Surface *surface = NULL;
    Uint8 header[128];
    if (SDL_RWread(file, header, sizeof header, 1) != 1 ||
        memcmp(header, "DDS ", 4) != 0 || read_u32(header + 4) != 124 ||
        read_u32(header + 76) != 32 || read_u32(header + 80) != 0x41 ||
        read_u32(header + 88) != 32 || read_u32(header + 92) != 0x00ff0000 ||
        read_u32(header + 96) != 0x0000ff00 || read_u32(header + 100) != 0x000000ff ||
        read_u32(header + 104) != 0xff000000) {
        SDL_SetError("Expected an uncompressed ARGB8888 DDS");
        goto done;
    }

    const Uint32 height = read_u32(header + 12);
    const Uint32 width = read_u32(header + 16);
    if (!width || !height || width > 8192 || height > 8192 ||
        ((read_u32(header + 8) & 8) && read_u32(header + 20) != width * 4)) {
        SDL_SetError("Unsupported DDS dimensions or row pitch");
        goto done;
    }

    surface = SDL_CreateRGBSurfaceWithFormat(0, (int)width, (int)height, 32,
                                             SDL_PIXELFORMAT_BGRA32);
    if (surface && SDL_RWread(file, surface->pixels, (size_t)surface->pitch, height) != height) {
        SDL_FreeSurface(surface);
        surface = NULL;
        SDL_SetError("Truncated DDS pixel data");
    }

done:
    SDL_RWclose(file);
    return surface;
}

static char *asset_path(const char *root, const char *relative)
{
    const size_t size = strlen(root) + strlen(relative) + 2;
    char *path = SDL_malloc(size);
    if (path)
        SDL_snprintf(path, size, "%s/%s", root, relative);
    else
        SDL_OutOfMemory();
    return path;
}

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *root,
                                 const char *relative, bool gray)
{
    char *path = asset_path(root, relative);
    if (!path)
        return NULL;

    const size_t length = strlen(path);
    SDL_Surface *surface = length >= 4 && strcmp(path + length - 4, ".dds") == 0
        ? load_dds(path) : IMG_Load(path);
    if (surface && gray && surface->format->format != SDL_PIXELFORMAT_BGRA32) {
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_BGRA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
    }
    if (surface && gray) {
        /* Keep the original silhouette and alpha. */
        for (int y = 0; y < surface->h; ++y) {
            Uint8 *pixel = (Uint8 *)surface->pixels + y * surface->pitch;
            for (int x = 0; x < surface->w; ++x, pixel += 4) {
                const int luminance = (29 * pixel[0] + 150 * pixel[1] + 77 * pixel[2]) / 256;
                const Uint8 highlight = (Uint8)SDL_min(255, luminance + 48);
                pixel[0] = pixel[1] = pixel[2] = highlight;
            }
        }
    }
    SDL_Texture *texture = surface ? SDL_CreateTextureFromSurface(renderer, surface) : NULL;
    if (!texture)
        fprintf(stderr, "%s: %s\n", path, SDL_GetError());
    SDL_FreeSurface(surface);
    SDL_free(path);
    return texture;
}

SDL_Texture *ck2_load_texture(SDL_Renderer *renderer, const char *root, const char *relative)
{
    return load_texture(renderer, root, relative, false);
}

SDL_Texture *ck2_load_hover(SDL_Renderer *renderer, const char *root, const char *relative)
{
    return load_texture(renderer, root, relative, true);
}

static char *load_text(const char *root, const char *relative)
{
    char *path = asset_path(root, relative);
    if (!path)
        return NULL;
    char *text = SDL_LoadFile(path, NULL);
    if (!text)
        fprintf(stderr, "%s: %s\n", path, SDL_GetError());
    SDL_free(path);
    return text;
}

int ck2_load_glyphs(struct glyph glyphs[256], const char *root)
{
    char *font = load_text(root, "gfx/fonts/vic_22.fnt");
    if (!font)
        return -1;

    for (char *line = strtok(font, "\n"); line; line = strtok(NULL, "\n")) {
        unsigned id;
        struct glyph glyph;
        if (sscanf(line, "char id=%u x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d",
                   &id, &glyph.area.x, &glyph.area.y, &glyph.area.w, &glyph.area.h,
                   &glyph.x_offset, &glyph.y_offset, &glyph.advance) == 8 && id < 256)
            glyphs[id] = glyph;
    }
    SDL_free(font);
    if (glyphs['A'].advance <= 0)
        return SDL_SetError("Invalid vic_22 font metrics");
    return 0;
}

int menu_assets_load(struct menu_assets *assets, SDL_Renderer *renderer, const char *root)
{
    struct texture_asset {
        SDL_Texture **texture;
        const char *relative;
    } textures[] = {
        {&assets->map, "map/terrain/colormap.dds"},
        {&assets->title, "gfx/interface/frontend_title.dds"},
        {&assets->logo, "gfx/interface/pdx_dev_logo.dds"},
        {&assets->top, "gfx/interface/frontend_center_panel_top.dds"},
        {&assets->bottom, "gfx/interface/frontend_center_panel_bottom1.dds"},
        {&assets->button, "gfx/interface/big_button_232.dds"},
        {&assets->credits_frame, "gfx/interface/creditscreen_bg.dds"},
        {&assets->font, "gfx/fonts/vic_22.tga"},
    };

    for (size_t i = 0; i < SDL_arraysize(textures); ++i) {
        *textures[i].texture = ck2_load_texture(renderer, root, textures[i].relative);
        if (!*textures[i].texture)
            return -1;
    }

    assets->button_hover = ck2_load_hover(renderer, root, "gfx/interface/big_button_232.dds");
    if (!assets->button_hover)
        return -1;
    SDL_SetTextureBlendMode(assets->map, SDL_BLENDMODE_NONE);
    assets->credits = load_text(root, "interface/credits.txt");
    return assets->credits ? ck2_load_glyphs(assets->glyphs, root) : -1;
}

void menu_assets_free(struct menu_assets *assets)
{
    SDL_Texture *textures[] = {
        assets->map, assets->title, assets->logo, assets->top, assets->bottom,
        assets->button, assets->credits_frame, assets->font
    };
    for (size_t i = 0; i < SDL_arraysize(textures); ++i) {
        if (textures[i])
            SDL_DestroyTexture(textures[i]);
    }
    SDL_free(assets->credits);
}
