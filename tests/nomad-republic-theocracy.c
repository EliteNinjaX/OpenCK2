/* SDL-independent tests. Optional file paths read actual CK2 government data. */
#include "../src/game/government_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void load_file(struct government_catalog *cat, const char *filename)
{
    FILE *f=fopen(filename,"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));
    long n=ftell(f);
    assert(n>0 && n<1000000 && !fseek(f,0,SEEK_SET));
    char *buf=malloc((size_t)n+1);assert(buf);
    assert(fread(buf,1,(size_t)n,f)==(size_t)n);
    buf[n]=0;
    assert(government_catalog_parse(cat,buf));
    free(buf);fclose(f);
}
static void check_policies(struct government_catalog *cat)
{
    const struct government_definition *nomad=government_find(cat,"nomadic_government");
    const struct government_definition *republic=government_find(cat,"republic_government");
    const struct government_definition *merchant=government_find(cat,"merchant_republic_government");
    const struct government_definition *temple=government_find(cat,"theocracy_government");
    assert(nomad && republic && merchant && temple);
    assert(nomad->preferred_holdings == GOVERNMENT_NOMAD);
    assert(republic->preferred_holdings == GOVERNMENT_CITY);
    assert(merchant->preferred_holdings == GOVERNMENT_CITY);
    assert(temple->preferred_holdings == GOVERNMENT_TEMPLE);
    assert(government_provisional_holding(cat,GOVERNMENT_NOMAD) == nomad);
    assert(government_provisional_holding(cat,GOVERNMENT_CITY) == republic);
    assert(government_provisional_holding(cat,GOVERNMENT_TEMPLE) == temple);
    assert(government_provisional_holding(cat,GOVERNMENT_CASTLE) == NULL);
    assert(government_provisional_holding(cat,GOVERNMENT_CITY|GOVERNMENT_TEMPLE) == NULL);
    assert(government_vassal_tax_exempt(nomad,nomad));
    assert(!government_vassal_tax_exempt(nomad,republic));
    assert(!government_vassal_tax_exempt(republic,nomad));
    assert(government_key_set_has(&nomad->vassal_call_to_arms,"nomadic_government"));
    assert(government_key_set_has(&nomad->forced_contract,"feudal_government"));
    assert(!government_can_found_holding(nomad));
    assert(!government_uses_hereditary_succession(merchant));
    assert(!government_uses_hereditary_succession(temple));
    assert(temple->have_gender_laws_defined && !temple->have_gender_laws);
    assert(merchant->have_gender_laws_defined && !merchant->have_gender_laws);
    assert(merchant->merchant_republic && !republic->merchant_republic);
    assert(!government_allows_law_group(merchant,"gender_succession"));
    assert(!government_allows_law_group(merchant,"succession"));
    assert(!government_allows_law_group(temple,"gender_succession"));
    assert(!government_allows_law_group(temple,"succession"));
    assert(government_allows_law_group(nomad,"succession"));
    assert(government_allows_law_group(republic,"city_tax"));
    assert(nomad->color_defined && temple->color_defined && merchant->color_defined);
}
static void check_evidence(void)
{
    struct government_holding_evidence facts[]={
        {11,42,GOVERNMENT_CASTLE,"b_castle"},
        {11,42,GOVERNMENT_CITY,"b_city"},
        {22,80,GOVERNMENT_CITY,"b_port"},
        {22,81,GOVERNMENT_CITY,"b_town"},
        {33,14,GOVERNMENT_TEMPLE,"b_abbey"},
        {44,17,GOVERNMENT_NOMAD,"b_camp"},
        {55,18,GOVERNMENT_CASTLE,"b_castle"},
        {55,23,GOVERNMENT_NOMAD,"b_camp"},
        {66,19,GOVERNMENT_TEMPLE,"b_temple"},
        {66,19,GOVERNMENT_CITY,"b_city"}
    };
    const size_t n=sizeof facts/sizeof facts[0];
    /* Do not infer from a mixed estate, unless a directly held primary
     * barony or unambiguous title capital identifies the owner's seat. */
    assert(!government_verified_holding(facts,n,11,"c_unknown",0));
    assert(government_verified_holding(facts,n,11,"b_castle",0)==GOVERNMENT_CASTLE);
    assert(government_verified_holding(facts,n,11,"b_city",0)==GOVERNMENT_CITY);
    assert(government_verified_holding(facts,n,22,"d_republic",0)==GOVERNMENT_CITY);
    assert(government_verified_holding(facts,n,33,"b_abbey",0)==GOVERNMENT_TEMPLE);
    assert(government_verified_holding(facts,n,44,"k_steppe",0)==GOVERNMENT_NOMAD);
    assert(government_verified_holding(facts,n,55,"k_steppe",23)==GOVERNMENT_NOMAD);
    assert(!government_verified_holding(facts,n,66,"c_temple",19));
    assert(!government_verified_holding(facts,n,0,"c_unknown",0));
    assert(!government_verified_holding(NULL,10,1,"c_unknown",0));
}
static void check_religion_groups(void)
{
    struct government_religions index={0};
    const char *synthetic="christian={female_temple_holders=no catholic={female_temple_holders=yes} "
        "orthodox={ icon=2 } } muslim={sunni={icon=1} shia={icon=2}} "
        "secret_religion_visibility_trigger={secret_religion=FROM}";
    assert(government_religions_parse(&index,synthetic));
    const struct government_religion *c=government_religion_find(&index,"catholic");
    const struct government_religion *o=government_religion_find(&index,"orthodox");
    const struct government_religion *s=government_religion_find(&index,"sunni");
    assert(c && !strcmp(c->group,"christian") && c->female_temple && c->male_temple);
    assert(o && !strcmp(o->group,"christian") && !o->female_temple && o->male_temple);
    assert(s && !strcmp(s->group,"muslim") && s->male_temple);
    assert(!government_religion_find(&index,"never_defined"));
    government_religions_free(&index);
    const char *file=getenv("CK2_RELIGION_TEST_FILE");
    if (!file || !*file) return;
    FILE *f=fopen(file,"rb"); assert(f);
    assert(!fseek(f,0,SEEK_END));
    long n=ftell(f); assert(n>0 && n<5000000 && !fseek(f,0,SEEK_SET));
    char *source=malloc((size_t)n+1); assert(source);
    assert(fread(source,1,(size_t)n,f)==(size_t)n);
    source[n]=0;fclose(f);
    assert(government_religions_parse(&index,source));
    free(source);
    c=government_religion_find(&index,"catholic");
    s=government_religion_find(&index,"sunni");
    const struct government_religion *cathar=government_religion_find(&index,"cathar");
    assert(c && !strcmp(c->group,"christian") && !c->female_temple);
    assert(cathar && !strcmp(cathar->group,"christian") && cathar->female_temple);
    assert(s && !strcmp(s->group,"muslim"));
    printf("original CK2 religion groups: %zu entries parsed\n",index.count);
    government_religions_free(&index);
}

int main(int argc, char **argv)
{
    struct government_catalog cat={0};
    check_evidence();
    check_religion_groups();
    if (argc>1) {
        for (int i=1;i<argc;++i) load_file(&cat,argv[i]);
    } else {
        const char *data=
          "nomadic_governments={ nomadic_government={ preferred_holdings={NOMAD} "
          "no_vassal_tax={nomadic_government} vassal_call_to_arms={nomadic_government} "
          "forced_contract={feudal_government} can_build_holdings=no color={201 179 36} } }"
          "republic_governments={ republic_government={ preferred_holdings={CITY} } "
          "merchant_republic_government={preferred_holdings={CITY} merchant_republic=yes "
          "landed_inheritance=no have_gender_laws=no color={250 0 0} } }"
          "theocracy_governments={theocracy_government={preferred_holdings={TEMPLE} "
          "have_gender_laws=no color={220 220 220} } }";
        assert(government_catalog_parse(&cat,data));
    }
    check_policies(&cat);
    printf("government systems: %zu definitions, evidence and rule checks passed\n",cat.count);
    government_catalog_destroy(&cat);
    return 0;
}
