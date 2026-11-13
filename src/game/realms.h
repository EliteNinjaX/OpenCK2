#ifndef OPENCK2_REALMS_H
#define OPENCK2_REALMS_H

#include "assets.h"
#include "campaign.h"
#include "rulers.h"

struct realm_history;
struct county_holdings;
struct realm_title_info {
    char id[72], name[96], liege[72];
    unsigned holder, capital;
};

struct realm_label {
    char name[96];
    SDL_Point center;
    float scale, angle, length;
    SDL_Rect bounds;
};

struct realm_map {
    SDL_Texture *overlay, *font;
    struct glyph glyphs[256];
    struct realm_label *labels;
    size_t count;
    struct campaign_date date;
    struct realm_history *history;
    unsigned revision;
};

int realm_map_build(struct realm_map *, SDL_Renderer *, const char *, const struct province_map *,
                    const struct map_view *, struct campaign_date);
bool realm_title_info(const struct realm_map *, const char *, struct realm_title_info *);
/* De jure parent from common/landed_titles, distinct from dated liege history. */
bool realm_dejure_parent_info(const struct realm_map *, const char *, struct realm_title_info *);
size_t realm_held_titles(const struct realm_map *, unsigned, struct realm_title_info *, size_t);
bool realm_primary_title(const struct realm_map *, unsigned holder, struct realm_title_info *);
bool realm_vacant_barony(const struct realm_map *, const char *county,
                        const struct county_holdings *, struct realm_title_info *);
bool realm_county_info(const struct realm_map *, size_t, bool independent, struct realm_title_info *);
/* County-holder and independent-ruler identities must not be conflated. */
struct realm_province_ownership {
    struct realm_title_info county, independent;
    bool has_county, has_independent;
};
bool realm_province_ownership(const struct realm_map *, size_t province,
                              struct realm_province_ownership *);

size_t realm_vassal_titles(const struct realm_map *, const struct realm_title_info *, size_t,
                           struct realm_title_info *, size_t);
/* Direct character vassals, deduplicated across all titles of the liege.
 * Uses dated title holder/liege history; not a full CK2 allegiance model. */
size_t realm_direct_vassals(const struct realm_map *, unsigned liege_holder, size_t first,
                            struct realm_title_info *, size_t capacity);
const struct ruler_profile *realm_ruler(const struct realm_map *, unsigned id);
const struct ruler_profile *realm_province_ruler(const struct realm_map *, size_t province,
                                                  const char **title, unsigned *capital);
const char *realm_title_name(const struct realm_map *, const char *id);
unsigned realm_capital(const char *, const struct province_map *, const char *);
void realm_map_draw(const struct realm_map *, SDL_Renderer *);
void realm_map_free(struct realm_map *);

#endif
