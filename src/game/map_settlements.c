#include "map_settlements.h"
#include "map_positions.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int when_date(struct token value)
{
    char text[32]; int y, m, d;
    copy_token(text, sizeof text, value);
    return sscanf(text, "%d.%d.%d", &y, &m, &d) == 3 && y > 0 && m > 0 && d > 0
        ? y * 10000 + m * 100 + d : -1;
}

static void province_culture(char result[64], const char *data, struct campaign_date date)
{
    int best = -1;
    const int cutoff = date.year * 10000 + date.month * 100 + date.day;
    for (const char *p = data; p && *p;) {
        const struct token key = token(&p);
        if (!key.size || !is(token(&p), "=")) break;
        const struct token value = token(&p);
        if (is(key, "culture") && !is(value, "{")) {
            if (best < 0) copy_token(result, 64, value);
            continue;
        }
        if (!is(value, "{")) continue;
        const int when = when_date(key);
        if (when >= 0 && when <= cutoff && when >= best) {
            const struct token culture = field(p, "culture");
            if (culture.size) {
                copy_token(result, 64, culture);
                best = when;
            }
        }
        skip_block(&p);
    }
}

static void load_styles(struct map_settlement_scene *scene, const char *root)
{
    struct ck2_asset_files files = ck2_list_assets(root, "common/cultures");
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t n = strlen(name);
        if (n < 4 || strcmp(name + n - 4, ".txt")) continue;
        char *data = load_from_directory(root, "common/cultures", name);
        if (data) {
            (void)settlement_styles_parse_cultures(&scene->styles, data);
            SDL_free(data);
        }
    }
    ck2_free_asset_files(&files);
    char *gfx = load_data(root, "interface/settlements.gfx");
    if (gfx) {
        (void)settlement_styles_parse_actors(&scene->styles, gfx);
        SDL_free(gfx);
    }
}

static void estimate_centers(const struct province_map *map, struct map_settlement_scene *scene)
{
    /* Using a sparse pixel scan once at campaign initialization avoids
     * loading thousands of province histories on every camera movement. */
    if (!map->pixels || !scene->markers) return;
    struct sum { double x, y; unsigned n; int first_x, first_y; };
    struct sum *sums = calloc(map->count, sizeof *sums);
    if (!sums) return;
    const SDL_Surface *bmp = map->pixels;
    if (SDL_MUSTLOCK(bmp) && SDL_LockSurface(map->pixels) != 0) { free(sums); return; }
    for (int y = 0; y < bmp->h; y += 3) {
        const Uint32 *row = (const Uint32 *)((const Uint8 *)bmp->pixels + y * bmp->pitch);
        for (int x = 0; x < bmp->w; x += 3) {
            const Uint32 key = row[x] & 0xffffff;
            size_t lo = 0, hi = map->count;
            while (lo < hi) {
                const size_t mid = lo + (hi - lo) / 2;
                if (map->provinces[mid].color < key) lo = mid + 1;
                else hi = mid;
            }
            if (lo == map->count || map->provinces[lo].color != key) continue;
            struct sum *s = sums + lo;
            if (!s->n) { s->first_x = x; s->first_y = y; }
            s->x += x; s->y += y; ++s->n;
        }
    }
    if (SDL_MUSTLOCK(bmp)) SDL_UnlockSurface(map->pixels);
    for (size_t i = 0; i < scene->count; ++i) {
        struct map_settlement_marker *m = scene->markers + i;
        const struct province *p = province_by_id(map, m->province_id);
        if (!p) continue;
        const size_t idx = (size_t)(p - map->provinces);
        const struct sum *s = sums + idx;
        if (!s->n) continue;
        int x = (int)(s->x / s->n), y = (int)(s->y / s->n);
        /* A concave province's arithmetic centroid may be over a neighbor. */
        const struct province *check = province_at(map, x, y);
        if (!check || check->id != m->province_id) {
            x = s->first_x; y = s->first_y;
        }
        m->x = (float)x; m->y = (float)y;
    }
    free(sums);
}

static void apply_authored_positions(const char *root, const struct province_map *provinces,
                                     struct map_settlement_scene *scene)
{
    char *data = load_data(root, "map/positions.txt");
    if (!data) return;
    struct map_positions list = {0};
    if (map_positions_parse(&list, data))
        for (size_t i = 0; i < scene->count; ++i) {
            struct map_settlement_marker *m = scene->markers + i;
            const struct map_position *p = map_position_find(&list, m->province_id);
            if (!p) continue;
            const int x = (int)p->x;
            const int y_up = provinces->pixels->h - 1 - (int)p->y;
            const int y_down = (int)p->y;
            const struct province *in_up = province_at(provinces, x, y_up);
            const struct province *in_down = province_at(provinces, x, y_down);
            /* CK2 coordinates may be bottom-up. Validate against province
             * pixels instead of silently trusting either orientation. */
            if (in_up && in_up->id == m->province_id) {
                m->x = p->x; m->y = (float)y_up;
            } else if (in_down && in_down->id == m->province_id) {
                m->x = p->x; m->y = p->y;
            }
        }
    map_positions_destroy(&list);
    SDL_free(data);
}

int map_settlements_build(struct map_settlement_scene *scene, const char *root,
                           const struct province_map *provinces, const struct realm_map *realms,
                           struct campaign_date date)
{
    if (!scene || !root || !provinces || !realms) return SDL_SetError("Invalid settlement map inputs");
    map_settlements_free(scene);
    scene->date = date;
    scene->markers = calloc(provinces->count, sizeof *scene->markers);
    if (!scene->markers) return SDL_OutOfMemory();
    load_styles(scene, root);
    if (governments_load(&scene->governments, root) < 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Government definitions unavailable: %s", SDL_GetError());
    for (size_t i = 0; i < provinces->count; ++i) {
        const struct province *p = provinces->provinces + i;
        if (!p->land) continue;
        char *history = province_history_read(p, root);
        if (!history) continue;
        struct county_holdings county;
        holdings_read(&county, history, date);
        if (county.count) {
            struct map_settlement_marker *m = scene->markers + scene->count;
            const struct holding *capital = county.slots;
            m->province_id = p->id;
            m->type = capital->type;
            SDL_strlcpy(m->holding_id, capital->id, sizeof m->holding_id);
            province_culture(m->culture, history, date);
            struct realm_title_info title;
            if (realm_title_info(realms, capital->id, &title) && title.holder) {
                m->holder_id = title.holder;
                m->owner_known = true;
            } else if (realm_county_info(realms, i, false, &title) && title.holder) {
                /* This is a fallback for absent barony history, not a verified
                 * CK2 assignment. It is marked in UI tooltips. */
                m->holder_id = title.holder;
                m->county_holder_fallback = true;
            }
            if (m->holder_id) {
                const struct ruler_profile *owner = realm_ruler(realms, m->holder_id);
                if (owner) {
                    SDL_strlcpy(m->owner, owner->name, sizeof m->owner);
                    if (m->owner_known && owner->culture_key[0])
                        SDL_strlcpy(m->culture, owner->culture_key, sizeof m->culture);
                }
            }
            ++scene->count;
        }
        SDL_free(history);
    }
    if (scene->count) {
        struct ck2_translation *translations = calloc(scene->count, sizeof *translations);
        if (translations) {
            for (size_t i = 0; i < scene->count; ++i) {
                struct map_settlement_marker *m = scene->markers + i;
                SDL_strlcpy(m->name, m->holding_id, sizeof m->name);
                translations[i] = (struct ck2_translation){m->holding_id, m->name, sizeof m->name};
            }
            ck2_translate(root, translations, scene->count);
            free(translations);
        }
    }
    estimate_centers(provinces, scene);
    apply_authored_positions(root, provinces, scene);
    static const char *const types[] = {"", "castle", "city", "temple", "tribal", "nomad"};
    for (size_t i = 0; i < scene->count; ++i) {
        struct map_settlement_marker *m = scene->markers + i;
        const struct settlement_culture *style = settlement_culture_find(&scene->styles, m->culture);
        if (!style) continue;
        SDL_strlcpy(m->graphical, style->graphics, sizeof m->graphical);
        m->rgb = style->rgb;
        m->culture_color_known = style->has_color;
        const struct settlement_actor *actor = settlement_actor_find(&scene->styles,
            style->graphics, types[m->type], 1);
        if (actor) {
            SDL_strlcpy(m->actor, actor->key, sizeof m->actor);
            m->actor_known = true;
        }
    }
    return 0;
}

const struct map_settlement_marker *map_settlement_by_id(const struct map_settlement_scene *scene,
                                                          unsigned province_id)
{
    if (!scene || !province_id) return NULL;
    for (size_t i = 0; i < scene->count; ++i)
        if (scene->markers[i].province_id == province_id) return scene->markers + i;
    return NULL;
}

void map_settlements_free(struct map_settlement_scene *scene)
{
    if (!scene) return;
    free(scene->markers);
    settlement_styles_destroy(&scene->styles);
    government_catalog_destroy(&scene->governments);
    *scene = (struct map_settlement_scene){0};
}
