#ifndef OPENCK2_GOVERNMENT_CORE_H
#define OPENCK2_GOVERNMENT_CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Holding-type bits are deliberately independent of SDL and the UI module. */
enum government_holding_bit {
    GOVERNMENT_CASTLE = 1u << 0, GOVERNMENT_CITY = 1u << 1,
    GOVERNMENT_TEMPLE = 1u << 2, GOVERNMENT_TRIBAL = 1u << 3,
    GOVERNMENT_NOMAD = 1u << 4, GOVERNMENT_FORT = 1u << 5,
    GOVERNMENT_HOSPITAL = 1u << 6, GOVERNMENT_TRADE_POST = 1u << 7,
    GOVERNMENT_FAMILY_PALACE = 1u << 8
};
/* Original government ID lists, not holding-type masks. */
struct government_key_set { char ids[32][80]; size_t count; };
struct government_definition {
    char id[80], group[64], frame_suffix[64], title_prefix[64];
    uint32_t preferred_holdings, allowed_holdings, color_rgb;
    bool color_defined, has_potential, merchant_republic;
    bool allow_looting, can_build_holdings, can_build_holdings_defined;
    unsigned max_consorts;
    double aggression;
    struct government_key_set no_vassal_tax, vassal_call_to_arms, forced_contract;
    bool landed_inheritance, landed_inheritance_defined;
    bool have_gender_laws, have_gender_laws_defined;
    bool can_create_kingdoms, can_create_kingdoms_defined;
    bool can_build_forts, can_build_forts_defined;
    bool can_build_hospitals, can_build_hospitals_defined;
    char *potential_script; /* Raw original CK2 trigger block; NOT evaluated. */
};
struct government_catalog {
    struct government_definition *items;
    size_t count, capacity;
};
/* Reads groups and their definitions; later duplicate IDs replace earlier ones.
 * Returns false on allocation, malformed block or count limit failure. */
bool government_catalog_parse(struct government_catalog *, const char *source);
const struct government_definition *government_find(const struct government_catalog *, const char *id);
void government_catalog_destroy(struct government_catalog *);

/* Group membership from mounted common/religions, not guessed from names.
 * Temple-holder rules inherit group defaults and allow religion overrides. */
struct government_religion {
    char id[64], group[64];
    bool female_temple, male_temple;
};
struct government_religions {
    struct government_religion *entries;
    size_t count, capacity;
};
bool government_religions_parse(struct government_religions *, const char *source);
const struct government_religion *government_religion_find(
    const struct government_religions *, const char *id);
void government_religions_free(struct government_religions *);

/* Conservative subset of CK2's government `potential` language. No unsupported
 * trigger may silently evaluate true. Used for imperial title selection only;
 * not a general-purpose CK2 script engine. */
enum government_truth { GOVERNMENT_FALSE = 0, GOVERNMENT_TRUE = 1,
                        GOVERNMENT_UNRESOLVED = 2 };
struct government_context {
    const char *primary_title;             /* Actual holder's primary title ID. */
    const char *succession_law;            /* NULL if historical law is unknown. */
    bool game_started, save_game, is_patrician;
};
enum government_truth government_evaluate_potential(
    const struct government_definition *, const struct government_context *);
/* Extracts authored `title = ...` checks in a potential script. Returns the
 * complete count, even when output capacity is smaller. No inferred IDs. */
size_t government_potential_titles(const struct government_definition *,
                                   char (*titles)[80], size_t capacity);
/* Explicit history remains authoritative; inferred types require a fully true
 * potential expression and an installed government definition. */
const struct government_definition *government_resolve_special(
    const struct government_catalog *, const char *authored_key,
    const struct government_context *, bool *inferred);
uint32_t government_holding_mask(const char *key);
/* Conservative categorization from a *verified owned* capital holding. CK2
 * script `potential` conditions (e.g. religion, patrician, holy order) remain
 * outside this provisional classifier, so results must be marked inferred. */
struct government_holding_evidence {
    unsigned holder_id, province_id;
    uint32_t holding_bit;
    char title_id[72];
};
/* Picks one *verified* personal holding: prefer the primary barony, then
 * the recorded primary-title capital; ambiguous mixed estates stay unknown. */
uint32_t government_verified_holding(const struct government_holding_evidence *,
    size_t count, unsigned owner, const char *primary_title, unsigned primary_capital);
const struct government_definition *government_provisional_holding(
    const struct government_catalog *, uint32_t verified_holding);
bool government_key_set_has(const struct government_key_set *, const char *id);
/* Source-driven restrictions. No hardcoded government taxes or army bonuses. */
bool government_vassal_tax_exempt(const struct government_definition *liege,
                                 const struct government_definition *vassal);
bool government_uses_hereditary_succession(const struct government_definition *);
bool government_can_found_holding(const struct government_definition *);
/* Does the original government permit the selected law family? Election,
 * appointment and gender-law restrictions are enforced even for DEV changes. */
bool government_allows_law_group(const struct government_definition *, const char *group);

/* Not government assignment: returns candidates by *preferred* holding type only. */
size_t government_holding_candidates(const struct government_catalog *, uint32_t preferred,
                                     const struct government_definition **out, size_t capacity);
#endif
