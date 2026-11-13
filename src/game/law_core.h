#ifndef OPENCK2_LAW_CORE_H
#define OPENCK2_LAW_CORE_H
#include <stdbool.h>
#include <stddef.h>

/* An SDL-free catalog of authored CK2 law definitions. Only top-level effects
 * and numeric modifiers are interpreted; potential/allow/effect scripting
 * remains unexecuted, so changes are developer overrides, not legal votes. */
enum ck2_law_class { CK2_LAW_REALM, CK2_LAW_SUCCESSION, CK2_LAW_GENDER, CK2_LAW_CROWN };
struct ck2_law {
    char key[80], group[80], succession[48], gender[48];
    char election_base_type[80], election_voting_rules[80];
    enum ck2_law_class kind;
    double castle_tax, city_tax, temple_tax, tribal_tax;
    double demesne_size, vassal_limit;
    bool has_demesne_size, has_vassal_limit;
    double castle_min_levy, city_min_levy, temple_min_levy, tribal_min_levy;
    bool default_law, has_castle_tax, has_city_tax, has_temple_tax, has_tribal_tax;
    bool has_castle_levy, has_city_levy, has_temple_levy, has_tribal_levy;
};
struct ck2_law_catalog { struct ck2_law *items; size_t count, capacity; };
struct ck2_active_law { char group[80], key[80]; };
struct ck2_law_state { struct ck2_active_law *chosen; size_t count, capacity; };

bool ck2_law_parse(struct ck2_law_catalog *, const char *source);
const struct ck2_law *ck2_law_find(const struct ck2_law_catalog *, const char *key);
void ck2_law_catalog_free(struct ck2_law_catalog *);
void ck2_law_state_free(struct ck2_law_state *);
/* One explicit selection per law group. Does not evaluate CK2 allow/potential. */
bool ck2_law_select(struct ck2_law_state *, const struct ck2_law_catalog *, const char *key);
const struct ck2_law *ck2_law_selected(const struct ck2_law_state *,
                                       const struct ck2_law_catalog *, const char *group);
/* Reads explicitly authored history/titles/<id>.txt `law = key` assignments
 * at or before the bookmark date. Does not invent CK2 government defaults. */
bool ck2_law_title_history(struct ck2_law_state *, const struct ck2_law_catalog *,
                           const char *history, unsigned cutoff_yyyymmdd);

/* Returns false for unmapped holding type or absent/unknown tax law. */
bool ck2_law_holding_tax(const struct ck2_law_state *, const struct ck2_law_catalog *,
                         unsigned holding_type, double *tax_fraction);

/* Succession is only resolved for supported hereditary forms, with explicit
 * gender laws. No invented elective votes, Muslim open-succession or AI. */
enum ck2_succession_mode { CK2_SUCC_UNSUPPORTED, CK2_SUCC_PRIMOGENITURE,
                           CK2_SUCC_ULTIMOGENITURE, CK2_SUCC_SENIORITY, CK2_SUCC_GAVELKIND };
enum ck2_gender_mode { CK2_GENDER_UNSPECIFIED, CK2_GENDER_AGNATIC,
                       CK2_GENDER_COGNATIC, CK2_GENDER_EQUAL,
                       CK2_GENDER_ENATIC, CK2_GENDER_ENATIC_COGNATIC };
struct ck2_heir_candidate {
    unsigned id, father, mother, dynasty_id, age;
    bool female, alive, age_known;
};
enum ck2_succession_mode ck2_law_succession_mode(const struct ck2_law *);
enum ck2_gender_mode ck2_law_gender_mode(const struct ck2_law *);
/* Returns 0 if not enough evidence. Priority: oldest/youngest eligible child,
 * then compatible grandchild through predeceased child (not claimed to be
 * complete CK2 succession). Gavelkind shares titles amongst direct children. */
unsigned ck2_succession_heir(enum ck2_succession_mode, enum ck2_gender_mode,
                             unsigned dead_id, unsigned dynasty_id,
                             const struct ck2_heir_candidate *, size_t);
size_t ck2_succession_divide(enum ck2_succession_mode, enum ck2_gender_mode,
                             unsigned dead_id, unsigned dynasty_id,
                             const struct ck2_heir_candidate *, size_t,
                             unsigned *heirs, size_t capacity);
#endif
