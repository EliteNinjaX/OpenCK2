#include "law_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *slurp(const char *name)
{
    FILE *f=fopen(name,"rb"); if (!f) return NULL;
    if (fseek(f,0,SEEK_END)) {fclose(f);return NULL;}
    long n=ftell(f); if (n<0 || n>8000000) {fclose(f);return NULL;}
    rewind(f);char *s=malloc((size_t)n+1);
    if (!s) {fclose(f);return NULL;}
    if (fread(s,1,(size_t)n,f)!=(size_t)n) {fclose(f);free(s);return NULL;}
    s[n]=0;fclose(f);return s;
}
int main(int argc,char **argv)
{
    struct ck2_law_catalog c={0};struct ck2_law_state s={0};double fraction=-1;
    const char *fixture="laws = { feudal_tax_0 = { group = feudal_tax castle_vassal_tax_modifier = 0.0 "
        " effect = { castle_vassal_tax_modifier = 0.99 } } feudal_tax_2 = { group = feudal_tax "
        " castle_vassal_tax_modifier = 0.2 } city_tax_1 = { group = city_tax "
        " city_vassal_tax_modifier = 0.25 } } "
        "succession_laws = { succ_primogeniture = { effect = { succession = primogeniture } } "
        " succ_gavelkind = { effect = { succession = gavelkind } } "
        " succ_feudal_elective = { effect = { succession = { base_type = feudal_elective } } } } "
        "gender_laws = { agnatic_succession = { effect = { gender_succ = agnatic } } "
        " cognatic_succession = { effect = { gender_succ = cognatic } } }";
    assert(ck2_law_parse(&c,fixture));
    assert(c.count==8);
    const struct ck2_law *law=ck2_law_find(&c,"feudal_tax_0");
    assert(law && law->has_castle_tax && law->castle_tax==0.0);
    assert(!ck2_law_holding_tax(&s,&c,1,&fraction));
    assert(ck2_law_select(&s,&c,"feudal_tax_2"));
    assert(ck2_law_holding_tax(&s,&c,1,&fraction) && fabs(fraction-.2)<.0001);
    assert(ck2_law_select(&s,&c,"feudal_tax_0"));
    assert(ck2_law_holding_tax(&s,&c,1,&fraction) && fraction==0);
    assert(ck2_law_select(&s,&c,"city_tax_1"));
    assert(ck2_law_holding_tax(&s,&c,2,&fraction) && fraction==.25);
    assert(ck2_law_succession_mode(ck2_law_find(&c,"succ_feudal_elective"))==CK2_SUCC_UNSUPPORTED);
    assert(ck2_law_gender_mode(ck2_law_find(&c,"cognatic_succession"))==CK2_GENDER_COGNATIC);
    assert(ck2_law_select(&s,&c,"succ_primogeniture"));
    assert(ck2_law_selected(&s,&c,"succession"));
    const struct ck2_heir_candidate kids[]={
      {.id=11,.father=1,.dynasty_id=5,.female=true,.age=20,.age_known=true,.alive=true},
      {.id=12,.father=1,.dynasty_id=5,.female=false,.age=17,.age_known=true,.alive=true},
      {.id=13,.father=1,.dynasty_id=5,.female=false,.age=9,.age_known=true,.alive=true},
      {.id=14,.father=1,.dynasty_id=7,.female=false,.age=24,.age_known=true,.alive=true},
      {.id=15,.father=1,.dynasty_id=5,.female=false,.age=30,.age_known=true,.alive=false}};
    assert(ck2_succession_heir(CK2_SUCC_PRIMOGENITURE,CK2_GENDER_AGNATIC,1,5,kids,5)==12);
    assert(ck2_succession_heir(CK2_SUCC_PRIMOGENITURE,CK2_GENDER_COGNATIC,1,5,kids,5)==12);
    assert(ck2_succession_heir(CK2_SUCC_PRIMOGENITURE,CK2_GENDER_EQUAL,1,5,kids,5)==11);
    assert(ck2_succession_heir(CK2_SUCC_ULTIMOGENITURE,CK2_GENDER_AGNATIC,1,5,kids,5)==13);
    unsigned out[4]={0};
    assert(ck2_succession_divide(CK2_SUCC_GAVELKIND,CK2_GENDER_AGNATIC,1,5,kids,5,out,4)==2);
    assert(out[0]==12 && out[1]==13);
    assert(ck2_succession_heir(CK2_SUCC_UNSUPPORTED,CK2_GENDER_EQUAL,1,5,kids,5)==0);
    ck2_law_state_free(&s);
    const char *history="law = feudal_tax_0 "
       "1080.1.1 = { law = feudal_tax_2 law = city_tax_1 } "
       "1066.9.15 = { holder = 123 law = succ_gavelkind law = feudal_tax_0 } "
       "1111.1.1 = { law = succ_primogeniture law = feudal_tax_2 } "
       "1070.4.1 = { law = feudal_tax_2 }";
    assert(ck2_law_title_history(&s,&c,history,10720101));
    assert(!strcmp(ck2_law_selected(&s,&c,"feudal_tax")->key,"feudal_tax_2"));
    assert(!strcmp(ck2_law_selected(&s,&c,"succession")->key,"succ_gavelkind"));
    assert(!ck2_law_selected(&s,&c,"city_tax"));
    ck2_law_state_free(&s);
    if (argc>1) {
        ck2_law_catalog_free(&c);
        for (int i=1;i<argc;++i) {
            char *source=slurp(argv[i]);assert(source);
            assert(ck2_law_parse(&c,source));free(source);
        }
        law=ck2_law_find(&c,"succ_primogeniture");assert(law);
        assert(ck2_law_find(&c,"feudal_tax_2"));
        assert(ck2_law_find(&c,"tax_levy_law_feudal_3"));
        assert(ck2_law_find(&c,"agnatic_succession"));
        fprintf(stdout,"Parsed %zu original CK2 law definitions\n",c.count);
    }
    ck2_law_catalog_free(&c);
    puts("Law parsing, law groups, and limited inheritance rules passed");
    return 0;
}
