#ifndef OPENCK2_REALMS_H
#define OPENCK2_REALMS_H

#include "assets.h"
#include "campaign.h"
#include "rulers.h"

struct realm_history;
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
bool realm_primary_title(const struct realm_map *, unsigned holder, struct realm_title_info *);
bool realm_county_info(const struct realm_map *, size_t, bool independent, struct realm_title_info *);
size_t realm_vassal_titles(const struct realm_map *, const struct realm_title_info *, size_t,
                           struct realm_title_info *, size_t);
const struct ruler_profile *realm_ruler(const struct realm_map *, unsigned id);
const struct ruler_profile *realm_province_ruler(const struct realm_map *, size_t province,
                                                  const char **title, unsigned *capital);
const char *realm_title_name(const struct realm_map *, const char *id);
unsigned realm_capital(const char *, const struct province_map *, const char *);
void realm_map_draw(const struct realm_map *, SDL_Renderer *);
void realm_map_free(struct realm_map *);

#endif
