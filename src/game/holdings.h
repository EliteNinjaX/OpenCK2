#ifndef OPENCK2_HOLDINGS_H
#define OPENCK2_HOLDINGS_H

#include "campaign.h"

enum holding_type { HOLDING_EMPTY, HOLDING_CASTLE, HOLDING_CITY, HOLDING_TEMPLE,
                    HOLDING_TRIBAL, HOLDING_NOMAD, HOLDING_TYPE_COUNT };
enum { HOLDING_LIMIT = 12, HOLDING_BUILDING_LIMIT = 48 };
struct holding {
    char id[72], name[96];
    enum holding_type type;
    int effective;
    /* Installed province history assigns completed buildings via b_x = key.
     * Preserve IDs for date-aware economy calculation. */
    char buildings[HOLDING_BUILDING_LIMIT][96];
    size_t building_count;
};
struct county_holdings {
    struct holding slots[HOLDING_LIMIT];
    size_t count, capacity;
    char capital[72], culture_key[64], religion_key[64];
    char culture_name[96], religion_name[96];
    int capital_date, capacity_date, culture_date, religion_date;
};
void holdings_read(struct county_holdings *, const char *, struct campaign_date);
void holdings_load(struct county_holdings *, const char *, const struct province *, struct campaign_date);

#endif
