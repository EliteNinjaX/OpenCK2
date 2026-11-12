#include "../src/game/settlement_styles.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void read_file(const char *path, struct settlement_styles *s, int actors)
{
    FILE *f = fopen(path,"rb"); assert(f);
    assert(fseek(f,0,SEEK_END)==0);
    long length = ftell(f); assert(length>0 && length<1000000);
    rewind(f);
    char *data=malloc((size_t)length+1); assert(data);
    assert(fread(data,1,(size_t)length,f)==(size_t)length);
    data[length]=0; fclose(f);
    if (!(actors ? settlement_styles_parse_actors(s,data):settlement_styles_parse_cultures(s,data))) { fprintf(stderr, "Parse failed %s\n", path); abort(); }
    free(data);
}
int main(int argc, char **argv)
{
    struct settlement_styles s={0};
    assert(settlement_styles_parse_cultures(&s,
        "european={graphical_cultures={westerngfx} german={color={0.5 0.5 0.5}}"
        "norse={graphical_cultures={norsegfx}}}"));
    assert(!strcmp(settlement_culture_find(&s,"german")->graphics,"westerngfx"));
    assert(!strcmp(settlement_culture_find(&s,"norse")->graphics,"norsegfx"));
    assert(settlement_styles_parse_actors(&s,
        "objectTypes={EMFXActorType={name=\"westerngfx_castle_1\" "
        "actorfile=\"gfx\\models\\Settlements\\Western\\Castle.xac\" scale=1.0f}}"));
    assert(settlement_actor_find(&s,"westerngfx","castle",1));
    assert(strstr(settlement_actor_find(&s,"westerngfx","castle",1)->path,"/Western/"));
    assert(!settlement_actor_find(&s,"norsegfx","castle",1));
    settlement_styles_destroy(&s);
    if (argc>2) {
        read_file(argv[1],&s,0); read_file(argv[2],&s,1);
        assert(s.culture_count>100 && s.actor_count>70);
        assert(settlement_culture_find(&s,"german"));
        assert(settlement_actor_find(&s,"westerngfx","castle",1));
        printf("styles core: %zu cultures and %zu actor assets\n",s.culture_count,s.actor_count);
    }
    settlement_styles_destroy(&s);
    puts("styles core: all assertions passed");
    return 0;
}
