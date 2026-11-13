#include "../src/game/conversion_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file=fopen(path,"rb"); if (!file) return NULL;
    if (fseek(file,0,SEEK_END)) { fclose(file); return NULL; }
    long size=ftell(file);
    if (size<0 || size>16000000L) { fclose(file); return NULL; }
    rewind(file);
    char *text=malloc((size_t)size+1);
    if (!text) { fclose(file); return NULL; }
    if (fread(text,1,(size_t)size,file)!=(size_t)size) { fclose(file); free(text); return NULL; }
    text[size]=0; fclose(file); return text;
}

int main(int argc,char **argv)
{
    struct conversion_catalog catalog={0};
    struct conversion_state state={0};
    assert(conversion_catalog_parse(&catalog,CONVERSION_RELIGION,
        "christian = { color = { 0 0 0 } catholic = { parent = orthodox } "
        "orthodox = { icon = 1 } }"));
    assert(conversion_catalog_parse(&catalog,CONVERSION_CULTURE,
        "latin = { graphical_cultures = { westerngfx } frankish = { male_names = { Charles } } "
        "italian = { male_names = { Mario } } }"));
    assert(conversion_catalog_has(&catalog, CONVERSION_RELIGION,"catholic"));
    assert(conversion_catalog_has(&catalog, CONVERSION_CULTURE,"frankish"));
    assert(!conversion_catalog_has(&catalog, CONVERSION_RELIGION,"christian"));
    assert(!conversion_catalog_has(&catalog, CONVERSION_CULTURE,"male_names"));
    assert(conversion_catalog_jobs(&catalog, "action_inquisition = { attribute = learning "
          "trigger = { any_province_lord = { character = FROM } } events = { 900 901 902 903 } } "
          "action_settle_tribe = { events = { 925 20206 } }"));
    assert(catalog.inquisition_defined && catalog.settle_tribe_defined);
    struct conversion_catalog no_jobs = {0};
    assert(conversion_catalog_parse(&no_jobs, CONVERSION_RELIGION,
           "christian = { catholic = { } orthodox = { } }"));
    assert(conversion_mission_assign(&state,&no_jobs,CONVERSION_RELIGION,
           21,57,"catholic","orthodox") == CONVERSION_UNAVAILABLE);
    conversion_catalog_free(&no_jobs);
    assert(!strcmp(catalog.inquisition_events,"900 901 902 903"));
    assert(!strcmp(catalog.settle_tribe_events,"925 20206"));
    assert(conversion_province_set(&state,&catalog,22,CONVERSION_RELIGION,"orthodox","catholic")==CONVERSION_OK);
    assert(!strcmp(conversion_province_value(&state,22,CONVERSION_RELIGION,"catholic"),"orthodox"));
    assert(!strcmp(conversion_province_value(&state,22,CONVERSION_CULTURE,"frankish"),"frankish"));
    assert(conversion_province_set(&state,&catalog,22,CONVERSION_CULTURE,"italian","frankish")==CONVERSION_OK);
    assert(conversion_province_set(&state,&catalog,22,CONVERSION_CULTURE,"fake","frankish")==CONVERSION_UNKNOWN_KEY);
    assert(conversion_person_set(&state,&catalog,57,CONVERSION_RELIGION,"orthodox","catholic")==CONVERSION_OK);
    assert(!strcmp(conversion_person_value(&state,57,CONVERSION_RELIGION,"catholic"),"orthodox"));
    assert(conversion_mission_assign(&state,&catalog,CONVERSION_CULTURE,33,57,"italian","frankish")==CONVERSION_OK);
    assert(conversion_mission_complete(&state,&catalog,CONVERSION_CULTURE,"frankish")==CONVERSION_OK);
    assert(!strcmp(conversion_province_value(&state,33,CONVERSION_CULTURE,"frankish"),"italian"));
    assert(conversion_mission_complete(&state,&catalog,CONVERSION_CULTURE,"frankish")==CONVERSION_NO_MISSION);
    if (argc>3) {
        char *religions=read_file(argv[1]);
        char *cultures=read_file(argv[2]);
        char *jobs=read_file(argv[3]);
        assert(religions && cultures && jobs);
        assert(conversion_catalog_parse(&catalog,CONVERSION_RELIGION,religions));
        assert(conversion_catalog_parse(&catalog,CONVERSION_CULTURE,cultures));
        assert(conversion_catalog_jobs(&catalog,jobs));
        assert(conversion_catalog_has(&catalog,CONVERSION_RELIGION,"catholic"));
        assert(conversion_catalog_has(&catalog,CONVERSION_RELIGION,"orthodox"));
        assert(conversion_catalog_has(&catalog,CONVERSION_CULTURE,"english"));
        printf("Original CK2: %zu faiths, %zu cultures, council actions %d/%d\n",
               catalog.names[CONVERSION_RELIGION].count,catalog.names[CONVERSION_CULTURE].count,
               catalog.inquisition_defined,catalog.settle_tribe_defined);
        free(religions); free(cultures); free(jobs);
    }
    conversion_state_free(&state);
    conversion_catalog_free(&catalog);
    puts("conversion core: all assertions passed");
    return 0;
}
