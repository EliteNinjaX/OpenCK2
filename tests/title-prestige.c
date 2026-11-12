#include "../src/game/title_prestige.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    struct title_prestige_ledger l = {0};
    const double title[] = {0, .1, .2, .8, 1.6};
    const double vassal[] = {.01, .02, .03, .04, .05};
    assert(ck2_title_rank("k_england") == 4);
    assert(ck2_title_rank("e_byzantium") == 5);
    assert(ck2_title_rank("z_invalid") == 0);
    ck2_title_prestige_add(&l, "k_england", false, title);
    ck2_title_prestige_add(&l, "d_york", false, title);
    ck2_title_prestige_add(&l, "c_kent", true, vassal);
    ck2_title_prestige_add(&l, "invalid", true, vassal);
    assert(l.held[3] == 1 && l.held[2] == 1 && l.vassals[1] == 1);
    assert(l.total_monthly > 1.019 && l.total_monthly < 1.021);
    puts("title prestige: all assertions passed");
    return 0;
}
