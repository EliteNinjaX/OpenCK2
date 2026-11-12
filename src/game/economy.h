#ifndef OPENCK2_ECONOMY_H
#define OPENCK2_ECONOMY_H
#include "construction.h"
#include "realms.h"
#include "economy_core.h"

struct economy_snapshot {
    struct holding *direct_holdings;
    size_t direct_count, direct_capacity;
    unsigned holder_fallbacks, unknown_holders, missing_tax_definitions;
    double annual_direct_tax, monthly_prestige, monthly_piety;
    unsigned demesne_limit;
    bool has_demesne_limit, has_prestige_rate, has_piety_rate;
    struct ck2_economy_defines defines;
};
/* Snapshot uses the selected bookmark's province/title history, not live
 * territorial changes. Income is direct holdings only; no unverified feudal
 * vassal, bishop, trade, laws, technology or modifier tax calculations. */
int economy_snapshot_build(struct economy_snapshot *, const char *,
                           const struct province_map *, const struct realm_map *,
                           const struct construction *, unsigned player, struct campaign_date);
void economy_snapshot_refresh(struct economy_snapshot *, const struct construction *);
void economy_snapshot_free(struct economy_snapshot *);
#endif
