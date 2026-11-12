#include "../src/game/government_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void synthetic(void)
{
    const char *data = "feudal_governments = { \n"
      " feudal_government = { preferred_holdings = { CASTLE } allowed_holdings = { CASTLE FORT } "
      "potential = { OR = { a = yes NOT = { b = no } } } "
      " color = { 91 133 207 } aggression = 2.0 } }";
    struct government_catalog catalog = {0};
    assert(government_catalog_parse(&catalog, data));
    assert(catalog.count == 1);
    const struct government_definition *g = government_find(&catalog, "feudal_government");
    assert(g && g->preferred_holdings == GOVERNMENT_CASTLE);
    assert(g->allowed_holdings == (GOVERNMENT_CASTLE | GOVERNMENT_FORT));
    assert(g->color_rgb == 0x5b85cfu && g->color_defined);
    assert(g->has_potential && strstr(g->potential_script, "NOT") != NULL);
    assert(g->aggression == 2.0);
    assert(government_holding_candidates(&catalog, GOVERNMENT_CASTLE, NULL, 0) == 1);
    assert(government_holding_candidates(&catalog, GOVERNMENT_NOMAD, NULL, 0) == 0);
    assert(government_catalog_parse(&catalog,
      "feudal_governments={feudal_government={preferred_holdings={CITY} frame_suffix=\"_new\"}}"));
    assert(catalog.count == 1);
    assert(government_find(&catalog, "feudal_government")->preferred_holdings == GOVERNMENT_CITY);
    assert(!government_find(&catalog, "feudal_government")->has_potential);
    government_catalog_destroy(&catalog);
    assert(!catalog.count && !catalog.items);
}

static void original_file(const char *file, struct government_catalog *catalog)
{
    FILE *f = fopen(file, "rb"); assert(f);
    assert(fseek(f, 0, SEEK_END) == 0);
    long length = ftell(f); assert(length > 0 && length < 1000000);
    rewind(f);
    char *data = malloc((size_t)length + 1); assert(data);
    assert(fread(data, 1, (size_t)length, f) == (size_t)length);
    data[length] = 0;
    fclose(f);
    assert(government_catalog_parse(catalog, data));
    free(data);
}
int main(int argc, char **argv)
{
    synthetic();
    if (argc > 1) {
        struct government_catalog catalog = {0};
        for (int i = 1; i < argc; ++i) original_file(argv[i], &catalog);
        assert(government_find(&catalog, "nomadic_government"));
        assert(government_find(&catalog, "feudal_government"));
        assert(government_find(&catalog, "tribal_government"));
        const struct government_definition *nomadic = government_find(&catalog, "nomadic_government");
        assert(nomadic->preferred_holdings == GOVERNMENT_NOMAD);
        assert(nomadic->color_defined && nomadic->has_potential);
        printf("government core: parsed %zu original definitions\n", catalog.count);
        government_catalog_destroy(&catalog);
    }
    puts("government core: all assertions passed");
    return 0;
}
