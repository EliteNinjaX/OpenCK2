#ifndef OPENCK2_DEBUG_CONSOLE_H
#define OPENCK2_DEBUG_CONSOLE_H
#include <stdbool.h>
#include <stddef.h>

enum { OPENCK2_CONSOLE_INPUT = 127, OPENCK2_CONSOLE_LINES = 11 };
struct debug_console {
    bool open;
    char input[OPENCK2_CONSOLE_INPUT + 1];
    char lines[OPENCK2_CONSOLE_LINES][220];
    unsigned count;
};
struct debug_console_context {
    const char *date, *province, *ruler;
    double gold, prestige, piety;
    bool has_province;
};
enum debug_console_action { DEBUG_NO_ACTION, DEBUG_CLEAR, DEBUG_LIST_TECH, DEBUG_SET_MAP_MODE };
struct debug_console_result {
    enum debug_console_action action;
    char argument[96];
    char message[220];
};
void debug_console_output(struct debug_console *, const char *line);
/* Whitelisted read-only commands. No shell/Lua execution and no fake CK2 effects. */
struct debug_console_result debug_console_execute(struct debug_console *, const char *line,
                                                   const struct debug_console_context *);
#endif
