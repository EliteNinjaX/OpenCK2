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
    bool has_levy;
    double levy_potential;
};
enum debug_console_action { DEBUG_NO_ACTION, DEBUG_CLEAR, DEBUG_LIST_TECH, DEBUG_SET_MAP_MODE, DEBUG_FAMILY, DEBUG_MARRY, DEBUG_MARRY_MATRI, DEBUG_CONCEIVE, DEBUG_DIVORCE, DEBUG_SET_PROVINCE_RELIGION, DEBUG_SET_PROVINCE_CULTURE, DEBUG_SET_CHARACTER_RELIGION, DEBUG_SET_CHARACTER_CULTURE, DEBUG_MISSION_RELIGION, DEBUG_MISSION_CULTURE, DEBUG_RESOLVE_RELIGION, DEBUG_RESOLVE_CULTURE, DEBUG_CONVERSION_STATUS, DEBUG_FIRE_EVENT, DEBUG_CHOOSE_EVENT, DEBUG_CLOSE_EVENT, DEBUG_CHAR_BIRTH, DEBUG_CHAR_DEATH, DEBUG_WAR_DECLARE, DEBUG_WAR_PEACE, DEBUG_WAR_RAISE, DEBUG_WAR_RAISE_ENEMY, DEBUG_WAR_MARCH, DEBUG_WAR_DISBAND, DEBUG_WAR_LIST, DEBUG_LAW_LIST, DEBUG_LAW_SET, DEBUG_LAW_HEIR, DEBUG_LAW_TAXES, DEBUG_GOVERNMENT_STATUS, DEBUG_ELECTION_STATUS, DEBUG_ELECTION_CANDIDATE, DEBUG_CHECKPOINT_SAVE, DEBUG_CHECKPOINT_LOAD, DEBUG_GOV_CLANS, DEBUG_GOV_CLAN_REGISTER, DEBUG_GOV_CLAN_SPLIT, DEBUG_GOV_FAMILIES, DEBUG_GOV_FAMILY_REGISTER, DEBUG_GOV_FUND, DEBUG_GOV_DOGE_FORECAST, DEBUG_GOV_TRADEPOSTS, DEBUG_GOV_TRADE_BUILD, DEBUG_GOV_BISHOP_NOMINATE, DEBUG_GOV_BISHOP_APPROVE, DEBUG_GOV_BISHOP_LIST };
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
