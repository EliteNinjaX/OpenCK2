#ifndef OPENCK2_FAMILY_GAMEPLAY_H
#define OPENCK2_FAMILY_GAMEPLAY_H

#include "rulers.h"
#include <stdbool.h>
#include <stddef.h>

/* Runtime changes layered over bookmark character history. Do not mutate CK2 files. */
struct family_runtime_person {
    struct ruler_profile profile;
    struct campaign_date born;
    unsigned pregnancy_father;
    struct campaign_date due;
    bool has_due_date;
    bool matrilineal_marriage;
    struct campaign_date last_birth;
    struct campaign_date death_date;
    bool has_death_date;
};
struct family_gameplay {
    struct family_runtime_person *people;
    size_t count, capacity;
    unsigned next_id;
    unsigned births;
    unsigned marriage_min_male, marriage_min_female, max_birth_age;
    unsigned post_birth_wait_months;
};

enum family_status {
    FAMILY_OK, FAMILY_NO_PERSON, FAMILY_ALREADY_MARRIED, FAMILY_TOO_YOUNG,
    FAMILY_RELATED, FAMILY_NOT_PARTNERS, FAMILY_ALREADY_PREGNANT,
    FAMILY_NOT_ELIGIBLE, FAMILY_NO_MEMORY, FAMILY_INVALID_DATE, FAMILY_DEAD
};

void family_gameplay_free(struct family_gameplay *);
/* Read marriage and maternity thresholds from common/defines.lua. */
bool family_gameplay_rules(struct family_gameplay *, const char *defines);
/* Only mutable copies are stored. Original profiles remain authoritative for
 * people who have not been modified by gameplay. */
const struct ruler_profile *family_gameplay_find(const struct family_gameplay *, unsigned id);
bool family_gameplay_get(const struct family_gameplay *, const struct ruler_profile *original,
                         struct ruler_profile *out);
enum family_status family_gameplay_marriage(struct family_gameplay *,
       const struct ruler_profile *, const struct ruler_profile *);
enum family_status family_gameplay_marriage_type(struct family_gameplay *,
       const struct ruler_profile *, const struct ruler_profile *, bool matrilineal);
enum family_status family_gameplay_divorce(struct family_gameplay *, unsigned, unsigned);
/* Import one-sided historical spouse links without rewriting bookmark data. */
enum family_status family_gameplay_import_marriage(struct family_gameplay *,
       const struct ruler_profile *, const struct ruler_profile *);
enum family_status family_gameplay_conceive(struct family_gameplay *,
       const struct ruler_profile *, const struct ruler_profile *, struct campaign_date);
/* Advance at most one day per invocation. A born child can be fetched by ID. */
unsigned family_gameplay_day(struct family_gameplay *, struct campaign_date);
/* Explicit debugging/gameplay lifecycle: generate a baby in a valid union now, or
 * mark a historical/runtime person deceased without modifying CK2 files. */
enum family_status family_gameplay_birth(struct family_gameplay *,
       const struct ruler_profile *, const struct ruler_profile *, struct campaign_date, unsigned *);
enum family_status family_gameplay_death(struct family_gameplay *,
       const struct ruler_profile *, struct campaign_date);
const struct campaign_date *family_gameplay_death_date(const struct family_gameplay *, unsigned);
bool family_gameplay_name_child(struct family_gameplay *, unsigned id, const char *name);
size_t family_gameplay_children(const struct family_gameplay *, unsigned parent,
                                unsigned *ids, size_t capacity);
const char *family_status_description(enum family_status);

#endif
