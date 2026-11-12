#ifndef OPENCK2_TITLE_PRESTIGE_H
#define OPENCK2_TITLE_PRESTIGE_H
#include <stddef.h>
#include <stdbool.h>
/* Rank order matches CK2's b_/c_/d_/k_/e_ title identifiers. */
struct title_prestige_ledger {
    size_t held[5], vassals[5];
    double held_monthly[5], vassal_monthly[5], total_monthly;
};
unsigned ck2_title_rank(const char *id);
void ck2_title_prestige_add(struct title_prestige_ledger *ledger, const char *id,
                           bool vassal, const double rates[5]);
#endif
