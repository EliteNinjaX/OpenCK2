#include "../src/game/government_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void synthetic(void)
{
    const char *data = "feudal_governments = { \n"
      " feudal_government = { preferred_holdings = { CASTLE } allowed_holdings = { CASTLE FORT } "
      "potential = { OR = { a = yes NOT = { b = no } } } "
      " color = { 91 133 207 } aggression = 2.0 } }";
    struct government_catalog catalog = {0};
    assert(government_catalog_parse(&catalog, data));
    assert(catalog.count == 1);
    const struct government_definition *g = government_find(&catalog, "feudal_government");
    assert(g && g->preferred_holdings == GOVERNMENT_CASTLE);
    assert(g->allowed_holdings == (GOVERNMENT_CASTLE | GOVERNMENT_FORT));
    assert(g->color_rgb == 0x5b85cfu && g->color_defined);
    assert(g->has_potential && strstr(g->potential_script, "NOT") != NULL);
    assert(g->aggression == 2.0);
    assert(government_holding_candidates(&catalog, GOVERNMENT_CASTLE, NULL, 0) == 1);
    assert(government_holding_candidates(&catalog, GOVERNMENT_NOMAD, NULL, 0) == 0);
    assert(government_catalog_parse(&catalog,
      "feudal_governments={feudal_government={preferred_holdings={CITY} frame_suffix=\"_new\"}}"));
    assert(catalog.count == 1);
    assert(government_find(&catalog, "feudal_government")->preferred_holdings == GOVERNMENT_CITY);
    assert(!government_find(&catalog, "feudal_government")->has_potential);
    government_catalog_destroy(&catalog);
    assert(!catalog.count && !catalog.items);
}

static void original_file(const char *file, struct government_catalog *catalog)
{
    FILE *f = fopen(file, "rb"); assert(f);
    assert(fseek(f, 0, SEEK_END) == 0);
    long length = ftell(f); assert(length > 0 && length < 1000000);
    rewind(f);
    char *data = malloc((size_t)length + 1); assert(data);
    assert(fread(data, 1, (size_t)length, f) == (size_t)length);
    data[length] = 0;
    fclose(f);
    assert(government_catalog_parse(catalog, data));
    free(data);
}

static void imperial_assignment(const struct government_catalog *catalog)
{
    const struct government_definition *roman = government_find(catalog, "roman_imperial_government");
    assert(roman && roman->has_potential);
    assert(roman->color_defined && roman->color_rgb == 0x8f0c7du);
    char titles[8][80] = {{0}};
    assert(government_potential_titles(roman, titles, 8) == 2);
    assert(!strcmp(titles[0], "e_byzantium"));
    assert(!strcmp(titles[1], "e_roman_empire"));
    struct government_context ctx = {.primary_title="e_byzantium"};
    bool inferred=false;
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_TRUE);
    assert(government_resolve_special(catalog,NULL,&ctx,&inferred) == roman && inferred);
    ctx.primary_title="e_roman_empire";
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_TRUE);
    ctx.primary_title="e_france";
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_FALSE);
    ctx.primary_title="e_byzantium";
    ctx.is_patrician=true;
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_FALSE);
    ctx.is_patrician=false;
    ctx.game_started=true;
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_UNRESOLVED);
    ctx.succession_law="succ_byzantine_elective";
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_TRUE);
    ctx.succession_law="succ_primogeniture";
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_FALSE);
    ctx.game_started=false;
    ctx.save_game=true;
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_FALSE);
    inferred=true;
    assert(government_resolve_special(catalog,"feudal_government",&ctx,&inferred) ==
           government_find(catalog,"feudal_government") && !inferred);
    ctx.primary_title="e_byzantium";
    ctx.save_game=false;
    ctx.succession_law=NULL;
    assert(government_evaluate_potential(roman,&ctx) == GOVERNMENT_TRUE);
    puts("government core: original Roman imperial eligibility verified");
}


static void synthetic_imperial(void)
{
    const char *script=
      "feudal_governments={ roman_imperial_government={ color={143 12 125} "
      "preferred_holdings={ CASTLE CITY } frame_suffix=\"_romanimperial\" "
      "potential={ OR={ AND={ is_save_game=no has_game_started=no "
      "primary_title={OR={title=e_byzantium title=e_roman_empire}} } "
      "AND={primary_title={OR={title=e_byzantium title=e_roman_empire} "
      "has_law=succ_byzantine_elective}}} is_patrician=no } } }";
    struct government_catalog catalog={0};
    assert(government_catalog_parse(&catalog,script));
    imperial_assignment(&catalog);
    assert(!government_resolve_special(&catalog,"feudal_government",
           &(struct government_context){.primary_title="e_byzantium"},NULL));
    government_catalog_destroy(&catalog);
    /* Never accept an unknown CK2 trigger as fulfilled. */
    const char *uncertain="feudal_governments={roman_imperial_government={"
        "potential={primary_title={title=e_byzantium} has_character_flag=chosen}}}";
    assert(government_catalog_parse(&catalog,uncertain));
    const struct government_context ctx={.primary_title="e_byzantium"};
    assert(government_evaluate_potential(government_find(&catalog,"roman_imperial_government"),&ctx)
           == GOVERNMENT_UNRESOLVED);
    government_catalog_destroy(&catalog);
}

int main(int argc, char **argv)
{
    synthetic();
    synthetic_imperial();
    if (argc > 1) {
        struct government_catalog catalog = {0};
        for (int i = 1; i < argc; ++i) original_file(argv[i], &catalog);
        assert(government_find(&catalog, "nomadic_government"));
        assert(government_find(&catalog, "feudal_government"));
        assert(government_find(&catalog, "tribal_government"));
        imperial_assignment(&catalog);
        const struct government_definition *nomadic = government_find(&catalog, "nomadic_government");
        assert(nomadic->preferred_holdings == GOVERNMENT_NOMAD);
        assert(nomadic->color_defined && nomadic->has_potential);
        printf("government core: parsed %zu original definitions\n", catalog.count);
        government_catalog_destroy(&catalog);
    }
    puts("government core: all assertions passed");
    return 0;
}
