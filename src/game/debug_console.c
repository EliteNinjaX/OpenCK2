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
                 "help, savegame, mapmode, government, clans, clanregister, patricians, dogeforecast, tradebuild, bishoplist, bishopnominate");
    } else if (!strcmp(command,"clans")) { result.action=DEBUG_GOV_CLANS;
    } else if (!strcmp(command,"clanregister")) { result.action=DEBUG_GOV_CLAN_REGISTER;
    } else if (!strcmp(command,"clansplit")) { result.action=DEBUG_GOV_CLAN_SPLIT;
    } else if (!strcmp(command,"patricians")) { result.action=DEBUG_GOV_FAMILIES;
    } else if (!strcmp(command,"patricianadd")) { result.action=DEBUG_GOV_FAMILY_REGISTER;
    } else if (!strcmp(command,"campaignfund")) { result.action=DEBUG_GOV_FUND;
    } else if (!strcmp(command,"dogeforecast")) { result.action=DEBUG_GOV_DOGE_FORECAST;
    } else if (!strcmp(command,"tradeposts")) { result.action=DEBUG_GOV_TRADEPOSTS;
    } else if (!strcmp(command,"tradebuild")) { result.action=DEBUG_GOV_TRADE_BUILD;
    } else if (!strcmp(command,"bishopnominate")) { result.action=DEBUG_GOV_BISHOP_NOMINATE;
    } else if (!strcmp(command,"bishopapprove")) { result.action=DEBUG_GOV_BISHOP_APPROVE;
    } else if (!strcmp(command,"bishoplist")) { result.action=DEBUG_GOV_BISHOP_LIST;
    } else if (!strcmp(command, "savegame") || !strcmp(command, "quicksave")) {
        result.action = DEBUG_CHECKPOINT_SAVE;
    } else if (!strcmp(command, "loadgame") || !strcmp(command, "quickload")) {
        result.action = DEBUG_CHECKPOINT_LOAD;
    } else if (!strcmp(command,"election") || !strcmp(command,"electionstatus")) {
        result.action=DEBUG_ELECTION_STATUS;
    } else if (!strcmp(command,"electioncandidate") || !strcmp(command,"byzcandidate")) {
        if (!*p) snprintf(result.message,sizeof result.message,"Usage: electioncandidate <character ID>");
        else result.action=DEBUG_ELECTION_CANDIDATE;
    } else if (!strcmp(command,"government") || !strcmp(command,"gov")) {
        result.action=DEBUG_GOVERNMENT_STATUS;
    } else if (!strcmp(command,"laws") || !strcmp(command,"lawlist")) {
        result.action=DEBUG_LAW_LIST;
    } else if (!strcmp(command,"setlaw") || !strcmp(command,"law")) {
        if (!*p) snprintf(result.message,sizeof result.message,"Usage: setlaw <CK2 law ID>");
        else result.action=DEBUG_LAW_SET;
    } else if (!strcmp(command,"heir") || !strcmp(command,"succession")) {
        result.action=DEBUG_LAW_HEIR;
    } else if (!strcmp(command,"taxes") || !strcmp(command,"taxbreakdown")) {
        result.action=DEBUG_LAW_TAXES;
    } else if (!strcmp(command, "birth") || !strcmp(command, "charborn")) {
        result.action = DEBUG_CHAR_BIRTH;
    } else if (!strcmp(command, "die") || !strcmp(command, "chardie") || !strcmp(command, "kill")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: die <character ID>");
        else result.action = DEBUG_CHAR_DEATH;
    } else if (!strcmp(command, "declarewar") || !strcmp(command, "war")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: declarewar <enemy ruler ID>");
        else result.action = DEBUG_WAR_DECLARE;
    } else if (!strcmp(command, "peace")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: peace <enemy ruler ID>");
        else result.action = DEBUG_WAR_PEACE;
    } else if (!strcmp(command, "raise")) {
        result.action = DEBUG_WAR_RAISE;
    } else if (!strcmp(command, "raiseenemy")) {
        result.action = DEBUG_WAR_RAISE_ENEMY;
    } else if (!strcmp(command, "march")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: march <army ID> <province ID>");
        else result.action = DEBUG_WAR_MARCH;
    } else if (!strcmp(command, "disband")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: disband <army ID>");
        else result.action = DEBUG_WAR_DISBAND;
    } else if (!strcmp(command, "armies") || !strcmp(command, "warstatus")) {
        result.action = DEBUG_WAR_LIST;
    } else if (!strcmp(command, "event")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: event <CK2 event ID> (for current ruler or selected province)");
        else result.action = DEBUG_FIRE_EVENT;
    } else if (!strcmp(command, "eventchoose") || !strcmp(command, "eventchoice")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: eventchoose <1-8>");
        else result.action = DEBUG_CHOOSE_EVENT;
    } else if (!strcmp(command, "eventclose")) {
        result.action = DEBUG_CLOSE_EVENT;
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
    } else if (!strcmp(command, "levy")) {
        snprintf(result.message, sizeof result.message, context && context->has_levy ?
                 "Historical unraised levy potential: %.0f (not current troops)" :
                 "Select a province with authored levy data first.",
                 context ? context->levy_potential : 0.);
    } else if (!strcmp(command, "tech")) {
        result.action = DEBUG_LIST_TECH;
    } else if (!strcmp(command, "mapmode")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: mapmode <original map-mode name>");
        else result.action = DEBUG_SET_MAP_MODE;
    } else if (!strcmp(command, "faith") || !strcmp(command, "culture") ||
               !strcmp(command, "charfaith") || !strcmp(command, "charculture")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: %s <CK2 key>", command);
        else result.action = !strcmp(command,"faith") ? DEBUG_SET_PROVINCE_RELIGION :
            !strcmp(command,"culture") ? DEBUG_SET_PROVINCE_CULTURE :
            !strcmp(command,"charfaith") ? DEBUG_SET_CHARACTER_RELIGION : DEBUG_SET_CHARACTER_CULTURE;
    } else if (!strcmp(command, "missionfaith")) {
        result.action = DEBUG_MISSION_RELIGION;
    } else if (!strcmp(command, "missionculture")) {
        result.action = DEBUG_MISSION_CULTURE;
    } else if (!strcmp(command, "resolvefaith")) {
        result.action = DEBUG_RESOLVE_RELIGION;
    } else if (!strcmp(command, "resolveculture")) {
        result.action = DEBUG_RESOLVE_CULTURE;
    } else if (!strcmp(command, "conversions")) {
        result.action = DEBUG_CONVERSION_STATUS;
    } else if (!strcmp(command, "family")) {
        result.action = DEBUG_FAMILY;
    } else if (!strcmp(command, "marry")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: marry <character ID>");
        else result.action = DEBUG_MARRY;
    } else if (!strcmp(command, "marrymatri")) {
        if (!*p) snprintf(result.message, sizeof result.message, "Usage: marrymatri <character ID>");
        else result.action = DEBUG_MARRY_MATRI;
    } else if (!strcmp(command, "conceive")) {
        result.action = DEBUG_CONCEIVE;
    } else if (!strcmp(command, "divorce")) {
        result.action = DEBUG_DIVORCE;
    } else {
        snprintf(result.message, sizeof result.message, "Unknown command: %s (type help)", command);
    }
    if (result.message[0]) debug_console_output(console, result.message);
    return result;
}
