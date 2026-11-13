#ifndef OPENCK2_MAP_THEMES_H
#define OPENCK2_MAP_THEMES_H
#include "campaign.h"
#include "realms.h"
#include "map_settlements.h"
#include "map_modes.h"
#include "construction.h"

struct map_theme {
    SDL_Texture *overlay;
    uint32_t *province_colors;
    size_t province_color_count;
    enum map_mode_id mode;
    unsigned map_revision;
    struct campaign_date date;
};

/* Province groups come from bookmark-era facts and title history. A subset
 * of modes can be produced without simulating CK2 wars, opinions or disease. */
int map_theme_build(struct map_theme *, SDL_Renderer *, const struct map_view *,
                    const struct province_map *, const struct realm_map *,
                    const struct map_settlement_scene *, const struct construction *,
                    const char *root, struct campaign_date, enum map_mode_id);
void map_theme_draw(const struct map_theme *, SDL_Renderer *);
void map_theme_free(struct map_theme *);
#endif
