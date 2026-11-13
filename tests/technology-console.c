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
    assert(debug_console_execute(&console, "mapmode culture", &state).action == DEBUG_SET_MAP_MODE);
    assert(strstr(debug_console_execute(&console, "levy", &state).message, "Select a province"));
    state.has_levy = true;
    state.levy_potential = 225;
    assert(strstr(debug_console_execute(&console, "levy", &state).message, "225"));
    assert(debug_console_execute(&console, "tech", &state).action == DEBUG_LIST_TECH);
    assert(debug_console_execute(&console, "faith orthodox", &state).action == DEBUG_SET_PROVINCE_RELIGION);
    assert(debug_console_execute(&console, "culture english", &state).action == DEBUG_SET_PROVINCE_CULTURE);
    assert(debug_console_execute(&console, "charfaith catholic", &state).action == DEBUG_SET_CHARACTER_RELIGION);
    assert(debug_console_execute(&console, "charculture italian", &state).action == DEBUG_SET_CHARACTER_CULTURE);
    assert(debug_console_execute(&console, "missionfaith", &state).action == DEBUG_MISSION_RELIGION);
    assert(debug_console_execute(&console, "missionculture", &state).action == DEBUG_MISSION_CULTURE);
    assert(debug_console_execute(&console, "resolvefaith", &state).action == DEBUG_RESOLVE_RELIGION);
    assert(debug_console_execute(&console, "resolveculture", &state).action == DEBUG_RESOLVE_CULTURE);
    assert(debug_console_execute(&console, "conversions", &state).action == DEBUG_CONVERSION_STATUS);
    assert(strstr(debug_console_execute(&console, "faith", &state).message,"Usage"));
    assert(strstr(debug_console_execute(&console, "invalid", &state).message, "Unknown"));
    assert(debug_console_execute(&console,"event 900",&state).action == DEBUG_FIRE_EVENT);
    assert(debug_console_execute(&console,"event WoL.100",&state).action == DEBUG_FIRE_EVENT);
    assert(debug_console_execute(&console,"eventchoose 2",&state).action == DEBUG_CHOOSE_EVENT);
    assert(debug_console_execute(&console,"eventclose",&state).action == DEBUG_CLOSE_EVENT);
    assert(strstr(debug_console_execute(&console,"event",&state).message,"Usage"));
    assert(debug_console_execute(&console, "savegame", &state).action == DEBUG_CHECKPOINT_SAVE);
    assert(debug_console_execute(&console, "savegame backup_1", &state).action == DEBUG_CHECKPOINT_SAVE);
    assert(debug_console_execute(&console, "loadgame backup_1", &state).action == DEBUG_CHECKPOINT_LOAD);
    assert(debug_console_execute(&console, "quicksave", &state).action == DEBUG_CHECKPOINT_SAVE);
    assert(debug_console_execute(&console, "quickload", &state).action == DEBUG_CHECKPOINT_LOAD);
    assert(debug_console_execute(&console, "government", &state).action == DEBUG_GOVERNMENT_STATUS);
    assert(debug_console_execute(&console, "gov 12345", &state).action == DEBUG_GOVERNMENT_STATUS);
    assert(debug_console_execute(&console, "clear", &state).action == DEBUG_CLEAR && console.count == 0);
    puts("technology/console core: all assertions passed");
    return 0;
}
