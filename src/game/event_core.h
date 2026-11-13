#ifndef OPENCK2_EVENT_CORE_H
#define OPENCK2_EVENT_CORE_H

#include <stdbool.h>
#include <stddef.h>

/* A bounded CK2 event viewer and deliberately limited direct-effect runner.
 * Source, title and option strings are copied, so the loaded script can be freed.
 * Trigger/MTTH, nested scopes and delayed event chains are not evaluated. */
enum { CK2_EVENT_OPTIONS_MAX = 8, CK2_EVENT_EFFECTS_MAX = 16 };
enum ck2_event_kind { CK2_EVENT_CHARACTER, CK2_EVENT_PROVINCE };
enum ck2_event_effect_kind {
    CK2_EVENT_WEALTH, CK2_EVENT_PRESTIGE, CK2_EVENT_PIETY,
    CK2_EVENT_RELIGION, CK2_EVENT_CULTURE
};
struct ck2_event_effect {
    enum ck2_event_effect_kind kind;
    double amount;
    char value[64];
};
struct ck2_event_option {
    char name[128];
    struct ck2_event_effect effects[CK2_EVENT_EFFECTS_MAX];
    size_t effect_count;
    unsigned ignored_effects;
    bool conditional; /* trigger / nested script rules skipped */
};
struct ck2_event {
    char id[96], title[128], description[128], picture[128], source[192];
    enum ck2_event_kind kind;
    struct ck2_event_option options[CK2_EVENT_OPTIONS_MAX];
    size_t option_count;
    bool has_trigger, has_immediate, conditional_description, truncated_options;
};
/* 1 = found; 0 = absent; -1 = bad input/unsupported invalid event ID. */
int ck2_event_find_in_text(const char *script, const char *id,
                           const char *source_name, struct ck2_event *out);
bool ck2_event_valid_id(const char *id);

/* Returns number of supported direct effects for one option, or -1 if invalid.
 * Unsupported scripting remains explicitly recorded in ignored_effects. */
struct ck2_event_deltas { double wealth, prestige, piety; };
struct ck2_event_deltas ck2_event_numeric_deltas(const struct ck2_event_option *option);

#endif
