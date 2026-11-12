#ifndef OPENCK2_CONSTRUCTION_H
#define OPENCK2_CONSTRUCTION_H

#include "holdings.h"

struct building {
    char id[96], name[128];
    const char *requirements;
    enum holding_type type;
    unsigned gold, days;
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
    bool settlement_rules_ready, require_each;
    bool settlement_type_allowed[HOLDING_TYPE_COUNT];
};

enum building_status { BUILD_AVAILABLE, BUILD_COMPLETE, BUILD_RUNNING, BUILD_BUSY,
                       BUILD_REQUIREMENTS, BUILD_UNOWNED, BUILD_UNAFFORDABLE };

int construction_load(struct construction *, const char *root);
void construction_reset(struct construction *);
void construction_free(struct construction *);
void construction_day(struct construction *);
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

#endif
