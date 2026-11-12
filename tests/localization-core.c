#include "../src/game/localization_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    const char *line = "DYNA_FAMILYTREE;Family Tree;Arbre familial;Stammbaum;;Arbol familiar;;;;;;;;;x";
    for (int language = CK2_ENGLISH; language < CK2_LANGUAGE_COUNT; ++language) {
        struct loc_catalog loc;
        loc_catalog_init(&loc, (enum ck2_language)language);
        assert(loc_catalog_add_line(&loc, line, strlen(line)));
        const char *expected[] = {"Family Tree", "Arbre familial", "Stammbaum", "Arbol familiar"};
        assert(!strcmp(loc_catalog_find(&loc, "DYNA_FAMILYTREE"), expected[language]));
        assert(!loc_catalog_find(&loc, "UNKNOWN"));
        assert(loc_catalog_add_line(&loc, "BACK;Back;;;;;x", strlen("BACK;Back;;;;;x")));
        assert(!strcmp(loc_catalog_find(&loc, "BACK"), "Back"));
        for (int i = 0; i < 4000; ++i) {
            char buffer[96];
            snprintf(buffer, sizeof buffer, "key%d;Value%d;X;;;%d;", i, i, i);
            assert(loc_catalog_add_line(&loc, buffer, strlen(buffer)));
        }
        assert(loc_catalog_add_line(&loc, "QUOTE;\"Book;Title\";\"Livre;Titre\";Titel;;Libro;", strlen("QUOTE;\"Book;Title\";\"Livre;Titre\";Titel;;Libro;")));
        assert(!strcmp(loc_catalog_find(&loc, "QUOTE"), language == CK2_FRENCH ? "Livre;Titre" :
                       language == CK2_ENGLISH ? "Book;Title" : language == CK2_GERMAN ? "Titel" : "Libro"));
        assert(loc.count == 4003);
        loc_catalog_free(&loc);
    }
    if (argc > 1) {
        FILE *file = fopen(argv[1], "rb");
        assert(file);
        struct loc_catalog loc;
        loc_catalog_init(&loc, CK2_FRENCH);
        char buffer[32768];
        while (fgets(buffer, sizeof buffer, file))
            assert(loc_catalog_add_line(&loc, buffer, strlen(buffer)));
        assert(!strcmp(loc_catalog_find(&loc, "DYNA_FAMILYTREE"), "Arbre familial"));
        printf("localisation fixture: %zu entries\n", loc.count);
        fclose(file);
        loc_catalog_free(&loc);
    }
    puts("localization core: all assertions passed");
}
