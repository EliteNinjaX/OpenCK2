#include "economy_core.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

bool ck2_define_number(const char *source, const char *key, double *out)
{
    if (!source || !key || !out) return false;
    const size_t length = strlen(key);
    const char *p = source;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r') ++p;
        if (!strncmp(p, key, length) && (p[length] == ' ' || p[length] == '\t' || p[length] == '=')) {
            const char *s = p + length;
            while (*s == ' ' || *s == '\t') ++s;
            if (*s++ == '=') {
                while (*s == ' ' || *s == '\t') ++s;
                char *end;
                double value = strtod(s, &end);
                if (end != s && isfinite(value) &&
                    (*end == ',' || *end == '\n' || *end == '\r' || *end == ' ' ||
                     *end == '\t' || *end == '#' || (end[0] == '-' && end[1] == '-'))) {
                    *out = value;
                    return true;
                }
            }
        }
        const char *newline = strchr(p, '\n');
        if (!newline) break;
        p = newline + 1;
    }
    return false;
}

static bool read_keys(const char *data, const char *const *keys, double *values, size_t count)
{
    bool complete = true;
    for (size_t i = 0; i < count; ++i)
        complete &= ck2_define_number(data, keys[i], &values[i]);
    return complete;
}

void ck2_economy_parse_defines(struct ck2_economy_defines *defs, const char *defines,
                               const char *static_modifiers)
{
    if (!defs) return;
    *defs = (struct ck2_economy_defines){0};
    static const char *const rank_keys[] = {
        "DEMESNE_MAX_SIZE_BARON_MULT", "DEMESNE_MAX_SIZE_COUNT_MULT",
        "DEMESNE_MAX_SIZE_DUKE_MULT", "DEMESNE_MAX_SIZE_KING_MULT",
        "DEMESNE_MAX_SIZE_EMPEROR_MULT"
    };
    static const char *const title_keys[] = {
        "BARON_TITLE_PRESTIGE", "COUNT_TITLE_PRESTIGE", "DUKE_TITLE_PRESTIGE",
        "KING_TITLE_PRESTIGE", "EMPEROR_TITLE_PRESTIGE"
    };
    static const char *const vassal_keys[] = {
        "BARON_AS_VASSAL_PRESTIGE", "COUNT_AS_VASSAL_PRESTIGE",
        "DUKE_AS_VASSAL_PRESTIGE", "KING_AS_VASSAL_PRESTIGE", "EMPEROR_AS_VASSAL_PRESTIGE"
    };
    defs->demesne_ready = ck2_define_number(defines, "DEMESNE_BASE_MAX_SIZE", &defs->base_demesne) &&
        ck2_define_number(defines, "DEMESNE_MAX_SIZE_STEWARDSHIP_MULT", &defs->stewardship_demesne) &&
        ck2_define_number(defines, "DEMESNE_MAX_SIZE_GREAT_DUKE_BONUS", &defs->great_duke_demesne) &&
        read_keys(defines, rank_keys, defs->rank_demesne, 5);
    defs->prestige_ready = read_keys(defines, title_keys, defs->prestige_title, 5) &&
        read_keys(defines, vassal_keys, defs->prestige_vassal, 5);
    /* These modifiers are multipliers of the corresponding stats, not the
     * title monthly prestige itself. The static_modifiers lines are unique. */
    defs->attributes_ready = ck2_define_number(static_modifiers, "monthly_character_prestige",
                &defs->monthly_prestige_per_diplomacy) &&
        ck2_define_number(static_modifiers, "monthly_character_piety",
                &defs->monthly_piety_per_learning);
}

unsigned ck2_demesne_base_limit(const struct ck2_economy_defines *d, unsigned rank,
                                int stewardship, int spouse_stewardship, bool great_duke)
{
    if (!d || !d->demesne_ready || rank < 1 || rank > 5) return 0;
    double size = d->base_demesne + d->rank_demesne[rank - 1] +
        d->stewardship_demesne * (stewardship + spouse_stewardship) +
        (great_duke ? d->great_duke_demesne : 0);
    if (size <= 0) return 0;
    if (size >= UINT_MAX) return UINT_MAX;
    return (unsigned)floor(size + 1e-8);
}

double ck2_tax_monthly(double annual_income)
{
    return isfinite(annual_income) && annual_income > 0 ? annual_income / 12.0 : 0;
}
