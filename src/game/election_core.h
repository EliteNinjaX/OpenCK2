#ifndef OPENCK2_ELECTION_CORE_H
#define OPENCK2_ELECTION_CORE_H
#include <stdbool.h>
#include <stddef.h>

/* A read-only index of CK2's original common/succession_voting scripts.
 * Definitions and their scripted trigger bodies are preserved, NOT evaluated.
 * Keeping scripted eligibility unresolved is essential: most voting scopes
 * (council jobs, commander appointments, ambitions and opinions) have no
 * complete runtime representation yet. */
enum ck2_election_section {
    CK2_ELECTOR_SELECTION,
    CK2_ELECTOR_VOTE_STRENGTH,
    CK2_CANDIDATE_VOTE_SCORE,
    CK2_CANDIDATE_TRIGGER,
    CK2_ELECTION_SECTION_COUNT
};
struct ck2_election_rule {
    char id[80];
    char *script[CK2_ELECTION_SECTION_COUNT];
    int base_days[CK2_ELECTION_SECTION_COUNT];
    bool has_base_days[CK2_ELECTION_SECTION_COUNT];
    int max_electors;
    bool has_max_electors;
    unsigned modifier_count[CK2_ELECTION_SECTION_COUNT];
};
struct ck2_election_catalog {
    struct ck2_election_rule *rules;
    size_t count, capacity;
};
bool ck2_elections_parse(struct ck2_election_catalog *, const char *source);
const struct ck2_election_rule *ck2_election_find(const struct ck2_election_catalog *,
                                                   const char *id);
void ck2_elections_free(struct ck2_election_catalog *);

/* A provable, very narrow CK2 candidate-trigger condition. Only returns true
 * when 'NOT = { character = FROM }' exists at the top of candidate_trigger.
 * No other part of the candidate eligibility script is assumed satisfied. */
bool ck2_election_excludes_incumbent(const struct ck2_election_rule *);
/* Enumerates CK2 office keys mentioned by elector-selection scopes, without
 * asserting that any particular runtime ruler holds the role. */
size_t ck2_election_office_keys(const struct ck2_election_rule *,
                                char (*keys)[64],size_t capacity);

/* Safe, partial trigger evaluation. FALSE is an established disqualification;
 * UNKNOWN is never treated as eligible. The script evaluator recognizes only
 * ROOT/FROM, trait presence, character identity and Boolean AND/OR/NOT/NOR/NAND.
 * A trait not found in available history is UNKNOWN, not proven absent. */
enum ck2_election_truth { CK2_ELECTION_FALSE, CK2_ELECTION_TRUE, CK2_ELECTION_UNKNOWN };
struct ck2_election_candidate {
    unsigned ruler_id, candidate_id;
    const char *const *ruler_traits, *const *candidate_traits;
    size_t ruler_trait_count, candidate_trait_count;
};
enum ck2_election_truth ck2_election_candidate_trigger(
    const struct ck2_election_rule *, const struct ck2_election_candidate *);

#endif
