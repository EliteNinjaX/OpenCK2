#ifndef OPENCK2_CONSTRUCTION_H
#define OPENCK2_CONSTRUCTION_H

#include "holdings.h"
#include "levy_core.h"

struct building {
    char id[96], name[128];
    const char *requirements;
    enum holding_type type;
    unsigned gold, days;
    double annual_tax;
    struct levy_effect levy;
    char upgrades_from[96];
    bool supported;
};

struct construction {
    struct building *catalog;
    size_t count;
    struct building_source *sources;
    struct building_project *projects;
    size_t project_count;
    struct settlement_project *settlements;
    size_t settlement_count;
    unsigned settlement_gold[HOLDING_TYPE_COUNT], settlement_days[HOLDING_TYPE_COUNT];
    unsigned slot_cost_per_mille;
    double holding_base_tax[HOLDING_TYPE_COUNT];
    struct levy_effect holding_base_levy[HOLDING_TYPE_COUNT];
    bool holding_base_tax_known[HOLDING_TYPE_COUNT];
    bool settlement_rules_ready, require_each;
    bool settlement_type_allowed[HOLDING_TYPE_COUNT];
};

/* Runtime construction state only: catalog and CK2 source scripts are never copied.
 * Project IDs are stable CK2 building keys, not array indices from a mod load. */
struct construction_checkpoint_building {
    char holding[72], building[96];
    unsigned days;
};
struct construction_checkpoint_settlement {
    struct holding holding;
    unsigned province, days;
};
struct construction_checkpoint {
    struct construction_checkpoint_building *buildings;
    size_t building_count;
    struct construction_checkpoint_settlement *settlements;
    size_t settlement_count;
};
void construction_checkpoint_free(struct construction_checkpoint *);
bool construction_checkpoint_capture(const struct construction *, struct construction_checkpoint *);
/* Validation is side-effect-free. Restore allocates a replacement first and
 * changes the active construction state only on successful validation. */
bool construction_checkpoint_valid(const struct construction *, const struct construction_checkpoint *);
bool construction_checkpoint_restore(struct construction *, const struct construction_checkpoint *);

enum building_status { BUILD_AVAILABLE, BUILD_COMPLETE, BUILD_RUNNING, BUILD_BUSY,
                       BUILD_REQUIREMENTS, BUILD_UNOWNED, BUILD_UNAFFORDABLE };

int construction_load(struct construction *, const char *root);
void construction_reset(struct construction *);
void construction_free(struct construction *);
/* Advances all active projects one day. Returns true exactly when a
 * building or settlement finishes, so economy/HUD caches can refresh now. */
bool construction_day(struct construction *);
enum building_status construction_status(const struct construction *, const struct holding *,
                                         size_t index, bool owns, unsigned gold, unsigned *days);
bool construction_start(struct construction *, const struct holding *, size_t index,
                        bool owns, unsigned *gold);
void construction_history(struct construction *, const char *, struct campaign_date);

bool construction_settlement_quote(const struct construction *, const struct county_holdings *,
                                   unsigned province, enum holding_type, unsigned *gold, unsigned *days);
bool construction_settlement_start(struct construction *, const struct county_holdings *, unsigned province,
                                   const struct holding *, unsigned *gold);
unsigned construction_settlement_pending(const struct construction *, unsigned province, const struct holding **);
void construction_settlement_apply(const struct construction *, unsigned province, struct county_holdings *);
bool construction_owns_settlement(const struct construction *, const char *holding);
/* Safe read-only iteration over settlement orders. The project owns the
 * holding pointer; callers must not retain it past any state mutation.
 * Completed orders are permanent for the current campaign. */
size_t construction_settlement_count(const struct construction *);
bool construction_settlement_info(const struct construction *, size_t index,
                                  unsigned *province, const struct holding **holding,
                                  bool *complete);
/* Annual tax from one holding, using only verified historical/completed
 * building IDs, with upgrades replacing earlier levels. Returns false if
 * the installed static base tax for this holding type was unavailable. */
/* Potential troops, not currently raised/available levies. */
bool construction_holding_levy(const struct construction *, const struct holding *,
                              struct levy_potential *result);
bool construction_holding_annual_tax(const struct construction *, const struct holding *, double *result);


#endif
