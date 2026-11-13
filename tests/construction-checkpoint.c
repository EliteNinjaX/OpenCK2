#include "construction.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct construction first={0}, reordered={0};
    first.catalog=calloc(2,sizeof *first.catalog);
    reordered.catalog=calloc(2,sizeof *reordered.catalog);
    assert(first.catalog&&reordered.catalog);
    first.count=reordered.count=2;
    first.catalog[0]=(struct building){.type=HOLDING_CASTLE,.gold=10,.days=45,
        .requirements="}",.supported=true};
    strcpy(first.catalog[0].id,"ca_barracks_1");
    first.catalog[1]=(struct building){.type=HOLDING_CASTLE,.gold=20,.days=70,
        .requirements="}",.supported=true};
    strcpy(first.catalog[1].id,"ca_stables_1");
    reordered.catalog[0]=first.catalog[1];
    reordered.catalog[1]=first.catalog[0];
    struct holding holding={.type=HOLDING_CASTLE};
    strcpy(holding.id,"b_york");
    unsigned treasury=100;
    assert(construction_start(&first,&holding,0,true,&treasury));
    assert(treasury==90);
    struct construction_checkpoint save={0};
    assert(construction_checkpoint_capture(&first,&save));
    assert(save.building_count==1 && save.buildings[0].days==45);
    assert(!strcmp(save.buildings[0].building,"ca_barracks_1"));
    assert(construction_checkpoint_valid(&reordered,&save));
    assert(construction_checkpoint_restore(&reordered,&save));
    unsigned days=0;
    assert(construction_status(&reordered,&holding,1,true,treasury,&days)==BUILD_RUNNING);
    assert(days==45);
    strcpy(save.buildings[0].building,"no_such_building");
    assert(!construction_checkpoint_valid(&reordered,&save));
    assert(!construction_checkpoint_restore(&reordered,&save));
    assert(construction_status(&reordered,&holding,1,true,treasury,&days)==BUILD_RUNNING);
    strcpy(save.buildings[0].building,"ca_barracks_1");
    construction_checkpoint_free(&save);
    construction_reset(&first);
    construction_reset(&reordered);
    free(first.catalog);free(reordered.catalog);
    puts("Construction checkpoint: catalog reordering, valid restore and invalid data rejection passed");
}
