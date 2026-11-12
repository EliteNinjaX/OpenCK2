#define _POSIX_C_SOURCE 200809L
#include "construction.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static void write_data(const char *path, const char *contents)
{
    FILE *file = fopen(path, "w");
    assert(file);
    assert(fputs(contents, file) >= 0);
    assert(!fclose(file));
}

int main(void)
{
    char root[] = "/tmp/openck2-construction-XXXXXX";
    char common[128], directory[128], path[128], types[128], type_file[128], modifiers[128], defines[128];
    assert(mkdtemp(root));
    snprintf(common, sizeof common, "%s/common", root);
    snprintf(directory, sizeof directory, "%s/common/buildings", root);
    snprintf(path, sizeof path, "%s/common/buildings/buildings.txt", root);
    snprintf(types, sizeof types, "%s/common/holding_types", root);
    snprintf(type_file, sizeof type_file, "%s/common/holding_types/00_holding_types.txt", root);
    snprintf(modifiers, sizeof modifiers, "%s/common/static_modifiers.txt", root);
    snprintf(defines, sizeof defines, "%s/common/defines.lua", root);
    assert(!mkdir(common, 0700) && !mkdir(directory, 0700) && !mkdir(types, 0700));
    write_data(defines, "NDefines = {\n HOLDING_SLOT_BUILD_COST_MOD = 0.2,\n"
                        " ENFORCE_ONE_OF_EACH_HOLDING = 1,\n}");
    write_data(modifiers, "castle = { build_cost_modifier = 50 build_time_modifier = 3 }\n"
                          "city = { build_cost_modifier = 50 build_time_modifier = 3 }\n"
                          "temple = { build_cost_modifier = 50 build_time_modifier = 3 }");
    write_data(type_file, "castle = { trigger = {} } city = { trigger = {} } temple = { trigger = {} }");
    write_data(path,
        "castle = {"
        " base = { gold_cost = 10 build_time = 3 trigger = { TECH_CASTLE = 0 } }"
        " upgrade = { gold_cost = 20 build_time = 2 upgrades_from = base prerequisites = { wall } }"
        " wall = { gold_cost = 5 build_time = 2 }"
        " advanced = { gold_cost = 10 build_time = 3 trigger = { TECH_CASTLE = 1 } }"
        " restricted = { gold_cost = 10 build_time = 3 potential = { always = no } }"
        "}"
        "city = { market = { gold_cost = 30 build_time = 2 } }");

    struct construction state = {0};
    assert(!construction_load(&state, root) && state.count == 6);
    const struct holding castle = {.id = "b_castle", .type = HOLDING_CASTLE};
    const struct holding city = {.id = "b_city", .type = HOLDING_CITY};
    const struct holding other = {.id = "b_other", .type = HOLDING_CASTLE};
    unsigned gold = 50, days = 0;
    construction_history(&state, "900.1.1 = { b_castle = wall }\n1000.1.1 = { b_castle = advanced }",
                         (struct campaign_date){936, 1, 1});
    assert(construction_status(&state, &castle, 2, true, gold, &days) == BUILD_COMPLETE);
    assert(construction_status(&state, &castle, 3, true, gold, &days) == BUILD_REQUIREMENTS);
    assert(!construction_start(&state, &castle, 0, false, &gold));
    assert(!construction_start(&state, &castle, 1, true, &gold));
    assert(!construction_start(&state, &castle, 4, true, &gold));
    assert(!construction_start(&state, &city, 0, true, &gold));
    assert(gold == 50);
    assert(construction_start(&state, &castle, 0, true, &gold) && gold == 40);
    assert(!construction_start(&state, &castle, 0, true, &gold));
    assert(construction_start(&state, &city, 5, true, &gold) && gold == 10);
    assert(construction_start(&state, &other, 0, true, &gold) && gold == 0);
    assert(construction_status(&state, &other, 2, true, gold, &days) == BUILD_BUSY);
    construction_day(&state);
    construction_day(&state);
    assert(construction_status(&state, &city, 5, true, gold, &days) == BUILD_COMPLETE);
    assert(construction_status(&state, &castle, 0, true, gold, &days) == BUILD_RUNNING && days == 1);
    construction_day(&state);
    assert(construction_status(&state, &castle, 1, true, gold, &days) == BUILD_UNAFFORDABLE);
    gold = 20;
    assert(construction_start(&state, &castle, 1, true, &gold) && gold == 0);
    construction_reset(&state);
    assert(construction_status(&state, &castle, 0, true, gold, &days) == BUILD_UNAFFORDABLE);
    struct county_holdings county = {.slots = {castle, city}, .count = 2, .capacity = 3};
    const struct holding temple = {.id = "b_temple", .type = HOLDING_TEMPLE};
    unsigned quote;
    gold = 60;
    assert(!construction_settlement_quote(&state, &county, 32, HOLDING_CITY, &quote, &days));
    assert(construction_settlement_quote(&state, &county, 32, HOLDING_TEMPLE, &quote, &days));
    assert(quote == 60 && days == 3);
    assert(construction_settlement_start(&state, &county, 32, &temple, &gold) && !gold);
    assert(!construction_settlement_quote(&state, &county, 32, HOLDING_TEMPLE, &quote, &days));
    assert(construction_settlement_pending(&state, 32, NULL) == 3);
    construction_day(&state);
    construction_day(&state);
    construction_settlement_apply(&state, 32, &county);
    assert(county.count == 2 && !construction_owns_settlement(&state, temple.id));
    construction_day(&state);
    construction_settlement_apply(&state, 33, &county);
    assert(county.count == 2);
    construction_settlement_apply(&state, 32, &county);
    construction_settlement_apply(&state, 32, &county);
    assert(county.count == 3 && construction_owns_settlement(&state, temple.id));
    gold = 1000;
    assert(!construction_settlement_start(&state, &county, 32, &other, &gold) && gold == 1000);
    construction_reset(&state);
    assert(!construction_owns_settlement(&state, temple.id));
    construction_free(&state);
    assert(!unlink(path) && !unlink(type_file) && !unlink(modifiers) && !unlink(defines));
    assert(!rmdir(directory) && !rmdir(types) && !rmdir(common) && !rmdir(root));
    return 0;
}
