#ifndef OPENCK2_RULERS_H
#define OPENCK2_RULERS_H

#include "campaign.h"

struct ruler_profile {
    unsigned id, dynasty_id, age, religion_icon;
    char name[96], culture_key[64], religion_key[64], dynasty[96];
    char culture[96], religion[96], dna[32], properties[48];
    bool dynasty_titles, female, age_known;
};

int ruler_compare(const void *, const void *);
void rulers_load(struct ruler_profile *, size_t, const char *, struct campaign_date);

#endif
