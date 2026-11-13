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
/* In-memory succession transfers; CK2 history/titles stays read-only.
 * Rebuilds dependent realm identities after title ownership changes. */
bool realm_register_runtime_ruler(struct realm_map *, const struct ruler_profile *);
bool realm_transfer_runtime_title(struct realm_map *, const char *title, unsigned old_holder,
                                  unsigned heir);
/* Atomic title transfer across a single deceased holder's estate. All heirs
 * must already exist in the runtime ruler index. Primary heir gets top title. */
size_t realm_runtime_inherit_titles(struct realm_map *, unsigned from,
                                    const unsigned *heirs, size_t heir_count);

/* Only changed title owners are stored in native OpenCK2 checkpoints.
 * Missing titles/mods cause restore validation to fail, not to reassign
 * an unrelated landed title. */
struct realm_title_override { char id[72]; unsigned owner; };
size_t realm_runtime_title_overrides(const struct realm_map *, struct realm_title_override *, size_t);
bool realm_runtime_title_overrides_valid(const struct realm_map *,
                                         const struct realm_title_override *, size_t);
bool realm_runtime_title_overrides_restore(struct realm_map *,
                                           const struct realm_title_override *, size_t);

/* Authored landed-title color; independent from rendered realm overlay. */
bool realm_title_rgb(const struct realm_map *, const char *id, unsigned *rgb);
/* De jure parent from common/landed_titles, distinct from dated liege history. */
bool realm_dejure_parent_info(const struct realm_map *, const char *, struct realm_title_info *);
size_t realm_held_titles(const struct realm_map *, unsigned, struct realm_title_info *, size_t);
size_t realm_held_title_count(const struct realm_map *, unsigned);
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
/* Resolve the immediate vassal title below a province's independent ruler,
 * following historical de-facto lieges and higher personal titles. Returns
 * false if the authored history does not establish a complete chain. */
bool realm_direct_vassal_group(const struct realm_map *, size_t province,
                                char out[96]);
const struct ruler_profile *realm_ruler(const struct realm_map *, unsigned id);
/* Writable historical snapshot, never the original CK2 asset files. */
struct ruler_profile *realm_ruler_mutable(struct realm_map *, unsigned id);
const struct ruler_profile *realm_province_ruler(const struct realm_map *, size_t province,
                                                  const char **title, unsigned *capital);
const char *realm_title_name(const struct realm_map *, const char *id);
unsigned realm_capital(const char *, const struct province_map *, const char *);
void realm_map_draw(const struct realm_map *, SDL_Renderer *);
void realm_map_free(struct realm_map *);

#endif
