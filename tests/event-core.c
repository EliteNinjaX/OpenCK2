#include "event_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char sample[] =
    "namespace = WoL\n"
    "character_event = { id = WoL.1 desc = EVTDESC_WOL1 trigger = { has_trait = genius } "
    "option = { name = EVTOPTA_WOL1 wealth = 5 prestige = 2.5 piety = -1 } "
    "option = { name = EVTOPTB_WOL1 trigger = { age = 16 } "
    "random_list = { 50 = { add_trait = brave } } wealth = 9 } }\n"
    "province_event = { id = 900 title = MY_EVENT desc = { trigger = { religion = catholic } "
    "text = SOME_DESC } immediate = { piety = 2 } "
    "option = { name = OK culture = english religion = catholic } }\n";

int main(void)
{
    struct ck2_event e={0};
    assert(ck2_event_valid_id("WoL.1"));
    assert(!ck2_event_valid_id("../bad"));
    assert(!ck2_event_valid_id("900 | rm x"));
    assert(ck2_event_find_in_text(sample,"bad","events/test.txt",&e)==0);
    assert(ck2_event_find_in_text(sample,"WoL.1","events/test.txt",&e)==1);
    assert(e.kind==CK2_EVENT_CHARACTER && e.has_trigger);
    assert(!strcmp(e.id,"WoL.1") && !strcmp(e.description,"EVTDESC_WOL1"));
    assert(e.option_count==2 && e.options[0].effect_count==3);
    struct ck2_event_deltas d=ck2_event_numeric_deltas(&e.options[0]);
    assert(fabs(d.wealth-5.)<0.0001 && fabs(d.prestige-2.5)<0.0001 && fabs(d.piety+1)<0.0001);
    assert(e.options[1].conditional && e.options[1].ignored_effects==1);
    assert(ck2_event_find_in_text(sample,"900","events/test.txt",&e)==1);
    assert(e.kind==CK2_EVENT_PROVINCE && e.has_immediate && e.conditional_description);
    assert(!strcmp(e.description,"SOME_DESC") && e.option_count==1);
    assert(e.options[0].effects[0].kind==CK2_EVENT_CULTURE && e.options[0].effects[1].kind==CK2_EVENT_RELIGION);
    assert(ck2_event_find_in_text(sample,"../foo","events/test.txt",&e)==-1);
    assert(ck2_event_find_in_text("character_event = { id = 100 option = { name = test wealth = not_a_number some_script = { x = y } } }",
                                  "100","events/demo.txt",&e)==1);
    assert(!e.options[0].effect_count && e.options[0].ignored_effects==2);
    puts("event core: all assertions passed");
    return 0;
}
