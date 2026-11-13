#include "family_gameplay.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct family_gameplay state = {.next_id=0xf0000000u};
    assert(family_gameplay_rules(&state,
        "AGE_OF_MARRIAGE_MALE = 16,\nAGE_OF_MARRIAGE_FEMALE = 16,\n"
        "MAX_CHILD_BIRTH_AGE = 45,\nCHILD_BIRTH_TO_PREGNANCY_WAIT = 3,\n"));
    struct ruler_profile husband = {.id=42,.age=30,.age_known=true,.dynasty_id=101};
    struct ruler_profile wife = {.id=43,.age=25,.age_known=true,.female=true,.dynasty_id=200};
    struct ruler_profile father = {.id=44,.age=60,.age_known=true,.father=42};
    assert(family_gameplay_marriage(&state, &husband, &wife) == FAMILY_OK);
    assert(family_gameplay_marriage(&state, &husband, &wife) == FAMILY_ALREADY_MARRIED);
    assert(family_gameplay_find(&state, 42)->spouse == 43);
    assert(family_gameplay_find(&state, 43)->spouse == 42);
    assert(family_gameplay_conceive(&state,&husband,&wife,(struct campaign_date){1066,9,15}) == FAMILY_OK);
    assert(family_gameplay_conceive(&state,&husband,&wife,(struct campaign_date){1066,9,15}) == FAMILY_ALREADY_PREGNANT);
    assert(family_gameplay_day(&state,(struct campaign_date){1067,1,1})==0);
    unsigned born=family_gameplay_day(&state,(struct campaign_date){1067,8,1});
    assert(born==0xf0000000u);
    const struct ruler_profile *child=family_gameplay_find(&state,born);
    assert(child && child->father == 42 && child->mother == 43 && child->dynasty_id == 101);
    unsigned children[3];
    assert(family_gameplay_children(&state, 42, children, 3)==1 && children[0]==born);
    assert(family_gameplay_day(&state,(struct campaign_date){1067,8,2})==0);
    assert(family_gameplay_divorce(&state,42,43)==FAMILY_OK);
    assert(family_gameplay_find(&state,42)->spouse == 0);
    assert(family_gameplay_conceive(&state,&husband,&wife,(struct campaign_date){1068,1,1})==FAMILY_NOT_ELIGIBLE);
    assert(family_gameplay_marriage(&state,&husband,&father)==FAMILY_NOT_ELIGIBLE);
    struct ruler_profile mother = {.id=45,.age=19,.age_known=true,.female=true,.father=42};
    assert(family_gameplay_marriage(&state,&husband,&mother)==FAMILY_RELATED);
    assert(family_gameplay_marriage_type(&state, &husband, &wife, true) == FAMILY_OK);
    assert(family_gameplay_conceive(&state, &husband, &wife,
           (struct campaign_date){1068,6,1}) == FAMILY_OK);
    unsigned daughter = family_gameplay_day(&state, (struct campaign_date){1069,4,1});
    assert(daughter != 0);
    const struct ruler_profile *matri_child = family_gameplay_find(&state, daughter);
    assert(matri_child && matri_child->dynasty_id == wife.dynasty_id);
    assert(family_gameplay_name_child(&state, daughter, "Anna"));
    assert(!strcmp(family_gameplay_find(&state,daughter)->name,"Anna"));
    family_gameplay_free(&state);
    puts("family gameplay: all assertions passed");
    return 0;
}
