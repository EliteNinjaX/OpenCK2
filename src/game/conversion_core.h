#ifndef OPENCK2_CONVERSION_CORE_H
#define OPENCK2_CONVERSION_CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Runtime effects deliberately separate from immutable CK2 bookmark files.
 * They are not a reimplementation of CK2's scripted event scheduler. */
enum conversion_kind { CONVERSION_RELIGION = 0, CONVERSION_CULTURE = 1 };

enum conversion_status {
    CONVERSION_OK, CONVERSION_UNKNOWN_KEY, CONVERSION_INVALID_TARGET,
    CONVERSION_UNAVAILABLE, CONVERSION_ALREADY_SAME, CONVERSION_NO_MEMORY,
    CONVERSION_NO_MISSION
};
struct conversion_names {
    char (*items)[64];
    size_t count, capacity;
};
struct conversion_catalog {
    struct conversion_names names[2];
    bool inquisition_defined, settle_tribe_defined;
    char inquisition_events[80], settle_tribe_events[80];
};
struct conversion_province {
    unsigned id;
    char religion[64], culture[64];
    bool has_religion, has_culture;
};
struct conversion_person {
    unsigned id;
    char religion[64], culture[64];
    bool has_religion, has_culture;
};
struct conversion_mission {
    enum conversion_kind kind;
    unsigned province_id, actor_id;
    char target[64];
    bool active;
};
struct conversion_state {
    struct conversion_province *provinces;
    size_t province_count, province_capacity;
    struct conversion_person *people;
    size_t person_count, person_capacity;
    struct conversion_mission missions[2];
    unsigned revision;
};

/* Parse existing common/religions and common/cultures files: nested groups.
 * Mod definitions may add keys. An empty/unloaded catalog authorizes none. */
bool conversion_catalog_parse(struct conversion_catalog *, enum conversion_kind, const char *text);
bool conversion_catalog_has(const struct conversion_catalog *, enum conversion_kind, const char *key);
/* Read original job-action identifiers and event ID lists, not probabilities. */
bool conversion_catalog_jobs(struct conversion_catalog *, const char *job_actions);
void conversion_catalog_free(struct conversion_catalog *);
void conversion_state_free(struct conversion_state *);

enum conversion_status conversion_province_set(struct conversion_state *,
    const struct conversion_catalog *, unsigned province, enum conversion_kind,
    const char *key, const char *current);
enum conversion_status conversion_person_set(struct conversion_state *,
    const struct conversion_catalog *, unsigned person, enum conversion_kind,
    const char *key, const char *current);
const char *conversion_province_value(const struct conversion_state *,
    unsigned province, enum conversion_kind, const char *historical);
const char *conversion_person_value(const struct conversion_state *,
    unsigned person, enum conversion_kind, const char *historical);

/* Council missions are explicitly staged: no fictional monthly success
 * chance. Effect resolution is driven by a later real event evaluator or
 * an explicit developer command. */
enum conversion_status conversion_mission_assign(struct conversion_state *,
    const struct conversion_catalog *, enum conversion_kind, unsigned province,
    unsigned actor, const char *target, const char *current);
enum conversion_status conversion_mission_complete(struct conversion_state *,
    const struct conversion_catalog *, enum conversion_kind, const char *current);
const char *conversion_status_name(enum conversion_status);
#endif
