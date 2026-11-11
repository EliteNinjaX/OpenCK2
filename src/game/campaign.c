#define _POSIX_C_SOURCE 200809L
#include "../open_launcher/asset_paths.h"
#include "campaign.h"
#include "../open_launcher/install.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void campaign_start(struct campaign *campaign)
{
    *campaign = (struct campaign){.date = {1066, 9, 15}, .treasury = 50};
}

unsigned campaign_monthly_income(const struct campaign *campaign)
{
    return 5 + 2 * campaign->improvements;
}

static int days_in_month(struct campaign_date date)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = date.year % 4 == 0 && (date.year % 100 != 0 || date.year % 400 == 0);
    return days[date.month - 1] + (date.month == 2 && leap);
}

void campaign_advance_day(struct campaign *campaign)
{
    struct campaign_date *date = &campaign->date;
    if (date->day < days_in_month(*date)) {
        ++date->day;
        return;
    }

    date->day = 1;
    if (date->month == 12) {
        date->month = 1;
        ++date->year;
    } else {
        ++date->month;
    }
    const unsigned income = campaign_monthly_income(campaign);
    if (campaign->treasury <= UINT_MAX - income)
        campaign->treasury += income;
    else
        campaign->treasury = UINT_MAX;
}

void campaign_advance_month(struct campaign *campaign)
{
    do {
        campaign_advance_day(campaign);
    } while (campaign->date.day != 1);
}

bool campaign_improve(struct campaign *campaign)
{
    if (campaign->treasury < IMPROVEMENT_COST || campaign->improvements >= IMPROVEMENT_LIMIT)
        return false;
    campaign->treasury -= IMPROVEMENT_COST;
    ++campaign->improvements;
    return true;
}

bool campaign_goal_met(const struct campaign *campaign)
{
    return campaign->improvements > 0 && campaign->treasury >= TREASURY_TARGET;
}

static int compare_color(const void *left, const void *right)
{
    const Uint32 a = ((const struct province *)left)->color;
    const Uint32 b = ((const struct province *)right)->color;
    return (a > b) - (a < b);
}

static int load_definitions(struct province_map *map, const char *root)
{
    char *path = ck2_asset_path(root, "map/definition.csv");
    FILE *file = path ? fopen(path, "r") : NULL;
    free(path);
    if (!file)
        return SDL_SetError("Could not open map/definition.csv");

    char *line = NULL;
    size_t line_size = 0, capacity = 0;
    int status = 0;
    while (getline(&line, &line_size, file) >= 0) {
        unsigned r, g, b;
        struct province entry = {0};
        if (sscanf(line, "%u;%u;%u;%u;%127[^;\r\n]", &entry.id, &r, &g, &b, entry.name) != 5 ||
            entry.id == 0 || r > 255 || g > 255 || b > 255)
            continue;
        if (map->count == capacity) {
            const size_t next = capacity ? capacity * 2 : 256;
            struct province *entries = realloc(map->provinces, next * sizeof *entries);
            if (!entries) {
                status = SDL_OutOfMemory();
                break;
            }
            map->provinces = entries;
            capacity = next;
        }
        entry.color = (r << 16) | (g << 8) | b;
        map->provinces[map->count++] = entry;
    }

    if (ferror(file) || (status == 0 && map->count == 0))
        status = SDL_SetError("Could not read province definitions");
    free(line);
    fclose(file);
    return status;
}

static int mark_land_provinces(struct province_map *map, const char *root)
{
    struct ck2_asset_files files = ck2_list_assets(root, "history/provinces");
    if (!files.count)
        return SDL_SetError("Could not open history/provinces");

    for (size_t file = 0; file < files.count; ++file) {
        unsigned id;
        if (sscanf(files.names[file], "%u -", &id) != 1)
            continue;
        for (size_t i = 0; i < map->count; ++i) {
            if (map->provinces[i].id == id)
                map->provinces[i].land = true;
        }
    }
    ck2_free_asset_files(&files);
    return 0;
}

int province_map_load(struct province_map *map, const char *root)
{
    char *path = ck2_asset_path(root, "map/provinces.bmp");
    if (!path)
        return SDL_OutOfMemory();
    SDL_Surface *bmp = SDL_LoadBMP(path);
    free(path);
    if (!bmp)
        return -1;
    map->pixels = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(bmp);
    if (!map->pixels || load_definitions(map, root) < 0 || mark_land_provinces(map, root) < 0)
        return -1;

    qsort(map->provinces, map->count, sizeof *map->provinces, compare_color);
    return 0;
}

void province_map_free(struct province_map *map)
{
    SDL_FreeSurface(map->pixels);
    free(map->provinces);
    *map = (struct province_map){0};
}

const struct province *province_by_id(const struct province_map *map, unsigned id)
{
    for (size_t i = 0; i < map->count; ++i) {
        if (map->provinces[i].id == id && map->provinces[i].land)
            return &map->provinces[i];
    }
    return NULL;
}

static Uint32 color_at(const SDL_Surface *pixels, int x, int y)
{
    const Uint8 *row = (const Uint8 *)pixels->pixels + (size_t)y * pixels->pitch;
    return ((const Uint32 *)row)[x] & 0xffffff;
}

Uint32 province_map_color(const struct province_map *map, int x, int y)
{
    if (!map->pixels || x < 0 || y < 0 || x >= map->pixels->w || y >= map->pixels->h)
        return SDL_MAX_UINT32;
    return color_at(map->pixels, x, y);
}

const struct province *province_at(const struct province_map *map, int x, int y)
{
    if (!map->pixels || x < 0 || y < 0 || x >= map->pixels->w || y >= map->pixels->h)
        return NULL;
    const struct province key = {.color = color_at(map->pixels, x, y)};
    const struct province *province = bsearch(&key, map->provinces, map->count,
                                              sizeof *map->provinces, compare_color);
    return province && province->land ? province : NULL;
}

static bool projected_province(const struct province_map *map, const struct map_view *view,
                               const struct province *province, int x, int y)
{
    const Uint32 index = view->samples[(size_t)y * view->width + x];
    if (index == SDL_MAX_UINT32)
        return false;
    return color_at(map->pixels, (int)(index % (unsigned)view->world_width),
                    (int)(index / (unsigned)view->world_width)) == province->color;
}

SDL_Texture *province_highlight(const struct province_map *map, SDL_Renderer *renderer,
                               const struct province *province, const struct map_view *view)
{
    SDL_Surface *mask = SDL_CreateRGBSurfaceWithFormat(0, view->width, view->height, 32,
                                                      SDL_PIXELFORMAT_ARGB8888);
    if (!mask)
        return NULL;
    for (int y = 0; y < mask->h; ++y) {
        Uint32 *row = (Uint32 *)((Uint8 *)mask->pixels + (size_t)y * mask->pitch);
        for (int x = 0; x < mask->w; ++x) {
            row[x] = 0;
            if (!projected_province(map, view, province, x, y))
                continue;
            const bool edge = x == 0 || y == 0 || x + 1 == mask->w || y + 1 == mask->h ||
                !projected_province(map, view, province, x - 1, y) ||
                !projected_province(map, view, province, x + 1, y) ||
                !projected_province(map, view, province, x, y - 1) ||
                !projected_province(map, view, province, x, y + 1);
            row[x] = edge ? 0xfff6db8b : 0x66edb93f;
        }
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, mask);
    SDL_FreeSurface(mask);
    if (texture)
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}
