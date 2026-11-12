#include "debug_console.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

void debug_console_output(struct debug_console *console, const char *line)
{
    if (!console || !line) return;
    if (console->count == OPENCK2_CONSOLE_LINES) {
        memmove(console->lines[0], console->lines[1],
                sizeof console->lines[0] * (OPENCK2_CONSOLE_LINES - 1));
        --console->count;
    }
    snprintf(console->lines[console->count++], sizeof console->lines[0], "%s", line);
}

struct debug_console_result debug_console_execute(struct debug_console *console, const char *line,
                                                   const struct debug_console_context *context)
{
    struct debug_console_result result = {0};
    if (!console || !line) return result;
    const char *p = line;
    while (isspace((unsigned char)*p)) ++p;
    char command[40] = {0};
    size_t k = 0;
    while (*p && !isspace((unsigned char)*p)) {
        if (k + 1 < sizeof command) command[k++] = (char)tolower((unsigned char)*p);
        ++p;
    }
    while (isspace((unsigned char)*p)) ++p;
    if (!command[0]) return result;
    snprintf(result.argument, sizeof result.argument, "%s", p);
    if (!strcmp(command, "help")) {
        snprintf(result.message, sizeof result.message,
                 "help, clear, date, province, ruler, gold, prestige, piety, tech [KEY], mapmode [NAME]");
    } else if (!strcmp(command, "clear")) {
        result.action = DEBUG_CLEAR;
        console->count = 0;
        return result;
    } else if (!strcmp(command, "date")) {
        snprintf(result.message, sizeof result.message, "Date: %s", context && context->date ? context->date : "unknown");
    } else if (!strcmp(command, "province")) {
        snprintf(result.message, sizeof result.message, "Province: %s", context && context->has_province && context->province ? context->province : "none selected");
    } else if (!strcmp(command, "ruler")) {
        snprintf(result.message, sizeof result.message, "Ruler: %s", context && context->ruler ? context->ruler : "unknown");
    } else if (!strcmp(command, "gold") || !strcmp(command, "prestige") || !strcmp(command, "piety")) {
        const double amount = context ? !strcmp(command, "gold") ? context->gold :
            !strcmp(command, "prestige") ? context->prestige : context->piety : 0.0;
        snprintf(result.message, sizeof result.message, "%s: %.2f%s", command, amount,
                 context ? "" : " (no game state)");
    } else if (!strcmp(command, "tech")) {
        result.action = DEBUG_LIST_TECH;
    } else if (!strcmp(command, "mapmode")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: mapmode terrain|realms");
        else result.action = DEBUG_SET_MAP_MODE;
    } else {
        snprintf(result.message, sizeof result.message, "Unknown command: %s (type help)", command);
    }
    if (result.message[0]) debug_console_output(console, result.message);
    return result;
}
