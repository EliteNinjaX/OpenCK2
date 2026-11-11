#ifndef OPENCK2_HOLDINGS_H
#define OPENCK2_HOLDINGS_H

#include "campaign.h"

enum holding_type { HOLDING_EMPTY, HOLDING_CASTLE, HOLDING_CITY, HOLDING_TEMPLE,
                    HOLDING_TRIBAL, HOLDING_NOMAD, HOLDING_TYPE_COUNT };
enum { HOLDING_LIMIT = 12 };
struct holding {
    char id[72], name[96];
    enum holding_type type;
    int effective;
};
struct county_holdings {
    struct holding slots[HOLDING_LIMIT];
    size_t count, capacity;
    char capital[72];
    int capital_date, capacity_date;
};
void holdings_read(struct county_holdings *, const char *, struct campaign_date);
void holdings_load(struct county_holdings *, const char *, const struct province *, struct campaign_date);

#endif
