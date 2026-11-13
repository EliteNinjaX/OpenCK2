/* SDL-free integration: CK2 government potential + dated title succession law.
 * The fixture is synthetic; users can separately test installed definition
 * files via `make check-governments-core CK2_GOVERNMENT_TEST_FILES=...`. */
#include "../src/game/government_core.h"
#include "../src/game/law_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct government_catalog governments={0};
    struct ck2_law_catalog catalog={0};
    struct ck2_law_state active={0};
    const char *government_source=
        "feudal_governments = { roman_imperial_government = { "
        "color = { 143 12 125 } "
        "potential = { OR = { "
        "AND = { is_save_game = no has_game_started = no "
        "primary_title = { OR = { title = e_byzantium title = e_roman_empire } } } "
        "AND = { primary_title = { OR = { "
        "title = e_byzantium title = e_roman_empire } "
        "has_law = succ_byzantine_elective } } } is_patrician = no } } }";
    const char *laws="succession_laws={ "
        "succ_primogeniture={ succession=primogeniture } "
        "succ_byzantine_elective={ succession=byzantine_elective } }";
    const char *title_history=
        "law=succ_primogeniture "
        "1000.1.1={law=succ_primogeniture} "
        "1100.1.1={law=succ_byzantine_elective} "
        "1160.1.1={law=succ_primogeniture}";
    assert(government_catalog_parse(&governments,government_source));
    assert(ck2_law_parse(&catalog,laws));
    const struct government_definition *roman=government_find(&governments,"roman_imperial_government");
    assert(roman);
    struct government_context ctx={.primary_title="e_byzantium",.game_started=true};
    assert(ck2_law_title_history(&active,&catalog,title_history,10660101));
    const struct ck2_law *law=ck2_law_selected(&active,&catalog,"succession");
    assert(law && !strcmp(law->key,"succ_primogeniture"));
    ctx.succession_law=law->key;
    assert(government_evaluate_potential(roman,&ctx)==GOVERNMENT_FALSE);
    ctx.game_started=false;
    assert(government_evaluate_potential(roman,&ctx)==GOVERNMENT_TRUE);
    ctx.game_started=true;
    ck2_law_state_free(&active);
    assert(ck2_law_title_history(&active,&catalog,title_history,11010101));
    law=ck2_law_selected(&active,&catalog,"succession");
    assert(law && !strcmp(law->key,"succ_byzantine_elective"));
    ctx.succession_law=law->key;
    assert(government_evaluate_potential(roman,&ctx)==GOVERNMENT_TRUE);
    ctx.primary_title="e_france";
    assert(government_evaluate_potential(roman,&ctx)==GOVERNMENT_FALSE);
    ctx.primary_title="e_roman_empire";
    assert(government_evaluate_potential(roman,&ctx)==GOVERNMENT_TRUE);
    ck2_law_state_free(&active);
    assert(ck2_law_title_history(&active,&catalog,title_history,12000101));
    law=ck2_law_selected(&active,&catalog,"succession");
    assert(law && !strcmp(law->key,"succ_primogeniture"));
    ctx.succession_law=law->key;
    assert(government_evaluate_potential(roman,&ctx)==GOVERNMENT_FALSE);
    ck2_law_state_free(&active);
    ck2_law_catalog_free(&catalog);
    government_catalog_destroy(&governments);
    puts("Byzantine government: dated title-law/imperial government integration passed");
    return 0;
}
