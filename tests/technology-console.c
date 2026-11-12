#include "technology_core.h"
#include "debug_console.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    assert(!fseek(f, 0, SEEK_END));
    long n = ftell(f);
    assert(n >= 0 && !fseek(f, 0, SEEK_SET));
    char *s = malloc((size_t)n + 1);
    assert(s && fread(s, 1, (size_t)n, f) == (size_t)n);
    s[n] = '\0';
    fclose(f);
    return s;
}
int main(int argc, char **argv)
{
    struct ck2_technology_catalog catalog;
    assert(ck2_technology_parse(&catalog, "# comment\nTECH_GROUP_MILITARY = { TECH_INFANTRY = { modifier = { foo = 5 } 1 = { add_building = x } } }\nTECH_GROUP_CULTURE = { TECH_LEGALISM = { modifier = { bar = yes } } }\n") == 0);
    assert(catalog.count == 2 && ck2_technology_group_count(&catalog, CK2_TECH_MILITARY) == 1);
    assert(ck2_technology_find(&catalog, "TECH_LEGALISM"));
    assert(ck2_technology_parse(&catalog, "TECH_GROUP_CULTURE = { TECH_ABC = {") == -1);
    if (argc > 1) {
        char *contents = read_text(argv[1]); assert(contents);
        assert(ck2_technology_parse(&catalog, contents) == 0);
        assert(ck2_technology_group_count(&catalog, CK2_TECH_MILITARY) > 0);
        assert(ck2_technology_group_count(&catalog, CK2_TECH_ECONOMY) > 0);
        assert(ck2_technology_group_count(&catalog, CK2_TECH_CULTURE) > 0);
        free(contents);
    }
    struct debug_console console = {0};
    struct debug_console_context state = {.date="1 January 1066",.province="Constantinople",.gold=15};
    assert(strstr(debug_console_execute(&console, "help", &state).message, "mapmode"));
    assert(strstr(debug_console_execute(&console, "date", &state).message, "1066"));
    assert(debug_console_execute(&console, "mapmode terrain", &state).action == DEBUG_SET_MAP_MODE);
    assert(debug_console_execute(&console, "tech", &state).action == DEBUG_LIST_TECH);
    assert(strstr(debug_console_execute(&console, "invalid", &state).message, "Unknown"));
    assert(debug_console_execute(&console, "clear", &state).action == DEBUG_CLEAR && console.count == 0);
    puts("technology/console core: all assertions passed");
    return 0;
}
