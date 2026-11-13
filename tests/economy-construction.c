/* Independent simulation regression: a founded holding must start to earn
 * only AFTER it is built, and it must not be counted twice on later months.
 * Using the actual construction/economy modules with SDL stub headers; only
 * the asset-loading sections are discarded by --gc-sections. */
#include "economy.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool near(double a, double b) { return fabs(a - b) < 0.00001; }

int main(void)
{
    const char market_source[] = "gold_cost = 3 build_time = 2 }";
    const char upgrade_source[] =
        "gold_cost = 5 build_time = 3 upgrades_from = ct_market_1 }";
    struct building catalog[] = {
        {.id="ct_market_1", .type=HOLDING_CITY, .annual_tax=2.0,
         .levy={.troops={[LEVY_LIGHT_INFANTRY]=10}},
         .gold=3, .days=2, .supported=true, .requirements=market_source},
        {.id="ct_market_2", .type=HOLDING_CITY, .annual_tax=5.0,
         .levy={.troops={[LEVY_LIGHT_INFANTRY]=40},.levy_size=.1},
         .gold=5, .days=3, .supported=true, .upgrades_from="ct_market_1",
         .requirements=upgrade_source}
    };
    struct construction construction = {.catalog=catalog, .count=2, .settlement_rules_ready=true};
    construction.settlement_type_allowed[HOLDING_CITY] = true;
    construction.settlement_gold[HOLDING_CITY] = 12;
    construction.settlement_days[HOLDING_CITY] = 2;
    construction.holding_base_tax_known[HOLDING_CASTLE] = true;
    construction.holding_base_tax[HOLDING_CASTLE] = 4.0;
    construction.holding_base_tax_known[HOLDING_CITY] = true;
    construction.holding_base_tax[HOLDING_CITY] = 12.0;
    construction.holding_base_levy[HOLDING_CITY].troops[LEVY_LIGHT_INFANTRY] = 75;

    const struct holding castle = {.id="b_castle", .type=HOLDING_CASTLE};
    const struct holding city = {.id="b_city", .type=HOLDING_CITY};
    struct levy_potential troops;
    assert(construction_holding_levy(&construction, &city, &troops));
    assert(troops.known && near(troops.total, 75));
    struct county_holdings county = {.slots={castle}, .count=1, .capacity=3};
    struct economy_snapshot economy = {0};
    economy.direct_holdings = malloc(sizeof castle);
    assert(economy.direct_holdings);
    economy.direct_holdings[0] = castle;
    economy.direct_count = economy.direct_capacity = 1;
    assert(economy_snapshot_track_county(&economy, 42));
    assert(economy_snapshot_track_county(&economy, 42));
    assert(economy.direct_county_count == 1);
    assert(!economy_snapshot_track_county(&economy, 0));
    economy_snapshot_refresh(&economy, &construction);
    assert(near(economy.annual_direct_tax, 4.0) && economy.direct_count == 1);
    unsigned treasury = 20;
    assert(construction_settlement_start(&construction, &county, 42, &city, &treasury));
    assert(treasury == 8 && construction_settlement_count(&construction) == 1);
    bool done = true;
    const struct holding *seen = NULL;
    unsigned province = 0;
    assert(construction_settlement_info(&construction, 0, &province, &seen, &done));
    assert(!done && province == 42 && seen && !strcmp(seen->id, "b_city"));
    assert(!construction_settlement_info(&construction, 1, NULL, NULL, NULL));
    assert(!construction_settlement_info(NULL, 0, NULL, NULL, NULL));
    assert(!construction_day(&construction));
    economy_snapshot_refresh(&economy, &construction);
    assert(near(economy.annual_direct_tax, 4.0) && economy.direct_count == 1);
    assert(construction_day(&construction));
    assert(construction_settlement_info(&construction, 0, NULL, NULL, &done) && done);
    assert(!construction_day(&construction));
    economy_snapshot_refresh(&economy, &construction);
    assert(economy.direct_count == 2 && near(economy.annual_direct_tax, 16.0));
    for (int i = 0; i < 60; ++i) {
        construction_day(&construction);
        economy_snapshot_refresh(&economy, &construction);
    }
    assert(economy.direct_count == 2 && near(economy.annual_direct_tax, 16.0));
    /* A new building affects the holding tax only after its actual duration.
     * Upgrading it replaces the previous tier, rather than double-counting. */
    const struct holding new_city = economy.direct_holdings[1];
    assert(!strcmp(new_city.id, "b_city"));
    unsigned build_days = 0;
    assert(construction_status(&construction, &new_city, 2, true, treasury,
                               &build_days) == BUILD_REQUIREMENTS);
    assert(!construction_start(&construction, &new_city, 2, true, &treasury));
    assert(!construction_start(NULL, &new_city, 1, true, &treasury));
    assert(construction_status(&construction, &new_city, 1, true, treasury,
                               &build_days) == BUILD_REQUIREMENTS);
    assert(construction_start(&construction, &new_city, 0, true, &treasury));
    assert(treasury == 5);
    assert(!construction_start(&construction, &new_city, 0, true, &treasury));
    assert(!construction_start(&construction, &new_city, 1, true, &treasury));
    economy_snapshot_refresh(&economy, &construction);
    assert(near(economy.annual_direct_tax, 16.0));
    assert(!construction_day(&construction));
    economy_snapshot_refresh(&economy, &construction);
    assert(near(economy.annual_direct_tax, 16.0));
    assert(construction_day(&construction));
    economy_snapshot_refresh(&economy, &construction);
    assert(near(economy.annual_direct_tax, 18.0));
    assert(construction_holding_levy(&construction, &new_city, &troops));
    assert(near(troops.total, 85));
    assert(construction_start(&construction, &new_city, 1, true, &treasury));
    assert(treasury == 0);
    for (int i = 0; i < 2; ++i) {
        construction_day(&construction);
        economy_snapshot_refresh(&economy, &construction);
        assert(near(economy.annual_direct_tax, 18.0));
    }
    assert(construction_day(&construction));
    economy_snapshot_refresh(&economy, &construction);
    assert(near(economy.annual_direct_tax, 21.0));
    assert(construction_holding_levy(&construction, &new_city, &troops));
    /* Upgrade replaces the earlier building, including troop counts. */
    assert(near(troops.total, 126.5));
    assert(economy.direct_count == 2);
    /* A different county's holding is not mysteriously added to our demesne. */
    const struct holding foreign = {.id="b_foreign_city", .type=HOLDING_CITY};
    unsigned foreign_wallet = 12;
    assert(construction_settlement_start(&construction, &county, 43, &foreign,
                                         &foreign_wallet));
    assert(foreign_wallet == 0);
    assert(!construction_day(&construction));
    assert(construction_day(&construction));
    economy_snapshot_refresh(&economy, &construction);
    assert(economy.direct_count == 2 && near(economy.annual_direct_tax, 21.0));
    economy_snapshot_free(&economy);
    construction_reset(&construction);
    assert(economy.direct_count == 0 && economy.direct_counties == NULL);
    puts("economy construction: all assertions passed");
    return 0;
}
