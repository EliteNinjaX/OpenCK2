#include "economy_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    const char *defines = "NDefines = { NDiplomacy = {\n"
        " DEMESNE_BASE_MAX_SIZE = 1.0,\n"
        " DEMESNE_MAX_SIZE_BARON_MULT = 1,\n"
        " DEMESNE_MAX_SIZE_COUNT_MULT = 1,\n"
        " DEMESNE_MAX_SIZE_DUKE_MULT = 1,\n"
        " DEMESNE_MAX_SIZE_KING_MULT = 3,\n"
        " DEMESNE_MAX_SIZE_EMPEROR_MULT = 4,\n"
        " DEMESNE_MAX_SIZE_GREAT_DUKE_BONUS = 1,\n"
        " DEMESNE_MAX_SIZE_STEWARDSHIP_MULT = 0.15,\n"
        " } NTitle = {\n"
        " BARON_TITLE_PRESTIGE = 0.025,\n"
        " COUNT_TITLE_PRESTIGE = 0.1,\n"
        " DUKE_TITLE_PRESTIGE = 0.2,\n"
        " KING_TITLE_PRESTIGE = 0.8,\n"
        " EMPEROR_TITLE_PRESTIGE = 1.6,\n"
        " BARON_AS_VASSAL_PRESTIGE = 0.025,\n"
        " COUNT_AS_VASSAL_PRESTIGE = 0.1,\n"
        " DUKE_AS_VASSAL_PRESTIGE = 0.4,\n"
        " KING_AS_VASSAL_PRESTIGE = 1.6,\n"
        " EMPEROR_AS_VASSAL_PRESTIGE = 3.2,\n} ";
    const char *modifiers = "static_diplomacy_mod = {\nmonthly_character_prestige = 0.01\n}\n"
                            "static_learning_mod = {\nmonthly_character_piety = 0.02\n}\n";
    struct ck2_economy_defines d;
    ck2_economy_parse_defines(&d, defines, modifiers);
    assert(d.demesne_ready && d.prestige_ready && d.attributes_ready);
    assert(ck2_demesne_base_limit(&d, 4, 10, 6, false) == 6);
    assert(ck2_demesne_base_limit(&d, 3, 10, 6, true) == 5);
    assert(fabs(d.prestige_title[4] - 1.6) < 1e-9);
    assert(fabs(ck2_tax_monthly(24.6) - 2.05) < 1e-9);
    assert(ck2_tax_monthly(-10) == 0);
    assert(!ck2_define_number("PRESTIGE = foo\n", "PRESTIGE", &d.base_demesne));
    puts("economy core: all assertions passed");
}
