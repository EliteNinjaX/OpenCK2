#include "title_prestige.h"
unsigned ck2_title_rank(const char *id)
{
    if (!id || id[1] != '_' || !id[2]) return 0;
    switch (id[0]) {
    case 'b': return 1;
    case 'c': return 2;
    case 'd': return 3;
    case 'k': return 4;
    case 'e': return 5;
    default: return 0;
    }
}
void ck2_title_prestige_add(struct title_prestige_ledger *ledger, const char *id,
                           bool vassal, const double rates[5])
{
    if (!ledger || !rates) return;
    unsigned rank = ck2_title_rank(id);
    if (!rank) return;
    const unsigned i = rank - 1;
    if (vassal) {
        ++ledger->vassals[i];
        ledger->vassal_monthly[i] += rates[i];
    } else {
        ++ledger->held[i];
        ledger->held_monthly[i] += rates[i];
    }
    ledger->total_monthly += rates[i];
}
