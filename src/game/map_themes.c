#include "map_themes.h"
#include "map_theme_core.h"
#include "map_palette_core.h"
#include "../open_launcher/asset_paths.h"
#include "ck2_script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool date_equal(struct campaign_date a, struct campaign_date b)
{ return a.day == b.day && a.month == b.month && a.year == b.year; }

static void parent_key(const struct realm_map *realms, const char *county,
                       char prefix, char out[96])
{
    if (!county || !*county) return;
    char cursor[72];
    snprintf(cursor, sizeof cursor, "%s", county);
    for (unsigned depth = 0; depth < 12; ++depth) {
        struct realm_title_info parent;
        if (!realm_dejure_parent_info(realms, cursor, &parent) ||
            !strcmp(parent.id, cursor) || !parent.id[0]) return;
        if (parent.id[0] == prefix && parent.id[1] == '_') {
            snprintf(out, 96, "%s", parent.id);
            return;
        }
        snprintf(cursor, sizeof cursor, "%s", parent.id);
    }
}

static void load_palette(struct map_palette *palette, const char *root, const char *directory)
{
    if (!root || !directory) return;
    struct ck2_asset_files files = ck2_list_assets(root, directory);
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 4, ".txt")) continue;
        char *text = load_from_directory(root, directory, name);
        if (!text) continue;
        (void)map_palette_parse(palette, text);
        SDL_free(text);
    }
    ck2_free_asset_files(&files);
}

static uint32_t authored_or_fallback(const struct map_palette *palette, const char *key)
{
    uint32_t rgb = 0;
    return map_palette_find(palette, key, &rgb) && rgb ? rgb : map_theme_key_color(key);
}

static uint32_t province_theme_color(enum map_mode_id mode, size_t i,
                              const struct province_map *provinces,
                              const struct realm_map *realms,
                              const struct map_settlement_scene *scene,
                              const struct construction *construction,
                              const char *root, struct campaign_date date,
                              const struct map_palette *palette)
{
    char key[96] = {0};
    const struct map_province_facts *facts = i < scene->province_fact_count ?
        &scene->province_facts[i] : NULL;
    if (mode == MAP_MODE_RELIGION && facts) return authored_or_fallback(palette, facts->religion);
    if (mode == MAP_MODE_CULTURE && facts) return authored_or_fallback(palette, facts->culture);
    struct realm_title_info county;
    if (!realm_county_info(realms, i, false, &county)) return 0;
    if (mode == MAP_MODE_DUCHIES) parent_key(realms, county.id, 'd', key);
    else if (mode == MAP_MODE_KINGDOMS) parent_key(realms, county.id, 'k', key);
    else if (mode == MAP_MODE_EMPIRES) parent_key(realms, county.id, 'e', key);
    else if (mode == MAP_MODE_DIRECT_VASSALS) {
        /* Follow dated lieges and the holder's higher personal titles.
         * Unknown chains stay unknown instead of assigning a fake vassal. */
        if (!realm_direct_vassal_group(realms, i, key)) return 0;
    } else if (mode == MAP_MODE_DYNASTIES || mode == MAP_MODE_GOVERNMENTS) {
        const struct ruler_profile *r = realm_ruler(realms, county.holder);
        if (!r) return 0;
        if (mode == MAP_MODE_DYNASTIES && r->dynasty_id)
            snprintf(key, sizeof key, "dynasty:%u", r->dynasty_id);
        if (mode == MAP_MODE_GOVERNMENTS && r->government_key[0])
            snprintf(key, sizeof key, "%s", r->government_key);
    } else if (mode == MAP_MODE_ECONOMY) {
        if (!construction || !root || !provinces->provinces[i].history_path)
            return 0;
        char *history = province_history_read(&provinces->provinces[i], root);
        if (!history) return 0;
        struct county_holdings holdings;
        holdings_read(&holdings, history, date);
        SDL_free(history);
        double annual = 0;
        bool known = holdings.count != 0;
        for (size_t k = 0; k < holdings.count; ++k) {
            double income;
            if (!construction_holding_annual_tax(construction, &holdings.slots[k], &income)) {
                known = false;
                break;
            }
            annual += income;
        }
        return map_theme_economy_color(annual, known);
    }
    if (mode == MAP_MODE_GOVERNMENTS && key[0]) {
        const struct government_definition *gov = government_find(&scene->governments, key);
        if (gov && gov->color_defined && gov->color_rgb) return gov->color_rgb;
    }
    if ((mode == MAP_MODE_DUCHIES || mode == MAP_MODE_KINGDOMS ||
         mode == MAP_MODE_EMPIRES) && key[0]) {
        unsigned rgb = 0;
        if (realm_title_rgb(realms, key, &rgb) && rgb) return rgb;
    }
    return map_theme_key_color(key);
}

int map_theme_build(struct map_theme *theme, SDL_Renderer *renderer,
                    const struct map_view *view, const struct province_map *provinces,
                    const struct realm_map *realms, const struct map_settlement_scene *scene,
                    const struct construction *construction, const char *root,
                    struct campaign_date date, enum map_mode_id mode)
{
    if (!theme || !renderer || !view || !provinces || !realms || !scene ||
        !map_mode_definition(mode) || !map_mode_definition(mode)->implemented ||
        mode == MAP_MODE_TERRAIN || mode == MAP_MODE_REALMS)
        return -1;
    if (theme->overlay && theme->mode == mode && theme->map_revision == view->revision &&
        date_equal(theme->date, date)) return 0;

    /* Category colors do not depend on camera perspective. Cache once per
     * chosen mode/date; panning only reprojects the same province lookup. */
    const bool cached = theme->province_colors &&
        theme->province_color_count == provinces->count && theme->mode == mode &&
        date_equal(theme->date, date);
    uint32_t *lookup = theme->province_colors;
    if (!cached) {
        lookup = calloc(provinces->count, sizeof *lookup);
        if (!lookup) return -1;
        struct map_palette palette = {0};
        if (mode == MAP_MODE_RELIGION) load_palette(&palette, root, "common/religions");
        if (mode == MAP_MODE_CULTURE) load_palette(&palette, root, "common/cultures");
        for (size_t i = 0; i < provinces->count; ++i)
            if (provinces->provinces[i].land)
                lookup[i] = province_theme_color(mode, i, provinces, realms, scene,
                                                  construction, root, date, &palette);
        map_palette_free(&palette);
    }
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, view->width, view->height,
                                                         32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) { if (!cached) free(lookup); return -1; }
    uint32_t *previous = calloc((size_t)view->width, sizeof *previous);
    if (!previous) { if (!cached) free(lookup); SDL_FreeSurface(surface); return -1; }
    for (int y = 0; y < view->height; ++y) {
        uint32_t last = 0;
        uint32_t *row = (uint32_t *)((uint8_t *)surface->pixels + y * surface->pitch);
        for (int x = 0; x < view->width; ++x) {
            SDL_Point world;
            const struct province *p = map_view_pick(view, x, y, &world) ?
                province_at(provinces, world.x, world.y) : NULL;
            const uint32_t color = p ? lookup[p - provinces->provinces] : 0;
            if (!color) row[x] = 0;
            else {
                const bool border = (x > 0 && last && last != color) ||
                                    (y > 0 && previous[x] && previous[x] != color);
                row[x] = border ? (0xc0171820u) : (0xa0000000u | color);
            }
            last = color;
            previous[x] = color;
        }
    }
    free(previous);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) { if (!cached) free(lookup); return -1; }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (theme->overlay) SDL_DestroyTexture(theme->overlay);
    if (!cached) {
        free(theme->province_colors);
        theme->province_colors = lookup;
        theme->province_color_count = provinces->count;
    }
    theme->overlay = texture;
    theme->mode = mode;
    theme->map_revision = view->revision;
    theme->date = date;
    return 0;
}

void map_theme_draw(const struct map_theme *theme, SDL_Renderer *renderer)
{
    if (theme && theme->overlay) SDL_RenderCopy(renderer, theme->overlay, NULL, NULL);
}

void map_theme_free(struct map_theme *theme)
{
    if (!theme) return;
    if (theme->overlay) SDL_DestroyTexture(theme->overlay);
    free(theme->province_colors);
    *theme = (struct map_theme){0};
}
