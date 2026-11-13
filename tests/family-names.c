#include "family_names_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char fixture[] =
    "scandinavian = { norse = { male_names = { Erik_Eric Olaf Knut } "
    "female_names = { Astrid Ingibjorg } } }\n"
    "latin = { italian = { male_names = { Giovanni } female_names = { Maria } } }";
int main(int argc, char **argv)
{
    char name[96];
    assert(family_name_choose(fixture, "norse", false, 0, name, sizeof name));
    assert(!strcmp(name,"Erik"));
    assert(family_name_choose(fixture, "norse", true, 1, name, sizeof name));
    assert(!strcmp(name,"Ingibjorg"));
    assert(family_name_choose(fixture, "italian", true, 5, name, sizeof name));
    assert(!strcmp(name,"Maria"));
    assert(!family_name_choose(fixture,"missing",true,0,name,sizeof name));
    if (argc > 1) {
        FILE *f = fopen(argv[1],"rb");
        assert(f); assert(fseek(f,0,SEEK_END)==0);
        long n = ftell(f); assert(n > 0 && n < 1000000);
        rewind(f);
        char *source = malloc((size_t)n + 1); assert(source);
        assert(fread(source,1,(size_t)n,f)==(size_t)n);
        source[n] = 0;
        fclose(f);
        assert(family_name_choose(source,"norse",false,1,name,sizeof name));
        free(source);
    }
    puts("family names: all assertions passed");
}
