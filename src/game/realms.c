#include "../open_launcher/asset_paths.h"
#include "realms.h"
#include "holdings.h"
#include "ck2_script.h"
#include "../open_launcher/install.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct title {
    char id[72], liege[72], parent[72], name_key[96], name[96];
    unsigned holder, color, pixels, capital;
    size_t order;
    bool territorial_name;
    int rank, min_x, max_x, min_y, max_y;
    Uint64 x, y, xx, yy, xy;
};

static int title_rank(struct token t)
{
    if (t.size < 3 || t.start[1] != '_')
        return 0;
    const char *ranks = "bcdke";
    const char *rank = strchr(ranks, t.start[0]);
    return rank ? (int)(rank - ranks) + 1 : 0;
}

static int compare_title(const void *a, const void *b)
{
    return strcmp(((const struct title *)a)->id, ((const struct title *)b)->id);
}

static int compare_title_order(const void *a, const void *b)
{
    const int by_id = compare_title(a, b);
    if (by_id)
        return by_id;
    const struct title *aa = a, *bb = b;
    return (aa->order > bb->order) - (aa->order < bb->order);
}

static int title_index(const struct title *titles, size_t count, const char *id)
{
    struct title key = {0};
    SDL_strlcpy(key.id, id, sizeof key.id);
    const struct title *result = bsearch(&key, titles, count, sizeof *titles, compare_title);
    return result ? (int)(result - titles) : -1;
}

static int walk_titles(const char **p, struct title **output, size_t *count, size_t *capacity, const char *parent)
{
    for (struct token key; (key = token(p)).size && !is(key, "}");) {
        if (!is(token(p), "="))
            return 0;
        if (!is(token(p), "{"))
            continue;
        const int rank = title_rank(key);
        if (!rank) {
            skip_block(p);
            continue;
        }
        if (*count == *capacity) {
            *capacity = *capacity ? *capacity * 2 : 1024;
            struct title *next = realloc(*output, *capacity * sizeof **output);
            if (!next)
                return SDL_OutOfMemory();
            *output = next;
        }
        struct title *title = &(*output)[(*count)++];
        *title = (struct title){.rank = rank, .order = *count, .min_x = 4096, .min_y = 4096};
        copy_token(title->id, sizeof title->id, key);
        SDL_strlcpy(title->parent, parent, sizeof title->parent);
        SDL_strlcpy(title->name_key, title->id, sizeof title->name_key);
        title->territorial_name = is(field(*p, "dynasty_title_names"), "no");
        title->capital = number(field(*p, "capital"));
        const char *color = *p;
        if (is(find(&color, "color"), "{")) {
            unsigned rgb[3];
            for (int c = 0; c < 3; ++c) {
                const unsigned channel = number(token(&color));
                rgb[c] = SDL_min(channel, 255u);
            }
            title->color = rgb[0] << 16 | rgb[1] << 8 | rgb[2];
        }
        char id[72];
        SDL_strlcpy(id, title->id, sizeof id);
        if (walk_titles(p, output, count, capacity, id) < 0)
            return -1;
    }
    return 0;
}

static int read_titles(const char *root, struct title **output, size_t *count)
{
    struct ck2_asset_files files = ck2_list_assets(root, "common/landed_titles");
    size_t capacity = 0;
    int status = 0;
    for (size_t i = 0; i < files.count && status == 0; ++i) {
        const char *name = files.names[i];
        const size_t length = strlen(name);
        if (length < 4 || strcmp(name + length - 4, ".txt"))
            continue;
        char *data = load_from_directory(root, "common/landed_titles", name);
        if (data) {
            const char *p = data;
            status = walk_titles(&p, output, count, &capacity, "");
            SDL_free(data);
        }
    }
    ck2_free_asset_files(&files);
    if (status < 0)
        return status;
    if (!*count)
        return SDL_SetError("No CK2 landed titles found");
    qsort(*output, *count, sizeof **output, compare_title_order);
    size_t n = 0;
    for (size_t i = 0; i < *count; ++i) {
        if (i + 1 == *count || compare_title(*output + i, *output + i + 1))
            (*output)[n++] = (*output)[i];
    }
    *count = n;
    return 0;
}

static int date_key(struct token t)
{
    char date[24], extra;
    int year, month, day;
    copy_token(date, sizeof date, t);
    return sscanf(date, "%4d.%2d.%2d%c", &year, &month, &day, &extra) == 3
        ? year * 10000 + month * 100 + day : 0;
}

static void read_history(const char *root, struct title *title, int cutoff)
{
    char path[128];
    SDL_snprintf(path, sizeof path, "history/titles/%s.txt", title->id);
    char *data = load_data(root, path);
    const char *p = data;
    int holder = 0, liege = 0, name = 0;
    while (p && *p) {
        const struct token key = token(&p);
        if (!key.size || !is(token(&p), "="))
            break;
        if (!is(token(&p), "{"))
            continue;
        const int date = date_key(key);
        if (date && date <= cutoff) {
            struct token value = field(p, "holder");
            if (value.size && date >= holder) {
                holder = date;
                title->holder = number(value);
            }
            value = field(p, "liege");
            if (value.size && date >= liege) {
                liege = date;
                copy_token(title->liege, sizeof title->liege, value);
            }
            if (date >= name) {
                value = field(p, "name");
                if (value.size) {
                    name = date;
                    copy_token(title->name_key, sizeof title->name_key, value);
                }
                if (is(field(p, "reset_name"), "yes")) {
                    name = date;
                    SDL_strlcpy(title->name_key, title->id, sizeof title->name_key);
                }
            }
        }
        skip_block(&p);
    }
    SDL_free(data);
}

static int *province_titles(const char *root, const struct province_map *map,
                            const struct title *titles, size_t count)
{
    int *index = malloc(map->count * sizeof *index);
    if (!index)
        return NULL;
    for (size_t i = 0; i < map->count; ++i) {
        char *data = province_history_read(&map->provinces[i], root);
        char key[72];
        copy_token(key, sizeof key, data ? field(data, "title") : (struct token){0});
        index[i] = title_index(titles, count, key);
        SDL_free(data);
    }
    return index;
}

unsigned realm_capital(const char *root, const struct province_map *map, const char *name)
{
    struct title *titles = NULL;
    size_t count = 0;
    unsigned capital = 0;
    if (read_titles(root, &titles, &count) == 0) {
        const int title = title_index(titles, count, name);
        if (title >= 0) {
            capital = titles[title].capital;
            if (!capital) {
                int *provinces = province_titles(root, map, titles, count);
                if (provinces) {
                    for (size_t i = 0; i < map->count && !capital; ++i)
                        if (provinces[i] == title)
                            capital = map->provinces[i].id;
                    free(provinces);
                }
            }
        }
    }
    free(titles);
    return capital;
}

static int independent(const struct title *titles, size_t count, int index)
{
    for (int depth = 0; depth < 12; ++depth) {
        const struct title *title = &titles[index];
        const int liege = title_index(titles, count, title->liege);
        if (liege >= 0 && liege != index && titles[liege].holder) {
            index = liege;
            continue;
        }
        int primary = index;
        if (title->holder) {
            for (size_t i = 0; i < count; ++i)
                if (titles[i].holder == title->holder && titles[i].rank > titles[primary].rank)
                    primary = (int)i;
        }
        if (primary == index)
            return index;
        index = primary;
    }
    return index;
}

static void label_bounds(struct realm_map *map, struct realm_label *label)
{
    int width = 0;
    for (const unsigned char *c = (const unsigned char *)label->name; *c; ++c)
        width += map->glyphs[*c].advance + 5;
    label->length = width * label->scale;
    const float radians = label->angle * .01745329252f;
    const float height = label->scale * 75;
    const int w = (int)(label->length * cosf(radians) + height * fabsf(sinf(radians)));
    const int h = (int)(label->length * fabsf(sinf(radians)) + height * cosf(radians));
    label->bounds = (SDL_Rect){label->center.x - w / 2, label->center.y - h / 2, w, h};
}

static int compare_area(const void *a, const void *b)
{
    const struct title *const *aa = a, *const *bb = b;
    return ((*aa)->pixels < (*bb)->pixels) - ((*aa)->pixels > (*bb)->pixels);
}

static bool label_fits(const struct realm_map *map, const struct realm_label *label,
                       int width, int height)
{
    if (label->bounds.x < 2 || label->bounds.x + label->bounds.w > width - 2 ||
        label->bounds.y < 2 || label->bounds.y + label->bounds.h > height - 2)
        return false;
    for (size_t i = 0; i < map->count; ++i)
        if (SDL_HasIntersection(&label->bounds, &map->labels[i].bounds))
            return false;
    return true;
}

static int place_labels(struct realm_map *map, struct title *titles,
                        size_t count, const int *screen, int width, int height)
{
    struct title **ordered = malloc(count * sizeof *ordered);
    free(map->labels);
    map->labels = calloc(count, sizeof *map->labels);
    map->count = 0;
    if (!ordered || !map->labels) {
        free(ordered);
        return SDL_OutOfMemory();
    }
    size_t n = 0;
    for (size_t i = 0; i < count; ++i)
        if (titles[i].pixels >= 230)
            ordered[n++] = &titles[i];
    qsort(ordered, n, sizeof *ordered, compare_area);
    for (size_t i = 0; i < n; ++i) {
        const struct title *title = ordered[i];
        if (!title->name[0])
            continue;
        struct realm_label label = {0};
        for (size_t c = 0; title->name[c] && c + 1 < sizeof label.name; ++c)
            label.name[c] = (char)toupper((unsigned char)title->name[c]);
        label.center = (SDL_Point){(int)(title->x / title->pixels), (int)(title->y / title->pixels)};
        if (screen[label.center.y * width + label.center.x] != title - titles) {
            unsigned best = SDL_MAX_UINT32;
            const SDL_Point mean = label.center;
            for (int y = title->min_y; y <= title->max_y; y += 2) {
                for (int x = title->min_x; x <= title->max_x; x += 2) {
                    if (screen[y * width + x] != title - titles)
                        continue;
                    const int dx = mean.x - x, dy = mean.y - y;
                    const unsigned distance = (unsigned)(dx * dx + dy * dy);
                    if (distance < best) {
                        best = distance;
                        label.center = (SDL_Point){x, y};
                    }
                }
            }
        }
        const double n = title->pixels;
        const double mx = title->x / n, my = title->y / n;
        const double xx = title->xx / n - mx * mx, yy = title->yy / n - my * my;
        const double xy = title->xy / n - mx * my;
        if (title->pixels > 1300 && hypot(xx - yy, 2 * xy) > (xx + yy) * .27)
            label.angle = SDL_clamp((float)(atan2(2 * xy, xx - yy) * 28.64789), -52.f, 52.f);
        const float size = sqrtf((float)title->pixels) * .0022f;
        label.scale = SDL_clamp(size, .14f, .47f);
        label_bounds(map, &label);
        if (label.bounds.w > title->max_x - title->min_x + 22) {
            label.scale *= (title->max_x - title->min_x + 22.0f) / label.bounds.w;
            label_bounds(map, &label);
        }
        const SDL_Point origin = label.center;
        static const int offsets[][2] = {{0, 0}, {-26, 0}, {26, 0}, {0, -25}, {0, 25},
                                        {-42, 25}, {42, -25}, {-32, -35}, {32, 35}};
        bool placed = false;
        for (int attempt = 0; attempt < 3 && !placed; ++attempt) {
            if (attempt == 1)
                label.angle *= .45f;
            if (attempt == 2)
                label.scale *= .85f;
            if (label.scale < .12f)
                break;
            for (size_t j = 0; j < SDL_arraysize(offsets); ++j) {
                label.center = (SDL_Point){origin.x + offsets[j][0], origin.y + offsets[j][1]};
                if (label.center.x < 0 || label.center.x >= width ||
                    label.center.y < 0 || label.center.y >= height ||
                    screen[label.center.y * width + label.center.x] != title - titles)
                    continue;
                label_bounds(map, &label);
                if (label_fits(map, &label, width, height)) {
                    map->labels[map->count++] = label;
                    placed = true;
                    break;
                }
            }
        }
        if (!placed && title->pixels > 3500) {
            const float start = label.scale;
            for (int attempt = 0; attempt < 3 && !placed; ++attempt) {
                label.angle = 0;
                label.scale = SDL_max(.135f, start * (.9f - .2f * attempt));
                unsigned best = SDL_MAX_UINT32;
                struct realm_label chosen = {0};
                for (int y = title->min_y; y <= title->max_y; y += 9) {
                    for (int x = title->min_x; x <= title->max_x; x += 9) {
                        const int dx = x - origin.x, dy = y - origin.y;
                        const unsigned distance = (unsigned)(dx * dx + dy * dy);
                        if (distance >= best || screen[y * width + x] != title - titles)
                            continue;
                        label.center = (SDL_Point){x, y};
                        label_bounds(map, &label);
                        if (label_fits(map, &label, width, height)) {
                            best = distance;
                            chosen = label;
                        }
                    }
                }
                if (best != SDL_MAX_UINT32) {
                    map->labels[map->count++] = chosen;
                    placed = true;
                }
            }
        }
    }
    free(ordered);
    return 0;
}

struct realm_history {
    struct title *titles;
    struct ruler_profile *rulers;
    int *province, *county;
    size_t count, ruler_count, province_count;
};

const struct ruler_profile *realm_ruler(const struct realm_map *map, unsigned id)
{
    if (!map->history || !id)
        return NULL;
    const struct ruler_profile match = {.id = id};
    return bsearch(&match, map->history->rulers, map->history->ruler_count,
                   sizeof match, ruler_compare);
}

static void title_info(const struct title *title, struct realm_title_info *info)
{
    SDL_strlcpy(info->id, title->id, sizeof info->id);
    SDL_strlcpy(info->name, *title->name ? title->name : title->id, sizeof info->name);
    SDL_strlcpy(info->liege, *title->liege ? title->liege :
                title->id[0] == 'b' ? title->parent : "", sizeof info->liege);
    info->holder = title->holder;
    info->capital = title->capital;
}

bool realm_title_info(const struct realm_map *map, const char *id, struct realm_title_info *info)
{
    const struct realm_history *h = map->history;
    const int index = h ? title_index(h->titles, h->count, id) : -1;
    if (index < 0)
        return false;
    title_info(&h->titles[index], info);
    return true;
}

bool realm_primary_title(const struct realm_map *map, unsigned holder, struct realm_title_info *info)
{
    const struct realm_history *h = map->history;
    if (!h || !holder)
        return false;
    const struct title *best = NULL;
    for (size_t i = 0; i < h->count; ++i) {
        const struct title *title = &h->titles[i];
        if (title->holder == holder && (!best || title->rank > best->rank))
            best = title;
    }
    if (!best)
        return false;
    title_info(best, info);
    return true;
}

bool realm_county_info(const struct realm_map *map, size_t province, bool independent,
                       struct realm_title_info *info)
{
    const struct realm_history *h = map->history;
    if (!h || province >= h->province_count)
        return false;
    const int index = (independent ? h->province : h->county)[province];
    if (index < 0)
        return false;
    title_info(&h->titles[index], info);
    return true;
}

bool realm_vacant_barony(const struct realm_map *map, const char *county,
                        const struct county_holdings *holdings, struct realm_title_info *info)
{
    const struct realm_history *history = map->history;
    const struct title *first = NULL;
    for (size_t i = 0; history && i < history->count; ++i) {
        const struct title *title = &history->titles[i];
        if (title->rank != 1 || title->holder || strcmp(title->parent, county) ||
            (first && first->order < title->order))
            continue;
        size_t slot = 0;
        while (slot < holdings->count && strcmp(title->id, holdings->slots[slot].id))
            ++slot;
        if (slot == holdings->count)
            first = title;
    }
    if (first)
        title_info(first, info);
    return first != NULL;
}

size_t realm_vassal_titles(const struct realm_map *map, const struct realm_title_info *liege,
                          size_t first, struct realm_title_info *out, size_t capacity)
{
    const struct realm_history *h = map->history;
    size_t count = 0;
    if (!h || !liege->holder)
        return 0;
    for (size_t i = 0; i < h->count; ++i) {
        const struct title *title = &h->titles[i];
        if (!title->holder || title->holder == liege->holder)
            continue;
        const char *parent = *title->liege ? title->liege : title->id[0] == 'b' ? title->parent : "";
        if (strcmp(parent, liege->id))
            continue;
        if (count >= first && count - first < capacity)
            title_info(title, &out[count - first]);
        ++count;
    }
    return count;
}

const struct ruler_profile *realm_province_ruler(const struct realm_map *map, size_t province,
                                                  const char **name, unsigned *capital)
{
    const struct realm_history *history = map->history;
    if (!history || province >= history->province_count || history->province[province] < 0)
        return NULL;
    const struct title *title = &history->titles[history->province[province]];
    *name = title->name;
    *capital = title->capital;
    return realm_ruler(map, title->holder);
}

const char *realm_title_name(const struct realm_map *map, const char *id)
{
    if (!map->history)
        return NULL;
    const int index = title_index(map->history->titles, map->history->count, id);
    return index < 0 ? NULL : map->history->titles[index].name;
}

static int history_load(struct realm_map *map, const char *root, const struct province_map *provinces,
                        struct campaign_date date)
{
    struct realm_history *h = calloc(1, sizeof *h);
    if (!h)
        return SDL_OutOfMemory();
    map->history = h;
    if (read_titles(root, &h->titles, &h->count) < 0)
        return -1;
    h->rulers = calloc(h->count, sizeof *h->rulers);
    struct ck2_translation *terms = malloc(h->count * sizeof *terms);
    if (!terms || !h->rulers) {
        free(terms);
        return SDL_OutOfMemory();
    }
    const int cutoff = date.year * 10000 + date.month * 100 + date.day;
    for (size_t i = 0; i < h->count; ++i) {
        struct title *title = &h->titles[i];
        read_history(root, title, cutoff);
        terms[i] = (struct ck2_translation){title->name_key, title->name, sizeof title->name};
        if (title->holder)
            h->rulers[h->ruler_count++].id = title->holder;
    }
    ck2_translate(root, terms, h->count);
    free(terms);
    qsort(h->rulers, h->ruler_count, sizeof *h->rulers, ruler_compare);
    size_t unique = 0;
    for (size_t i = 0; i < h->ruler_count; ++i)
        if (!i || h->rulers[i].id != h->rulers[i - 1].id)
            h->rulers[unique++].id = h->rulers[i].id;
    h->ruler_count = unique;
    rulers_load(h->rulers, h->ruler_count, root, date);
    for (size_t i = 0; i < h->count; ++i) {
        struct title *title = &h->titles[i];
        const struct ruler_profile *ruler = realm_ruler(map, title->holder);
        if (!title->territorial_name && ruler && ruler->dynasty_titles && ruler->dynasty[0])
            SDL_strlcpy(title->name, ruler->dynasty, sizeof title->name);
    }
    h->county = province_titles(root, provinces, h->titles, h->count);
    h->province = malloc(provinces->count * sizeof *h->province);
    if (!h->county || !h->province)
        return -1;
    h->province_count = provinces->count;
    for (size_t i = 0; i < h->province_count; ++i) {
        const int county = h->county[i];
        if (county >= 0) {
            h->titles[county].capital = provinces->provinces[i].id;
            if (!*h->titles[county].name)
                SDL_strlcpy(h->titles[county].name, provinces->provinces[i].name,
                            sizeof h->titles[county].name);
        }
        h->province[i] = county >= 0 && h->titles[county].holder
            ? independent(h->titles, h->count, county) : -1;
    }
    map->date = date;
    return 0;
}

int realm_map_build(struct realm_map *out, SDL_Renderer *renderer, const char *root,
                    const struct province_map *provinces, const struct map_view *view,
                    struct campaign_date date)
{
    if (!out->history || out->date.year != date.year || out->date.month != date.month ||
        out->date.day != date.day) {
        realm_map_free(out);
        if (history_load(out, root, provinces, date) < 0) {
            realm_map_free(out);
            return -1;
        }
    }
    if (out->overlay && out->revision == view->revision)
        return 0;
    struct title *titles = out->history->titles;
    const size_t count = out->history->count;
    const int *province = out->history->province;
    for (size_t i = 0; i < count; ++i) {
        titles[i].pixels = 0;
        titles[i].x = titles[i].y = titles[i].xx = titles[i].yy = titles[i].xy = 0;
        titles[i].min_x = titles[i].min_y = 4096;
        titles[i].max_x = titles[i].max_y = 0;
    }
    SDL_Surface *surface = NULL;
    SDL_Texture *overlay = NULL;
    int status = -1;
    const int w = view->width, h = view->height;
    int *screen = malloc((size_t)w * h * sizeof *screen);
    Uint32 *colors = malloc((size_t)w * h * sizeof *colors);
    surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!screen || !colors || !surface) {
        SDL_OutOfMemory();
        goto done;
    }
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            SDL_Point world;
            const bool on_map = map_view_pick(view, x, y, &world);
            const struct province *p = on_map ? province_at(provinces, world.x, world.y) : NULL;
            colors[y * w + x] = on_map ? province_map_color(provinces, world.x, world.y) : SDL_MAX_UINT32;
            const int id = p ? province[p - provinces->provinces] : -1;
            screen[y * w + x] = id;
            if (id >= 0) {
                struct title *t = &titles[id];
                ++t->pixels;
                t->x += x;
                t->y += y;
                t->xx += (Uint64)x * x;
                t->yy += (Uint64)y * y;
                t->xy += (Uint64)x * y;
                t->min_x = SDL_min(t->min_x, x);
                t->max_x = SDL_max(t->max_x, x);
                t->min_y = SDL_min(t->min_y, y);
                t->max_y = SDL_max(t->max_y, y);
            }
        }
    }
    for (int y = 0; y < h; ++y) {
        Uint32 *row = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch);
        for (int x = 0; x < w; ++x) {
            const int i = y * w + x, id = screen[i];
            const bool border = (x > 0 && screen[i - 1] != id) || (y > 0 && screen[i - w] != id);
            const unsigned color = id >= 0 ? titles[id].color : 0;
            const unsigned red = (color >> 16) & 255, green = (color >> 8) & 255, blue = color & 255;
            const unsigned neutral = (red + green + blue + 145) / 4;
            const unsigned blended = ((red * 4 + neutral) / 5) << 16 |
                                     ((green * 4 + neutral) / 5) << 8 |
                                     ((blue * 4 + neutral) / 5);
            const Uint32 province_color = colors[i];
            const bool local_border = province_color != SDL_MAX_UINT32 &&
                ((x > 0 && colors[i - 1] != province_color && colors[i - 1] != SDL_MAX_UINT32) ||
                 (y > 0 && colors[i - w] != province_color && colors[i - w] != SDL_MAX_UINT32));
            if (id < 0)
                row[x] = local_border ? 0x48314052u : 0;
            else
                row[x] = border ? 0xbf302630u : local_border ? (0xc0000000u | ((blended & 0xfefefe) >> 1))
                    : (0xa0000000u | blended);
        }
    }
    overlay = SDL_CreateTextureFromSurface(renderer, surface);
    if (!out->font) {
        out->font = ck2_load_texture(renderer, root, "gfx/fonts/Mapfont.dds");
        if (!out->font || ck2_load_font_metrics(out->glyphs, root, "gfx/fonts/Mapfont.fnt") < 0)
            goto done;
    }
    if (!overlay || place_labels(out, titles, count, screen, w, h) < 0)
        goto done;
    SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND);
    if (out->overlay)
        SDL_DestroyTexture(out->overlay);
    out->overlay = overlay;
    overlay = NULL;
    out->revision = view->revision;
    status = 0;

done:
    if (overlay)
        SDL_DestroyTexture(overlay);
    SDL_FreeSurface(surface);
    free(screen);
    free(colors);
    return status;
}

void realm_map_draw(const struct realm_map *map, SDL_Renderer *renderer)
{
    if (!map->overlay)
        return;
    SDL_RenderCopy(renderer, map->overlay, NULL, NULL);
    for (size_t i = 0; i < map->count; ++i) {
        const struct realm_label *label = &map->labels[i];
        const float radians = label->angle * .01745329252f;
        const float cosine = cosf(radians), sine = sinf(radians);
        float advance = -label->length / 2;
        for (const unsigned char *c = (const unsigned char *)label->name; *c; ++c) {
            const struct glyph *glyph = &map->glyphs[*c];
            const float gx = advance + (glyph->x_offset + glyph->area.w * .5f) * label->scale;
            const float gy = (glyph->y_offset - 43 + glyph->area.h * .5f) * label->scale;
            const int w = (int)(glyph->area.w * label->scale), h = (int)(glyph->area.h * label->scale);
            SDL_Rect target = {(int)(label->center.x + gx * cosine - gy * sine - w * .5f),
                (int)(label->center.y + gx * sine + gy * cosine - h * .5f), w, h};
            if (w && h) {
                SDL_SetTextureColorMod(map->font, 195, 178, 142);
                SDL_Rect shadow = target;
                ++shadow.x;
                ++shadow.y;
                SDL_RenderCopyEx(renderer, map->font, &glyph->area, &shadow, label->angle, NULL, SDL_FLIP_NONE);
                SDL_SetTextureColorMod(map->font, 39, 30, 24);
                SDL_RenderCopyEx(renderer, map->font, &glyph->area, &target, label->angle, NULL, SDL_FLIP_NONE);
            }
            advance += (glyph->advance + 5) * label->scale;
        }
    }

    SDL_SetTextureColorMod(map->font, 255, 255, 255);
}

void realm_map_free(struct realm_map *map)
{
    if (map->overlay)
        SDL_DestroyTexture(map->overlay);
    if (map->font)
        SDL_DestroyTexture(map->font);
    free(map->labels);
    if (map->history) {
        free(map->history->titles);
        free(map->history->province);
        free(map->history->county);
        free(map->history->rulers);
        free(map->history);
    }
    *map = (struct realm_map){0};
}
