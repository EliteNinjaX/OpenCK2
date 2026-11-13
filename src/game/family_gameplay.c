#include "family_gameplay.h"
#include "economy_core.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool valid_date(struct campaign_date d)
{
    static const unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (d.year < 1 || d.month < 1 || d.month > 12 || d.day < 1) return false;
    unsigned max = days[d.month - 1];
    if (d.month == 2 && d.year % 4 == 0 && (d.year % 100 || d.year % 400 == 0)) ++max;
    return (unsigned)d.day <= max;
}
static int compare_dates(struct campaign_date a, struct campaign_date b)
{
    if (a.year != b.year) return (a.year > b.year) - (a.year < b.year);
    if (a.month != b.month) return (a.month > b.month) - (a.month < b.month);
    return (a.day > b.day) - (a.day < b.day);
}
static struct campaign_date add_days(struct campaign_date d, int count)
{
    for (int i = 0; i < count; ++i) {
        ++d.day;
        if (!valid_date(d)) {
            d.day = 1;
            if (++d.month == 13) { d.month = 1; ++d.year; }
        }
    }
    return d;
}
static struct family_runtime_person *find_mutable(struct family_gameplay *state, unsigned id)
{
    if (!state || !id) return NULL;
    for (size_t i = 0; i < state->count; ++i)
        if (state->people[i].profile.id == id) return &state->people[i];
    return NULL;
}
const struct ruler_profile *family_gameplay_find(const struct family_gameplay *state, unsigned id)
{
    if (!state || !id) return NULL;
    for (size_t i = 0; i < state->count; ++i)
        if (state->people[i].profile.id == id) return &state->people[i].profile;
    return NULL;
}
bool family_gameplay_get(const struct family_gameplay *state, const struct ruler_profile *original,
                         struct ruler_profile *out)
{
    if (!out) return false;
    if (original) {
        const struct ruler_profile *edited = family_gameplay_find(state, original->id);
        *out = edited ? *edited : *original;
        return true;
    }
    return false;
}
static struct family_runtime_person *ensure_person(struct family_gameplay *state,
                                                     const struct ruler_profile *profile)
{
    struct family_runtime_person *person = find_mutable(state, profile->id);
    if (person) return person;
    if (state->count == state->capacity) {
        size_t next = state->capacity ? state->capacity * 2 : 8;
        if (next < state->capacity || next > SIZE_MAX / sizeof *state->people) return NULL;
        void *memory = realloc(state->people, next * sizeof *state->people);
        if (!memory) return NULL;
        state->people = memory;
        state->capacity = next;
    }
    person = &state->people[state->count++];
    *person = (struct family_runtime_person){0};
    person->profile = *profile;
    return person;
}
static bool related(const struct ruler_profile *a, const struct ruler_profile *b)
{
    if (a->father == b->id || a->mother == b->id ||
        b->father == a->id || b->mother == a->id) return true;
    if ((a->father && a->father == b->father) ||
        (a->mother && a->mother == b->mother)) return true;
    return false;
}
bool family_gameplay_rules(struct family_gameplay *state, const char *defines)
{
    if (!state || !defines) return false;
    double male, female, maximum, wait;
    if (!ck2_define_number(defines, "AGE_OF_MARRIAGE_MALE", &male) ||
        !ck2_define_number(defines, "AGE_OF_MARRIAGE_FEMALE", &female) ||
        !ck2_define_number(defines, "MAX_CHILD_BIRTH_AGE", &maximum) ||
        !ck2_define_number(defines, "CHILD_BIRTH_TO_PREGNANCY_WAIT", &wait) ||
        male < 1 || male > 100 || female < 1 || female > 100 ||
        maximum < female || maximum > 110 || wait < 0 || wait > 120) return false;
    state->marriage_min_male = (unsigned)male;
    state->marriage_min_female = (unsigned)female;
    state->max_birth_age = (unsigned)maximum;
    state->post_birth_wait_months = (unsigned)wait;
    return true;
}
static bool adult(const struct family_gameplay *state, const struct ruler_profile *p)
{
    /* Custom religious/trait eligibility still requires CK2 script evaluation. */
    const unsigned required = p && p->female ? state->marriage_min_female : state->marriage_min_male;
    return p && p->age_known && required && p->age >= required;
}
enum family_status family_gameplay_marriage_type(struct family_gameplay *state,
       const struct ruler_profile *left, const struct ruler_profile *right, bool matrilineal)
{
    if (!state || !left || !right || !left->id || !right->id || left->id == right->id)
        return FAMILY_NO_PERSON;
    const struct ruler_profile *a = family_gameplay_find(state, left->id);
    const struct ruler_profile *b = family_gameplay_find(state, right->id);
    if (!a) a = left;
    if (!b) b = right;
    if (a->deceased || b->deceased || a->female == b->female) return FAMILY_NOT_ELIGIBLE;
    if (!adult(state, a) || !adult(state, b)) return FAMILY_TOO_YOUNG;
    if (related(a, b)) return FAMILY_RELATED;
    if (a->spouse || b->spouse) return FAMILY_ALREADY_MARRIED;
    /* Insertion may realloc state->people, invalidating a and b. */
    const unsigned a_id = a->id, b_id = b->id;
    struct ruler_profile a_snapshot = *a, b_snapshot = *b;
    if (!ensure_person(state, &a_snapshot) || !ensure_person(state, &b_snapshot)) return FAMILY_NO_MEMORY;
    find_mutable(state, a_id)->profile.spouse = b_id;
    find_mutable(state, b_id)->profile.spouse = a_id;
    find_mutable(state, a_id)->matrilineal_marriage = matrilineal;
    find_mutable(state, b_id)->matrilineal_marriage = matrilineal;
    return FAMILY_OK;
}
enum family_status family_gameplay_marriage(struct family_gameplay *state,
       const struct ruler_profile *left, const struct ruler_profile *right)
{
    return family_gameplay_marriage_type(state, left, right, false);
}
enum family_status family_gameplay_import_marriage(struct family_gameplay *state,
       const struct ruler_profile *left, const struct ruler_profile *right)
{
    if (!state || !left || !right || !left->id || !right->id || left->id == right->id)
        return FAMILY_NO_PERSON;
    const struct ruler_profile *a = family_gameplay_find(state, left->id);
    const struct ruler_profile *b = family_gameplay_find(state, right->id);
    if (!a) a = left;
    if (!b) b = right;
    if ((a->spouse != right->id && b->spouse != left->id) ||
        (a->spouse && a->spouse != right->id) ||
        (b->spouse && b->spouse != left->id)) return FAMILY_NOT_PARTNERS;
    struct ruler_profile ac = *a, bc = *b;
    if (!ensure_person(state, &ac) || !ensure_person(state, &bc)) return FAMILY_NO_MEMORY;
    find_mutable(state, ac.id)->profile.spouse = bc.id;
    find_mutable(state, bc.id)->profile.spouse = ac.id;
    return FAMILY_OK;
}

enum family_status family_gameplay_divorce(struct family_gameplay *state, unsigned a, unsigned b)
{
    struct family_runtime_person *first = find_mutable(state, a);
    struct family_runtime_person *second = find_mutable(state, b);
    if (!first || !second || first->profile.spouse != b || second->profile.spouse != a)
        return FAMILY_NOT_PARTNERS;
    first->profile.spouse = second->profile.spouse = 0;
    first->matrilineal_marriage = second->matrilineal_marriage = false;
    return FAMILY_OK;
}
enum family_status family_gameplay_conceive(struct family_gameplay *state,
       const struct ruler_profile *left, const struct ruler_profile *right, struct campaign_date date)
{
    if (!state || !left || !right || !valid_date(date)) return FAMILY_INVALID_DATE;
    const struct ruler_profile *a = family_gameplay_find(state, left->id);
    const struct ruler_profile *b = family_gameplay_find(state, right->id);
    if (!a) a = left;
    if (!b) b = right;
    const struct ruler_profile *mother = a->female ? a : b;
    const struct ruler_profile *father = a->female ? b : a;
    if (!mother->female || father->female || mother->deceased || father->deceased ||
        mother->spouse != father->id || father->spouse != mother->id ||
        !adult(state, mother) || !adult(state, father) ||
        !state->max_birth_age || mother->age > state->max_birth_age) return FAMILY_NOT_ELIGIBLE;
    struct family_runtime_person *existing = find_mutable(state, mother->id);
    if (existing && existing->has_due_date) return FAMILY_ALREADY_PREGNANT;
    if (existing && valid_date(existing->last_birth)) {
        const int months = (date.year - existing->last_birth.year) * 12 +
                           date.month - existing->last_birth.month;
        if (months < (int)state->post_birth_wait_months) return FAMILY_NOT_ELIGIBLE;
    }
    if (!ensure_person(state, mother)) return FAMILY_NO_MEMORY;
    struct family_runtime_person *record = find_mutable(state, mother->id);
    record->has_due_date = true;
    record->pregnancy_father = father->id;
    /* Opt-in experimental pregnancy, not CK2's recovered conception algorithm. */
    record->due = add_days(date, 280);
    return FAMILY_OK;
}
static unsigned deliver_child(struct family_gameplay *state,
                              struct family_runtime_person *mother, struct campaign_date date)
{
    if (!state || !mother || !state->next_id || state->next_id == UINT_MAX) return 0;
    const struct family_runtime_person *father = find_mutable(state, mother->pregnancy_father);
    const unsigned child_id = state->next_id++;
    struct ruler_profile child = {.id = child_id, .father = mother->pregnancy_father,
        .mother = mother->profile.id,
        .dynasty_id = mother->matrilineal_marriage ? mother->profile.dynasty_id :
                      father ? father->profile.dynasty_id : 0,
        .age = 0, .age_known = true, .female = (child_id & 1u) != 0};
    if (mother->matrilineal_marriage)
        snprintf(child.dynasty, sizeof child.dynasty, "%s", mother->profile.dynasty);
    if (father) {
        if (!mother->matrilineal_marriage)
            snprintf(child.dynasty, sizeof child.dynasty, "%s", father->profile.dynasty);
        snprintf(child.culture_key, sizeof child.culture_key, "%s", father->profile.culture_key);
        snprintf(child.religion_key, sizeof child.religion_key, "%s", father->profile.religion_key);
    }
    snprintf(child.name, sizeof child.name, "Child %u", child_id);
    /* Ensure space BEFORE clearing pregnancy so an allocation failure cannot
     * erase an expected birth. A later realloc invalidates mother. */
    const unsigned mother_id = mother->profile.id;
    if (!ensure_person(state, &child)) { --state->next_id; return 0; }
    struct family_runtime_person *mom = find_mutable(state, mother_id);
    mom->has_due_date = false;
    mom->last_birth = date;
    find_mutable(state, child_id)->born = date;
    ++state->births;
    return child_id;
}

enum family_status family_gameplay_birth(struct family_gameplay *state,
        const struct ruler_profile *left, const struct ruler_profile *right,
        struct campaign_date date, unsigned *child_id)
{
    if (child_id) *child_id = 0;
    /* The conception helper may grow the runtime array; save IDs before it
     * runs, because callers can pass pointers into that same array. */
    const unsigned mother_id = left && right ? (left->female ? left->id : right->id) : 0;
    enum family_status status = family_gameplay_conceive(state, left, right, date);
    if (status != FAMILY_OK) return status;
    struct family_runtime_person *record = find_mutable(state, mother_id);
    const unsigned child = deliver_child(state, record, date);
    if (!child) return FAMILY_NO_MEMORY;
    if (child_id) *child_id = child;
    return FAMILY_OK;
}

enum family_status family_gameplay_death(struct family_gameplay *state,
        const struct ruler_profile *person, struct campaign_date date)
{
    if (!state || !person || !person->id || !valid_date(date)) return FAMILY_INVALID_DATE;
    const struct ruler_profile *current = family_gameplay_find(state, person->id);
    if (!current) current = person;
    if (current->deceased) return FAMILY_DEAD;
    struct ruler_profile snapshot = *current;
    const unsigned person_id=snapshot.id;
    struct family_runtime_person *edited = ensure_person(state, &snapshot);
    if (!edited) return FAMILY_NO_MEMORY;
    const unsigned spouse_id = edited->profile.spouse;
    edited->profile.deceased = true;
    edited->death_date = date;
    edited->has_death_date = true;
    edited->has_due_date = false;
    edited->profile.spouse = 0;
    struct family_runtime_person *spouse = find_mutable(state, spouse_id);
    if (spouse && spouse->profile.spouse == person_id) {
        spouse->profile.spouse = 0;
        spouse->has_due_date = false; /* A simplified widowhood rule. */
    }
    return FAMILY_OK;
}

const struct campaign_date *family_gameplay_death_date(const struct family_gameplay *state,
                                                        unsigned id)
{
    if (!state) return NULL;
    for (size_t i = 0; i < state->count; ++i)
        if (state->people[i].profile.id == id && state->people[i].has_death_date)
            return &state->people[i].death_date;
    return NULL;
}

unsigned family_gameplay_day(struct family_gameplay *state, struct campaign_date date)
{
    if (!state || !valid_date(date)) return 0;
    for (size_t i = 0; i < state->count; ++i) {
        struct family_runtime_person *person = &state->people[i];
        if (valid_date(person->born) && compare_dates(date, person->born) >= 0) {
            unsigned age = (unsigned)(date.year - person->born.year);
            if (date.month < person->born.month ||
                (date.month == person->born.month && date.day < person->born.day)) --age;
            person->profile.age = age;
        }
        struct family_runtime_person *mother = person;
        if (!mother->has_due_date || compare_dates(date, mother->due) < 0) continue;
        return deliver_child(state, mother, date);
    }
    return 0;
}
bool family_gameplay_name_child(struct family_gameplay *state, unsigned id, const char *name)
{
    struct family_runtime_person *record = find_mutable(state, id);
    if (!record || !valid_date(record->born) || !name || !*name) return false;
    snprintf(record->profile.name, sizeof record->profile.name, "%s", name);
    return true;
}

size_t family_gameplay_children(const struct family_gameplay *state, unsigned parent,
                                unsigned *ids, size_t capacity)
{
    size_t count = 0;
    if (!state || !parent) return 0;
    for (size_t i = 0; i < state->count; ++i) {
        const struct ruler_profile *person = &state->people[i].profile;
        if (person->father != parent && person->mother != parent) continue;
        if (ids && count < capacity) ids[count] = person->id;
        ++count;
    }
    return count;
}
void family_gameplay_free(struct family_gameplay *state)
{
    if (!state) return;
    free(state->people);
    *state = (struct family_gameplay){0};
}
const char *family_status_description(enum family_status status)
{
    static const char *names[] = {
        "Family action succeeded.", "Character unavailable.", "One partner is already married.",
        "Marriage requires known adult ages (16+).", "Close relatives cannot marry.",
        "Not a recorded runtime marriage.", "Pregnancy already in progress.",
        "This pairing is not currently eligible.", "Unable to allocate family records.",
        "Invalid campaign date.", "Character is already deceased."
    };
    return status >= FAMILY_OK && status <= FAMILY_INVALID_DATE ? names[status] : "Unknown family result.";
}
