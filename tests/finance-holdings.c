#include "holdings.h"
#include "construction.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *history =
        "culture = greek\nreligion = orthodox\nmax_settlements = 4\n"
        "b_castle = castle\nb_castle = ca_wall_1\n"
        "b_city = city\nb_city = ct_port_1\n"
        "950.1.1 = { culture = armenian b_castle = ca_wall_2 }\n"
        "1050.1.1 = { religion = catholic b_castle = ca_wall_3 }\n"
        "1200.1.1 = { culture = turkish b_castle = ca_wall_4 }\n";
    struct county_holdings county;
    holdings_read(&county, history, (struct campaign_date){1066,9,15});
    assert(county.count == 2 && county.capacity == 4);
    assert(!strcmp(county.culture_key,"armenian") && !strcmp(county.religion_key,"catholic"));
    const struct holding *castle = &county.slots[0];
    assert(castle->type == HOLDING_CASTLE && castle->building_count == 3);
    const struct building definitions[] = {
        {.id="ca_wall_1", .type=HOLDING_CASTLE, .annual_tax=0.2},
        {.id="ca_wall_2", .type=HOLDING_CASTLE, .annual_tax=0.2, .upgrades_from="ca_wall_1"},
        {.id="ca_wall_3", .type=HOLDING_CASTLE, .annual_tax=0.5, .upgrades_from="ca_wall_2"},
        {.id="ct_port_1", .type=HOLDING_CITY, .annual_tax=2.0}
    };
    struct construction state = {.catalog=(struct building *)definitions,
        .count=sizeof definitions/sizeof definitions[0]};
    state.holding_base_tax[HOLDING_CASTLE] = 4;
    state.holding_base_tax_known[HOLDING_CASTLE] = true;
    state.holding_base_tax[HOLDING_CITY] = 12;
    state.holding_base_tax_known[HOLDING_CITY] = true;
    double tax = -1;
    assert(construction_holding_annual_tax(&state, castle, &tax));
    assert(fabs(tax - 4.5) < 0.0001); /* previous wall tiers replaced */
    assert(construction_holding_annual_tax(&state, &county.slots[1], &tax));
    assert(fabs(tax - 14) < 0.0001);
    holdings_read(&county, history, (struct campaign_date){900,1,1});
    assert(!strcmp(county.culture_key,"greek") && !strcmp(county.religion_key,"orthodox"));
    assert(county.slots[0].building_count == 1);
    puts("finance holdings: all assertions passed");
}
