#ifndef OPENCK2_ECONOMY_CORE_H
#define OPENCK2_ECONOMY_CORE_H
#include <stdbool.h>
#include <stddef.h>

/* Engine-independent values loaded from the installed game's defines.lua.
 * Missing definitions are explicit: no guessed default tax or rank limits. */
struct ck2_economy_defines {
    double base_demesne, rank_demesne[5], stewardship_demesne, great_duke_demesne;
    double prestige_title[5], prestige_vassal[5];
    double monthly_prestige_per_diplomacy, monthly_piety_per_learning;
    bool demesne_ready, prestige_ready, attributes_ready;
};
bool ck2_define_number(const char *source, const char *key, double *out);
void ck2_economy_parse_defines(struct ck2_economy_defines *defs, const char *defines,
                               const char *static_modifiers);
/* Covers only base/rank/stewardship. Succession law, Jain, games rules,
 * patrician government and other modifiers are separate future consumers. */
unsigned ck2_demesne_base_limit(const struct ck2_economy_defines *, unsigned rank,
                                int stewardship, int spouse_stewardship, bool great_duke);
/* CK2 settlement and building tax_income is defined per year. */
double ck2_tax_monthly(double annual_income);
#endif
