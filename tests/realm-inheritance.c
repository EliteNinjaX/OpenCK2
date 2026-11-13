/* Integration regression: use the exact production realm-history/title
 * mutation code. The private realm_history layout is included deliberately;
 * SDL drawing and file loading are discarded at link time. */
#include "../src/game/realms.c"
#include <assert.h>
#include <stdio.h>

int ruler_compare(const void *left,const void *right)
{
    unsigned a=((const struct ruler_profile *)left)->id;
    unsigned b=((const struct ruler_profile *)right)->id;
    return (a>b)-(a<b);
}
int main(void)
{
    struct realm_history h={0};struct realm_map map={.history=&h};
    h.titles=calloc(4,sizeof(*h.titles));assert(h.titles);
    h.count=4;
    strcpy(h.titles[0].id,"c_county_a");h.titles[0].holder=2;h.titles[0].rank=2;
    strcpy(h.titles[0].liege,"k_crown");
    strcpy(h.titles[1].id,"c_county_b");h.titles[1].holder=2;h.titles[1].rank=2;
    strcpy(h.titles[1].liege,"k_crown");
    strcpy(h.titles[2].id,"d_duchy");h.titles[2].holder=2;h.titles[2].rank=3;
    strcpy(h.titles[2].liege,"k_crown");
    strcpy(h.titles[3].id,"k_crown");h.titles[3].holder=2;h.titles[3].rank=4;
    h.county=calloc(2,sizeof(int));h.province=calloc(2,sizeof(int));
    assert(h.county&&h.province);
    h.county[0]=0;h.county[1]=1;h.province_count=2;
    h.rulers=calloc(2,sizeof(*h.rulers));assert(h.rulers);
    h.ruler_count=2;h.rulers[0].id=2;h.rulers[1].id=3;
    assert(!realm_runtime_inherit_titles(&map,2,(unsigned[]){3,4},2));
    struct ruler_profile heir4={.id=4};
    strcpy(heir4.name,"Heir Four");
    assert(realm_register_runtime_ruler(&map,&heir4));
    const unsigned recipients[]={3,4};
    for (size_t i=0;i<h.count;++i) h.titles[i].bookmark_holder=h.titles[i].holder;
    assert(realm_runtime_title_overrides(&map,NULL,0)==0);
    assert(realm_held_title_count(&map,2)==4);
    assert(realm_runtime_inherit_titles(&map,2,recipients,2)==4);
    assert(realm_held_title_count(&map,2)==0);
    assert(realm_held_title_count(&map,3)==2);
    assert(realm_held_title_count(&map,4)==2);
    struct realm_title_info crown;
    assert(realm_title_info(&map,"k_crown",&crown));
    assert(crown.holder==3);
    assert(realm_ruler(&map,4));
    assert(!realm_runtime_inherit_titles(&map,2,recipients,2));
    /* Ensure the dated province and independent owner tables were refreshed. */
    assert(h.province[0]==3 && h.province[1]==3);
    struct realm_title_override delta[4]={0};
    assert(realm_runtime_title_overrides(&map,delta,4)==4);
    assert(realm_runtime_title_overrides_valid(&map,delta,4));
    const struct realm_title_override bad={{'x'},3};
    assert(!realm_runtime_title_overrides_restore(&map,&bad,1));
    assert(realm_title_info(&map,"k_crown",&crown)&&crown.holder==3);
    assert(realm_runtime_title_overrides_restore(&map,NULL,0));
    assert(realm_held_title_count(&map,2)==4);
    assert(realm_runtime_title_overrides_restore(&map,delta,4));
    assert(realm_held_title_count(&map,2)==0);
    assert(h.province[0]==3 && h.province[1]==3);

    free(h.rulers);free(h.titles);free(h.county);free(h.province);
    puts("Realm inheritance: atomic transfer, primary title, heir index, realm references passed");
}
