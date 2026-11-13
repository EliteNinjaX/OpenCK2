#include "event_core.h"
#include "ck2_lexer_core.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool ck2_event_valid_id(const char *id)
{
    if (!id || !*id || strlen(id) >= 96) return false;
    for (const unsigned char *p = (const unsigned char *)id; *p; ++p)
        if (!isalnum(*p) && *p != '_' && *p != '.' && *p != '-') return false;
    return true;
}
static bool accept(struct ck2_span span, const char *s) { return ck2_span_eq(span,s); }
static void copy(char *dst, size_t capacity, struct ck2_span text)
{ ck2_span_copy(dst,capacity,text); }
static bool read_assignment(struct ck2_lexer *lx, struct ck2_span *value)
{
    if (!accept(ck2_lex(lx),"=")) return false;
    *value = ck2_lex(lx);
    return value->length != 0;
}
static bool double_value(struct ck2_span span, double *out)
{
    char number[80];
    if (!span.length || span.length >= sizeof number) return false;
    copy(number,sizeof number,span);
    errno = 0;
    char *end = NULL;
    const double value = strtod(number,&end);
    if (errno || !end || *end || !isfinite(value) || fabs(value) > 100000000.) return false;
    *out = value;
    return true;
}
static bool is_option_metadata(struct ck2_span field)
{
    return accept(field,"name") || accept(field,"ai_chance") ||
        accept(field,"trigger") || accept(field,"tooltip") ||
        accept(field,"custom_tooltip") || accept(field,"highlight") ||
        accept(field,"show_only_if_valid");
}
static bool parse_option(struct ck2_lexer *lx, struct ck2_event_option *option)
{
    struct ck2_span field;
    while ((field = ck2_lex(lx)).length) {
        if (accept(field,"}")) return true;
        struct ck2_span value;
        if (!read_assignment(lx,&value)) return false;
        if (accept(field,"name") && !accept(value,"{")) {
            copy(option->name,sizeof option->name,value);
            continue;
        }
        if (accept(value,"{")) {
            ck2_skip_scope(lx);
            if (!is_option_metadata(field)) ++option->ignored_effects;
            else if (accept(field,"trigger")) option->conditional = true;
            continue;
        }
        enum ck2_event_effect_kind kind;
        bool numeric = true;
        if (accept(field,"wealth")) kind = CK2_EVENT_WEALTH;
        else if (accept(field,"prestige")) kind = CK2_EVENT_PRESTIGE;
        else if (accept(field,"piety")) kind = CK2_EVENT_PIETY;
        else if (accept(field,"religion")) {kind=CK2_EVENT_RELIGION; numeric=false;}
        else if (accept(field,"culture")) {kind=CK2_EVENT_CULTURE; numeric=false;}
        else { if (!is_option_metadata(field)) ++option->ignored_effects; continue; }
        struct ck2_event_effect effect = {.kind=kind};
        if (numeric) {
            if (!double_value(value,&effect.amount)) {++option->ignored_effects; continue;}
        } else {
            copy(effect.value,sizeof effect.value,value);
            if (!effect.value[0]) {++option->ignored_effects; continue;}
        }
        if (option->effect_count >= CK2_EVENT_EFFECTS_MAX) ++option->ignored_effects;
        else option->effects[option->effect_count++] = effect;
    }
    return false;
}
static void default_localization(struct ck2_event *event)
{
    if (!event->title[0]) snprintf(event->title,sizeof event->title,"EVTNAME%.110s",event->id);
    if (!event->description[0]) snprintf(event->description,sizeof event->description,"EVTDESC%.110s",event->id);
}
static bool parse_event(struct ck2_lexer *lx, enum ck2_event_kind kind,
                        const char *id, const char *source, struct ck2_event *out)
{
    struct ck2_event event={.kind=kind};
    struct ck2_span field;
    while ((field=ck2_lex(lx)).length) {
        if (accept(field,"}")) {
            if (!strcmp(event.id,id)) {
                snprintf(event.source,sizeof event.source,"%s",source ? source : "events");
                default_localization(&event);
                *out=event;
                return true;
            }
            return false;
        }
        struct ck2_span value;
        if (!read_assignment(lx,&value)) return false;
        if (accept(field,"id") && !accept(value,"{")) copy(event.id,sizeof event.id,value);
        else if (accept(field,"title") && !accept(value,"{")) copy(event.title,sizeof event.title,value);
        else if (accept(field,"picture") && !accept(value,"{")) copy(event.picture,sizeof event.picture,value);
        else if (accept(field,"desc") && !accept(value,"{")) {
            if (!event.description[0]) copy(event.description,sizeof event.description,value);
        } else if (accept(field,"option") && accept(value,"{")) {
            if (event.option_count < CK2_EVENT_OPTIONS_MAX) {
                if (!parse_option(lx,&event.options[event.option_count])) return false;
                ++event.option_count;
            } else {ck2_skip_scope(lx);event.truncated_options=true;}
        } else if (accept(field,"trigger")) {
            event.has_trigger=true;
            if (accept(value,"{")) ck2_skip_scope(lx);
        } else if (accept(field,"immediate")) {
            event.has_immediate=true;
            if (accept(value,"{")) ck2_skip_scope(lx);
        } else if (accept(field,"desc") && accept(value,"{")) {
            event.conditional_description=true;
            struct ck2_span k;
            while ((k=ck2_lex(lx)).length && !accept(k,"}")) {
                struct ck2_span v;
                if (!read_assignment(lx,&v)) return false;
                if (accept(k,"text") && !accept(v,"{") && !event.description[0])
                    copy(event.description,sizeof event.description,v);
                if (accept(v,"{")) ck2_skip_scope(lx);
            }
        } else if (accept(value,"{")) ck2_skip_scope(lx);
    }
    return false;
}
int ck2_event_find_in_text(const char *script, const char *id,
                           const char *source_name, struct ck2_event *out)
{
    if (!script || !out || !ck2_event_valid_id(id)) return -1;
    struct ck2_lexer lx={script};
    struct ck2_span field;
    while ((field=ck2_lex(&lx)).length) {
        struct ck2_span value;
        if (!read_assignment(&lx,&value)) return 0;
        enum ck2_event_kind kind;
        if (accept(field,"character_event") || accept(field,"narrative_event") ||
            accept(field,"letter_event") || accept(field,"long_character_event"))
            kind=CK2_EVENT_CHARACTER;
        else if (accept(field,"province_event")) kind=CK2_EVENT_PROVINCE;
        else { if (accept(value,"{")) ck2_skip_scope(&lx); continue; }
        if (!accept(value,"{")) continue;
        if (parse_event(&lx,kind,id,source_name,out)) return 1;
    }
    return 0;
}
struct ck2_event_deltas ck2_event_numeric_deltas(const struct ck2_event_option *option)
{
    struct ck2_event_deltas result={0};
    if (!option) return result;
    for (size_t i=0;i<option->effect_count;i++) {
        const struct ck2_event_effect *e=&option->effects[i];
        if (e->kind==CK2_EVENT_WEALTH) result.wealth+=e->amount;
        else if (e->kind==CK2_EVENT_PRESTIGE) result.prestige+=e->amount;
        else if (e->kind==CK2_EVENT_PIETY) result.piety+=e->amount;
    }
    return result;
}
