#include "economy.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const char *script="laws = { feudal_tax_0 = { group = feudal_tax castle_vassal_tax_modifier = 0.0 } "
       "feudal_tax_2 = { group = feudal_tax castle_vassal_tax_modifier = 0.2 } "
       "city_tax_1 = { group = city_tax city_vassal_tax_modifier = 0.25 } "
       "tax_levy_law_feudal_3 = { group = tax_levy_law_feudal castle_vassal_tax_modifier = 0.25 } }";
    struct ck2_law_catalog cat={0};struct ck2_law_state selected={0};
    assert(ck2_law_parse(&cat,script));
    struct construction buildings={0};
    buildings.holding_base_tax_known[HOLDING_CASTLE]=true;
    buildings.holding_base_tax[HOLDING_CASTLE]=4;
    buildings.holding_base_tax_known[HOLDING_CITY]=true;
    buildings.holding_base_tax[HOLDING_CITY]=12;
    struct economy_snapshot snapshot={0};
    snapshot.vassal_holdings=calloc(3,sizeof(*snapshot.vassal_holdings));
    assert(snapshot.vassal_holdings);
    snapshot.vassal_count=snapshot.vassal_capacity=3;
    snapshot.vassal_holdings[0].holding=(struct holding){.id="b_keep",.type=HOLDING_CASTLE};
    snapshot.vassal_holdings[0].holder_id=2;
    snapshot.vassal_holdings[1].holding=(struct holding){.id="b_burg",.type=HOLDING_CITY};
    snapshot.vassal_holdings[1].holder_id=3;
    snapshot.vassal_holdings[2].holding=(struct holding){.id="b_unknown",.type=HOLDING_TEMPLE};
    snapshot.vassal_holdings[2].holder_id=4;
    economy_snapshot_apply_laws(&snapshot,&buildings,&cat,&selected);
    assert(snapshot.annual_law_vassal_tax==0 && snapshot.vassal_tax_skipped==3);
    assert(ck2_law_select(&selected,&cat,"feudal_tax_0"));
    assert(ck2_law_select(&selected,&cat,"city_tax_1"));
    economy_snapshot_apply_laws(&snapshot,&buildings,&cat,&selected);
    assert(fabs(snapshot.annual_law_vassal_tax-3.0)<.00001);
    assert(snapshot.vassal_tax_skipped==1);
    assert(ck2_law_select(&selected,&cat,"feudal_tax_2"));
    economy_snapshot_apply_laws(&snapshot,&buildings,&cat,&selected);
    assert(fabs(snapshot.annual_law_vassal_tax-3.8)<.00001);
    assert(ck2_law_select(&selected,&cat,"tax_levy_law_feudal_3"));
    economy_snapshot_apply_laws(&snapshot,&buildings,&cat,&selected);
    assert(fabs(snapshot.annual_law_vassal_tax-4.0)<.00001);
    for (int i=0;i<5;++i) {
        economy_snapshot_apply_laws(&snapshot,&buildings,&cat,&selected);
        assert(fabs(snapshot.annual_law_vassal_tax-4.0)<.00001);
    }
    ck2_law_state_free(&selected);
    ck2_law_catalog_free(&cat);
    economy_snapshot_free(&snapshot);
    puts("Law-controlled vassal tax: no double-counting and missing data handled");
}
