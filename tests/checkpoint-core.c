/* Native checkpoint serialization round-trip: no SDL runtime is needed. */
#include "checkpoint_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void family_gameplay_free(struct family_gameplay *f) { free(f->people); *f = (struct family_gameplay){0}; }
void conversion_state_free(struct conversion_state *s)
{ free(s->provinces); free(s->people); *s = (struct conversion_state){0}; }
void warfare_state_free(struct warfare_state *s)
{ for (size_t i=0;i<s->army_count;++i) free(s->armies[i].route); *s=(struct warfare_state){0}; }
void ck2_law_state_free(struct ck2_law_state *s)
{ free(s->chosen); *s=(struct ck2_law_state){0}; }
void construction_checkpoint_free(struct construction_checkpoint *s)
{ free(s->buildings); free(s->settlements); *s=(struct construction_checkpoint){0}; }

int main(void)
{
    struct openck2_checkpoint c={0}, decoded={0};
    c.bookmark=(struct campaign_date){1066,9,15};
    c.bookmark_ruler=42;c.player=44;c.home_province=57;
    c.campaign.date=(struct campaign_date){1068,3,5};
    c.campaign.treasury=301;c.campaign.gold_remainder=0.75;
    c.campaign.monthly_tax=1.75;c.campaign.prestige=50.5;
    c.campaign.demesne_count=3;c.campaign.demesne_limit=5;
    c.family.count=1;
    c.family.people=calloc(1,sizeof *c.family.people);assert(c.family.people);
    c.family.people[0].profile.id=90001;
    c.family.people[0].profile.father=42;
    c.family.people[0].profile.dynasty_id=123;
    c.family.people[0].profile.known_stats=1;
    c.family.people[0].profile.stats[RULER_DIPLOMACY]=13;
    c.family.people[0].profile.trait_count=1;
    strcpy(c.family.people[0].profile.traits[0],"brave");
    c.family.people[0].profile.claim_count=1;
    strcpy(c.family.people[0].profile.claims[0].title,"k_england");
    strcpy(c.family.people[0].profile.name,"William's Child");
    strcpy(c.family.people[0].profile.culture_key,"norman");
    c.family.people[0].born=(struct campaign_date){1067,6,4};
    c.family.people[0].profile.age=1;
    c.family.next_id=90002;c.family.births=1;c.family.marriage_min_male=16;
    c.conversion.province_count=1;c.conversion.provinces=calloc(1,sizeof *c.conversion.provinces);
    assert(c.conversion.provinces);
    c.conversion.provinces[0].id=57;c.conversion.provinces[0].has_culture=true;
    strcpy(c.conversion.provinces[0].culture,"english");
    c.conversion.revision=2;
    c.warfare.war_count=1;c.warfare.wars[0]=(struct warfare_war){42,88,true};
    c.warfare.army_count=1;c.warfare.next_army_id=3;
    c.warfare.armies[0]=(struct warfare_army){.id=2,.ruler_id=42,.province_id=58,
        .troops=850,.origin=57,.target=59,.travel_days=2,
        .route=calloc(3,sizeof(unsigned)),.route_count=3,.route_step=1};
    assert(c.warfare.armies[0].route);
    memcpy(c.warfare.armies[0].route,(unsigned[]){57,58,59},3*sizeof(unsigned));
    c.warfare.levy_source_count=1;
    c.warfare.levy_sources[0]=(struct warfare_levy_source){42,57};
    c.laws.count=1;c.laws.chosen=calloc(1,sizeof *c.laws.chosen);assert(c.laws.chosen);
    strcpy(c.laws.chosen[0].group,"feudal_tax");
    strcpy(c.laws.chosen[0].key,"feudal_tax_1");
    c.construction.building_count=1;
    c.construction.buildings=calloc(1,sizeof *c.construction.buildings);assert(c.construction.buildings);
    strcpy(c.construction.buildings[0].holding,"b_york");
    strcpy(c.construction.buildings[0].building,"ca_barracks_1");
    c.construction.buildings[0].days=35;
    c.construction.settlement_count=1;
    c.construction.settlements=calloc(1,sizeof *c.construction.settlements);
    assert(c.construction.settlements);
    c.construction.settlements[0].province=57;
    c.construction.settlements[0].days=20;
    strcpy(c.construction.settlements[0].holding.id,"b_test_new_castle");
    strcpy(c.construction.settlements[0].holding.name,"New Castle");
    c.construction.settlements[0].holding.type=HOLDING_CASTLE;
    c.title_count=1;c.titles=calloc(1,sizeof *c.titles);assert(c.titles);
    strcpy(c.titles[0].id,"c_york");c.titles[0].owner=90001;
    c.governments.clan_count=1;
    c.governments.clans[0]=(struct ck2_nomad_clan){.head=44,.provinces=2,
                                .population=300,.capacity=1000,.manpower=60};
    c.governments.family_count=1;
    c.governments.families[0]=(struct ck2_patrician_family){.head=44,.campaign_fund=75};
    c.governments.post_count=1;
    c.governments.posts[0].province=57;
    c.governments.posts[0].patrician=44;
    strcpy(c.governments.posts[0].completed[0],"tr_port_1");
    c.governments.posts[0].completed_count=1;
    c.governments.bishop_count=1;
    strcpy(c.governments.bishops[0].temple,"b_test");
    c.governments.bishops[0].liege=44;
    c.governments.bishops[0].candidate=90001;
    const char *path="bin/checkpoint-test.dat";
    assert(checkpoint_write(path,&c));
    assert(checkpoint_read(path,&decoded));
    assert(decoded.bookmark.year==1066&&decoded.player==44);
    assert(decoded.campaign.date.year==1068&&decoded.campaign.treasury==301);
    assert(decoded.campaign.gold_remainder==0.75);
    assert(decoded.family.count==1&&decoded.family.people[0].profile.id==90001);
    assert(!strcmp(decoded.family.people[0].profile.traits[0],"brave"));
    assert(decoded.conversion.provinces[0].has_culture);
    assert(!strcmp(decoded.conversion.provinces[0].culture,"english"));
    assert(decoded.warfare.armies[0].route_count==3);
    assert(decoded.warfare.armies[0].route[2]==59);
    assert(decoded.construction.settlements[0].holding.type==HOLDING_CASTLE);
    assert(decoded.construction.buildings[0].days==35);
    assert(!strcmp(decoded.titles[0].id,"c_york")&&decoded.titles[0].owner==90001);
    assert(decoded.governments.clan_count==1&&decoded.governments.clans[0].head==44);
    assert(decoded.governments.clans[0].population==300);
    assert(decoded.governments.family_count==1&&decoded.governments.families[0].campaign_fund==75);
    assert(decoded.governments.post_count==1&&
           !strcmp(decoded.governments.posts[0].completed[0],"tr_port_1"));
    assert(decoded.governments.bishop_count==1&&
           !strcmp(decoded.governments.bishops[0].temple,"b_test"));
    checkpoint_free(&decoded);
    /* Checksum must reject tampering without modifying the target. */
    FILE *f=fopen(path,"r+b");assert(f);
    assert(fseek(f,20,SEEK_SET)==0);
    int byte=fgetc(f);assert(byte!=EOF);
    assert(fseek(f,20,SEEK_SET)==0);
    assert(fputc(byte^1,f)!=EOF);assert(!fclose(f));
    decoded=(struct openck2_checkpoint){.player=999};
    assert(!checkpoint_read(path,&decoded));
    assert(decoded.player==999);
    /* Writing a valid snapshot again atomically restores it. */
    assert(checkpoint_write(path,&c));
    assert(checkpoint_read(path,&decoded));
    checkpoint_free(&decoded);
    /* A bad writer input cannot destroy a previous valid save. */
    memset(c.family.people[0].profile.name,'Q',sizeof c.family.people[0].profile.name);
    assert(!checkpoint_write(path,&c));
    assert(checkpoint_read(path,&decoded));
    assert(decoded.player==44);
    checkpoint_free(&decoded);
    /* The versioned header rejects future formats. */
    f=fopen(path,"r+b");assert(f);
    assert(fseek(f,8,SEEK_SET)==0);
    assert(fputc(99,f)!=EOF);assert(!fclose(f));
    decoded=(struct openck2_checkpoint){.player=123};
    assert(!checkpoint_read(path,&decoded) && decoded.player==123);
    /* Truncated payloads fail before attempting a live-state commit. */
    f=fopen(path,"wb");assert(f);
    assert(fwrite("OCK2SAVE",1,8,f)==8);assert(!fclose(f));
    assert(!checkpoint_read(path,&decoded) && decoded.player==123);
    assert(!remove(path));
    checkpoint_free(&c);
    puts("Native checkpoint: all runtime subsystems, round-trip, checksum and atomic write passed");
    return 0;
}
