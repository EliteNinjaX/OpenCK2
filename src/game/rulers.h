#ifndef OPENCK2_RULERS_H
#define OPENCK2_RULERS_H

#include "campaign.h"

enum ruler_stat { RULER_DIPLOMACY, RULER_MARTIAL, RULER_STEWARDSHIP, RULER_INTRIGUE,
                  RULER_LEARNING, RULER_STAT_COUNT };
extern const char *const ruler_stat_keys[RULER_STAT_COUNT];

enum { RULER_TRAIT_LIMIT = 24, RULER_CLAIM_LIMIT = 24 };
enum ruler_resource { RULER_WEALTH, RULER_PRESTIGE, RULER_PIETY, RULER_RESOURCE_COUNT };

struct ruler_profile {
    char traits[RULER_TRAIT_LIMIT][64];
    size_t trait_count;
    struct ruler_claim { char title[64]; bool weak; } claims[RULER_CLAIM_LIMIT];
    size_t claim_count;
    double resources[RULER_RESOURCE_COUNT];
    unsigned known_resources;
    int stats[RULER_STAT_COUNT];
    unsigned known_stats;
    unsigned father, mother, spouse;
    bool deceased;
    unsigned id, dynasty_id, age, religion_icon;
    char name[96], culture_key[64], religion_key[64], dynasty[96];
    char culture[96], religion[96], dna[32], properties[48];
    bool dynasty_titles, female, age_known;
};

void rulers_family_ids(const char *, const struct ruler_profile *, struct campaign_date,
                       unsigned *, size_t, unsigned *, size_t);
int ruler_compare(const void *, const void *);
void rulers_load(struct ruler_profile *, size_t, const char *, struct campaign_date);

#endif
