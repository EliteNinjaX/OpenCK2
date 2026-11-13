#ifndef OPENCK2_ECONOMY_H
#define OPENCK2_ECONOMY_H
#include "construction.h"
#include "realms.h"
#include "economy_core.h"
#include "title_prestige.h"
#include "law_core.h"

struct taxable_vassal_holding {
    struct holding holding;
    unsigned holder_id;
};
struct economy_snapshot {
    struct taxable_vassal_holding *vassal_holdings;
    size_t vassal_count, vassal_capacity;
    double annual_law_vassal_tax;
    unsigned vassal_tax_skipped;

    struct holding *direct_holdings;
    size_t direct_count, direct_capacity;
    /* Bookmark-correct direct counties. Used to recognize a settlement
     * founded by the player without guessing ownership from government. */
    unsigned *direct_counties;
    size_t direct_county_count, direct_county_capacity;
    unsigned holder_fallbacks, unknown_holders, missing_tax_definitions;
    double annual_direct_tax, monthly_prestige, monthly_piety;
    unsigned demesne_limit;
    bool has_demesne_limit, has_prestige_rate, has_piety_rate;
    struct ck2_economy_defines defines;
    struct title_prestige_ledger title_prestige;
};
/* Snapshot uses the selected bookmark's province/title history, not live
 * territorial changes. Income is direct holdings only; no unverified feudal
 * vassal, bishop, trade, laws, technology or modifier tax calculations. */
int economy_snapshot_build(struct economy_snapshot *, const char *,
                           const struct province_map *, const struct realm_map *,
                           const struct construction *, unsigned player, struct campaign_date);
/* Track an explicitly owned county in the bookmark snapshot. */
bool economy_snapshot_track_county(struct economy_snapshot *, unsigned province_id);
/* Refreshes finished buildings AND incorporates newly founded holdings in
 * tracked counties. Does not simulate transfers of counties between rulers. */
void economy_snapshot_refresh(struct economy_snapshot *, const struct construction *);
/* Computes only holdings of known direct vassals under explicitly selected
 * tax laws. Unknown obligations, modifiers, and religious exemptions omitted. */
void economy_snapshot_apply_laws(struct economy_snapshot *, const struct construction *,
                                 const struct ck2_law_catalog *, const struct ck2_law_state *);
void economy_snapshot_free(struct economy_snapshot *);
#endif
