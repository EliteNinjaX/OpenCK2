#include "../src/game/family_gameplay.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct ruler_profile male(unsigned id)
{
    struct ruler_profile p = {.id = id, .age = 24, .age_known = true, .dynasty_id = 12};
    strcpy(p.name,"Father"); strcpy(p.dynasty,"Father House"); strcpy(p.culture_key,"english");
    return p;
}
static struct ruler_profile female(unsigned id)
{
    struct ruler_profile p = {.id = id, .female = true, .age = 22, .age_known = true, .dynasty_id = 23};
    strcpy(p.name,"Mother"); strcpy(p.dynasty,"Mother House");
    return p;
}
int main(void)
{
    struct family_gameplay s = {.marriage_min_male = 16, .marriage_min_female = 16,
                                .max_birth_age = 45, .post_birth_wait_months = 3, .next_id = 0xf0000000u};
    struct ruler_profile a = male(10), b = female(20);
    struct campaign_date d = {1066, 9, 15};
    assert(family_gameplay_marriage_type(&s, &a, &b, true) == FAMILY_OK);
    unsigned id = 0;
    /* Exercise pointers owned by the reallocating runtime state. */
    const struct ruler_profile *live_a = family_gameplay_find(&s, a.id);
    const struct ruler_profile *live_b = family_gameplay_find(&s, b.id);
    assert(live_a && live_b);
    assert(family_gameplay_birth(&s, live_a, live_b, d, &id) == FAMILY_OK);
    const struct ruler_profile *baby = family_gameplay_find(&s, id);
    assert(baby && baby->mother == 20 && baby->father == 10 && baby->dynasty_id == 23);
    assert(baby->age_known && baby->age == 0 && !baby->deceased);
    assert(family_gameplay_children(&s, 20, NULL, 0) == 1);
    assert(family_gameplay_birth(&s, &a, &b, d, NULL) == FAMILY_NOT_ELIGIBLE);
    assert(family_gameplay_death(&s, &b, d) == FAMILY_OK);
    assert(family_gameplay_find(&s, 20)->deceased);
    assert(family_gameplay_find(&s, 10)->spouse == 0);
    assert(family_gameplay_death_date(&s, 20)->year == 1066);
    assert(family_gameplay_death(&s, &b, d) == FAMILY_DEAD);
    assert(family_gameplay_conceive(&s, &a, &b, d) == FAMILY_NOT_ELIGIBLE);
    assert(family_gameplay_death(&s, baby, d) == FAMILY_OK);
    assert(family_gameplay_find(&s, id)->deceased);
    family_gameplay_free(&s);
    puts("family lifecycle: all assertions passed");
}
