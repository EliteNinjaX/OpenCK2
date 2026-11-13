#include "../src/game/debug_console.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    struct debug_console console = {0};
    struct debug_console_context context = {.date = "1066.9.15"};
    const struct {const char *command; enum debug_console_action action;} cases[] = {
        {"birth", DEBUG_CHAR_BIRTH}, {"charborn", DEBUG_CHAR_BIRTH},
        {"die 10", DEBUG_CHAR_DEATH}, {"kill 10", DEBUG_CHAR_DEATH}, {"chardie 10", DEBUG_CHAR_DEATH},
        {"war 20", DEBUG_WAR_DECLARE}, {"declarewar 20", DEBUG_WAR_DECLARE},
        {"peace 20", DEBUG_WAR_PEACE},
        {"raise", DEBUG_WAR_RAISE}, {"raise 10", DEBUG_WAR_RAISE},
        {"raiseenemy 11", DEBUG_WAR_RAISE_ENEMY},
        {"march 1 11", DEBUG_WAR_MARCH}, {"disband 1", DEBUG_WAR_DISBAND},
        {"armies", DEBUG_WAR_LIST}, {"warstatus", DEBUG_WAR_LIST},
        {"election", DEBUG_ELECTION_STATUS}, {"electionstatus", DEBUG_ELECTION_STATUS},
        {"electioncandidate 123", DEBUG_ELECTION_CANDIDATE},
        {"byzcandidate 456", DEBUG_ELECTION_CANDIDATE}
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; ++i) {
        struct debug_console_result result = debug_console_execute(&console, cases[i].command, &context);
        assert(result.action == cases[i].action);
    }
    assert(debug_console_execute(&console, "die", &context).action == DEBUG_NO_ACTION);
    assert(debug_console_execute(&console, "march", &context).action == DEBUG_NO_ACTION);
    assert(debug_console_execute(&console, "electioncandidate", &context).action == DEBUG_NO_ACTION);
    puts("console warfare: all assertions passed");
    return 0;
}
