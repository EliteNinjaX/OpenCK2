#include "government_mechanics_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_all(const char *path)
{
    FILE *f=fopen(path,"rb");if (!f) return NULL;
    assert(fseek(f,0,SEEK_END)==0);
    long n=ftell(f);assert(n>=0 && n<8*1024*1024);
    rewind(f);
    char *b=calloc((size_t)n+1,1);assert(b);
    assert(fread(b,1,(size_t)n,f)==(size_t)n);assert(!fclose(f));return b;
}
int main(int argc, char **argv)
{
    const char *defines= "NDefines = {\n"
        "NCharacter = { MERCHANT_REPUBLIC_MAX_PATRICIANS = 5, },\n"
        "NEconomy = { PATRICIAN_CAMPAIGN_FUND_FACTOR = 5, PATRICIAN_PRESTIGE_RESPECT_FACTOR = 2, PATRICIAN_AGE_RESPECT_FACTOR = 1, },\n"
        "NNomad = { POPULATION_INCREASE_MULTIPLIER = 0.01, START_POPULATION = 0.40, MAX_MANPOWER_POPULATION_MULTIPLIER = 0.20, POPULATION_TAX_MULTIPLIER = 0.0005, MAX_POPULATION_EMPTY_HOLDING_MULTIPLIER = 1000, MAX_CLANS = 9, },\n} ";
    /* Real CK2 defines are one assignment per line; test against them when
     * provided, and use a normal multiline fixture independently. */
    (void)defines;
    const char *fixture="NDefines = {\nNCharacter = {\n"
        "MERCHANT_REPUBLIC_MAX_PATRICIANS = 5,\n},\nNEconomy = {\n"
        "PATRICIAN_CAMPAIGN_FUND_FACTOR = 5,\nPATRICIAN_PRESTIGE_RESPECT_FACTOR = 2,\n"
        "PATRICIAN_AGE_RESPECT_FACTOR = 1,\n},\nNNomad = {\n"
        "POPULATION_INCREASE_MULTIPLIER = 0.01,\nSTART_POPULATION = 0.4,\n"
        "MAX_MANPOWER_POPULATION_MULTIPLIER = 0.20,\nPOPULATION_TAX_MULTIPLIER = 0.0005,\n"
        "MAX_POPULATION_EMPTY_HOLDING_MULTIPLIER = 1000,\nMAX_CLANS = 9,\n},\n}";
    struct ck2_government_rules r={0};
    assert(ck2_government_rules_parse(&r,fixture));
    assert(r.max_clans==9 && r.max_patricians==5 && r.nomad_loaded && r.merchant_loaded);
    if (argc>1) { char *actual=read_all(argv[1]);assert(actual);
        assert(ck2_government_rules_parse(&r,actual));free(actual);
        assert(r.max_clans==9 && r.max_patricians==5);
        assert(fabs(r.respect_fund-5)<1e-6 && fabs(r.nomad_start-0.4)<1e-6);
    }
    struct ck2_government_mechanics s={0};
    assert(ck2_nomad_add(&s,&r,10,6,6000)==CK2_GOV_OK);
    assert(s.clans[0].population==2400 && s.clans[0].manpower==480);
    assert(ck2_nomad_add(&s,&r,10,6,6000)==CK2_GOV_DUPLICATE);
    assert(ck2_nomad_split(&s,&r,10,11,2,1000)==CK2_GOV_OK);
    assert(s.clan_count==2 && s.clans[0].provinces==4 && s.clans[1].provinces==2);
    const double before=s.clans[0].population;
    ck2_nomad_month(&s,&r);
    assert(s.clans[0].population>before);
    assert(ck2_nomad_monthly_tax(&s,&r,10)>0);
    assert(ck2_patrician_add(&s,&r,40)==CK2_GOV_OK);
    assert(ck2_patrician_add(&s,&r,41)==CK2_GOV_OK);
    assert(ck2_patrician_add(&s,&r,40)==CK2_GOV_DUPLICATE);
    assert(ck2_patrician_fund(&s,40,200)==CK2_GOV_OK);
    double score;
    assert(ck2_patrician_respect(&r,60,100,200,&score));
    assert(score==60*60+200+1000);
    assert(!ck2_patrician_respect(&r,60,NAN,200,&score));
    assert(ck2_patrician_respect(&r,10,-200,0,&score) && score==-300);
    const unsigned ages[]={60,45};const double prestige[]={100,200};const bool known[]={true,true};
    assert(ck2_patrician_respect_leader(&s,&r,ages,prestige,known)==40);
    struct ck2_trade_catalog catalog={0};
    const char *posts="trade_post = { tr_port_1 = { gold_cost = 100 build_time = 182 tradevalue = 10 potential = { port = yes } } "
        "tr_port_2 = { gold_cost = 150 build_time = 274 tradevalue = 15 upgrades_from = tr_port_1 } tr_enclave_1={ gold_cost=50 build_time=30 tax_income=2 } }";
    assert(ck2_trade_catalog_parse(&catalog,posts));
    assert(catalog.count==3);
    double cost=0;
    assert(ck2_trade_build(&s,&catalog,40,101,"tr_port_1",false,&cost)==CK2_GOV_UNSUPPORTED);
    assert(ck2_trade_build(&s,&catalog,40,101,"tr_port_2",true,&cost)==CK2_GOV_LIMIT);
    assert(ck2_trade_build(&s,&catalog,40,101,"tr_port_1",true,&cost)==CK2_GOV_OK);
    assert(cost==100);
    assert(s.post_count==1 && s.posts[0].days_left==182);
    for (unsigned i=0;i<182;++i) ck2_trade_day(&s);
    assert(!s.posts[0].current_build[0] && s.posts[0].completed_count==1);
    assert(ck2_trade_value(s.posts,&catalog)==10);
    assert(ck2_trade_build(&s,&catalog,40,101,"tr_port_2",true,&cost)==CK2_GOV_OK);
    for (unsigned i=0;i<274;++i) ck2_trade_day(&s);
    assert(ck2_trade_value(s.posts,&catalog)==15);
    assert(ck2_trade_build(&s,&catalog,40,101,"tr_enclave_1",true,&cost)==CK2_GOV_OK);
    for (unsigned i=0;i<30;++i) ck2_trade_day(&s);
    assert(ck2_trade_annual_tax(s.posts,&catalog)==2);
    assert(ck2_bishop_nominate(&s,"b_test",1,99,"investiture_law_0")==CK2_GOV_OK);
    assert(ck2_bishop_approve(&s,"b_test",false)==CK2_GOV_UNSUPPORTED);
    assert(ck2_bishop_approve(&s,"b_test",true)==CK2_GOV_OK);
    assert(ck2_bishop_nominate(&s,"b_other",1,98,"investiture_law_1")==CK2_GOV_OK);
    assert(ck2_bishop_approve(&s,"b_other",false)==CK2_GOV_OK);
    assert(ck2_government_mechanics_valid(&s));
    s.posts[0].days_left=500;
    assert(!ck2_government_mechanics_valid(&s));
    ck2_trade_catalog_free(&catalog);
    if (argc>2) { char *file=read_all(argv[2]);assert(file);
        assert(ck2_trade_catalog_parse(&catalog,file));
        const struct ck2_trade_building *b=ck2_trade_find(&catalog,"tr_port_1");
        if (b) assert(b->gold_cost==100 && b->build_days==182);
        else {
            /* Optional independent fixture: 01_tradepost.txt is the CK2
             * Saharan chain, not the ordinary merchant port chain. */
            b=ck2_trade_find(&catalog,"tr_sahara_1");
            assert(b && b->gold_cost==200);
        }
        ck2_trade_catalog_free(&catalog);free(file);
    }
    puts("CK2 government mechanics: nomads, patrician respect, trade construction, investiture passed");
    return 0;
}
