#include "conversion_core.h"
#include "ck2_lexer_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static bool valid_key(const char *p)
{
    if (!p || !*p || strlen(p) >= 64) return false;
    for (; *p; ++p)
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-') return false;
    return true;
}

static bool names_add(struct conversion_names *list, const char *value)
{
    if (!valid_key(value)) return false;
    for (size_t i = 0; i < list->count; ++i)
        if (!strcmp(value, list->items[i])) return true;
    if (list->count == list->capacity) {
        const size_t next = list->capacity ? list->capacity * 2 : 32;
        if (next > 65536 || next < list->capacity) return false;
        void *items = realloc(list->items, next * sizeof *list->items);
        if (!items) return false;
        list->items = items;
        list->capacity = next;
    }
    snprintf(list->items[list->count++], 64, "%s", value);
    return true;
}

bool conversion_catalog_has(const struct conversion_catalog *catalog,
                            enum conversion_kind kind, const char *key)
{
    if (!catalog || (unsigned)kind > CONVERSION_CULTURE || !valid_key(key)) return false;
    const struct conversion_names *names = &catalog->names[kind];
    for (size_t i = 0; i < names->count; ++i)
        if (!strcmp(names->items[i], key)) return true;
    return false;
}

static bool is_metadata(struct ck2_span name)
{
    static const char *const skip[] = {
        "color", "graphical_cultures", "graphical_culture", "male_names",
        "female_names", "dynasty_title_names", "alternate_start", "portraits",
        "unit_graphical_cultures", "unit_graphical_culture", "modifier",
        "character_modifier", "province_modifier", "trigger", "potential",
        "allow", "ai_will_do", "religious_clothing_head", "religious_clothing_priest"
    };
    for (size_t i = 0; i < sizeof skip / sizeof *skip; ++i)
        if (ck2_span_eq(name, skip[i])) return true;
    return false;
}

bool conversion_catalog_parse(struct conversion_catalog *catalog,
                              enum conversion_kind kind, const char *text)
{
    if (!catalog || (unsigned)kind > CONVERSION_CULTURE || !text) return false;
    struct ck2_lexer parser = {text};
    struct ck2_span group;
    while ((group = ck2_lex(&parser)).length) {
        /* Some CK2 culture packs contain an extra closing delimiter at EOF. */
        if (ck2_span_eq(group, "}")) continue;
        if (!ck2_span_eq(ck2_lex(&parser), "=")) return false;
        if (!ck2_span_eq(ck2_lex(&parser), "{")) continue;
        struct ck2_span key;
        while ((key = ck2_lex(&parser)).length && !ck2_span_eq(key, "}")) {
            if (!ck2_span_eq(ck2_lex(&parser), "=")) return false;
            struct ck2_span value = ck2_lex(&parser);
            if (!value.length) return false;
            if (!ck2_span_eq(value, "{")) continue;
            if (!is_metadata(key)) {
                char name[64];
                ck2_span_copy(name, sizeof name, key);
                if (!names_add(&catalog->names[kind], name)) return false;
            }
            ck2_skip_scope(&parser);
        }
        if (!key.length) return false;
    }
    return true;
}

/* Only identifies council actions and their event references; it does not
 * claim the event chains have been loaded or can execute. */
bool conversion_catalog_jobs(struct conversion_catalog *catalog, const char *data)
{
    if (!catalog || !data) return false;
    struct ck2_lexer parser = {data};
    struct ck2_span key;
    while ((key = ck2_lex(&parser)).length) {
        if (!ck2_span_eq(ck2_lex(&parser), "=")) return false;
        if (!ck2_span_eq(ck2_lex(&parser), "{")) continue;
        const bool spiritual = ck2_span_eq(key, "action_inquisition");
        const bool settle = ck2_span_eq(key, "action_settle_tribe");
        if (spiritual) catalog->inquisition_defined = true;
        if (settle) catalog->settle_tribe_defined = true;
        if (!spiritual && !settle) { ck2_skip_scope(&parser); continue; }
        struct ck2_span field;
        while ((field = ck2_lex(&parser)).length && !ck2_span_eq(field, "}")) {
            if (!ck2_span_eq(ck2_lex(&parser), "=")) return false;
            struct ck2_span value = ck2_lex(&parser);
            if (ck2_span_eq(value, "{") && ck2_span_eq(field, "events")) {
                char *dst = spiritual ? catalog->inquisition_events : catalog->settle_tribe_events;
                size_t off = 0;
                struct ck2_span event;
                while ((event = ck2_lex(&parser)).length && !ck2_span_eq(event, "}")) {
                    if (off && off + 1 < 80) dst[off++] = ' ';
                    const size_t room = 79 - off;
                    const size_t n = event.length < room ? event.length : room;
                    if (n) memcpy(dst + off, event.start, n);
                    off += n; dst[off] = 0;
                }
                if (!event.length) return false;
            } else if (ck2_span_eq(value, "{")) ck2_skip_scope(&parser);
            else if (!value.length) return false;
        }
        if (!field.length) return false;
    }
    return true;
}

void conversion_catalog_free(struct conversion_catalog *catalog)
{
    if (!catalog) return;
    for (size_t i = 0; i < 2; ++i) free(catalog->names[i].items);
    *catalog = (struct conversion_catalog){0};
}
void conversion_state_free(struct conversion_state *state)
{
    if (!state) return;
    free(state->provinces);
    free(state->people);
    *state = (struct conversion_state){0};
}

static bool more(void **data, size_t *capacity, size_t count, size_t size)
{
    if (count < *capacity) return true;
    size_t next = *capacity ? *capacity * 2 : 8;
    if (next > 100000 || next < *capacity) return false;
    void *memory = realloc(*data, next * size);
    if (!memory) return false;
    *data = memory;
    *capacity = next;
    return true;
}

static struct conversion_province *province_entry(struct conversion_state *s, unsigned id)
{
    for (size_t i = 0; i < s->province_count; ++i)
        if (s->provinces[i].id == id) return s->provinces + i;
    if (!more((void **)&s->provinces, &s->province_capacity,
              s->province_count, sizeof *s->provinces)) return NULL;
    struct conversion_province *e = s->provinces + s->province_count++;
    *e = (struct conversion_province){.id = id};
    return e;
}
static struct conversion_person *person_entry(struct conversion_state *s, unsigned id)
{
    for (size_t i = 0; i < s->person_count; ++i)
        if (s->people[i].id == id) return s->people + i;
    if (!more((void **)&s->people, &s->person_capacity,
              s->person_count, sizeof *s->people)) return NULL;
    struct conversion_person *e = s->people + s->person_count++;
    *e = (struct conversion_person){.id = id};
    return e;
}
const char *conversion_province_value(const struct conversion_state *s,
                                      unsigned id, enum conversion_kind kind,
                                      const char *historical)
{
    if (s && (unsigned)kind <= CONVERSION_CULTURE)
        for (size_t i = 0; i < s->province_count; ++i)
            if (s->provinces[i].id == id) {
                const struct conversion_province *e = s->provinces + i;
                if (kind == CONVERSION_RELIGION && e->has_religion) return e->religion;
                if (kind == CONVERSION_CULTURE && e->has_culture) return e->culture;
            }
    return historical ? historical : "";
}
const char *conversion_person_value(const struct conversion_state *s,
                                    unsigned id, enum conversion_kind kind,
                                    const char *historical)
{
    if (s && (unsigned)kind <= CONVERSION_CULTURE)
        for (size_t i = 0; i < s->person_count; ++i)
            if (s->people[i].id == id) {
                const struct conversion_person *e = s->people + i;
                if (kind == CONVERSION_RELIGION && e->has_religion) return e->religion;
                if (kind == CONVERSION_CULTURE && e->has_culture) return e->culture;
            }
    return historical ? historical : "";
}

enum conversion_status conversion_province_set(struct conversion_state *s,
    const struct conversion_catalog *catalog, unsigned id, enum conversion_kind kind,
    const char *key, const char *current)
{
    if (!s || !id || (unsigned)kind > CONVERSION_CULTURE) return CONVERSION_INVALID_TARGET;
    if (!conversion_catalog_has(catalog, kind, key)) return CONVERSION_UNKNOWN_KEY;
    if (!strcmp(conversion_province_value(s,id,kind,current),key)) return CONVERSION_ALREADY_SAME;
    struct conversion_province *e = province_entry(s,id);
    if (!e) return CONVERSION_NO_MEMORY;
    if (kind == CONVERSION_RELIGION) { snprintf(e->religion,sizeof e->religion,"%s",key); e->has_religion=true; }
    else { snprintf(e->culture,sizeof e->culture,"%s",key); e->has_culture=true; }
    ++s->revision;
    return CONVERSION_OK;
}
enum conversion_status conversion_person_set(struct conversion_state *s,
    const struct conversion_catalog *catalog, unsigned id, enum conversion_kind kind,
    const char *key, const char *current)
{
    if (!s || !id || (unsigned)kind > CONVERSION_CULTURE) return CONVERSION_INVALID_TARGET;
    if (!conversion_catalog_has(catalog, kind, key)) return CONVERSION_UNKNOWN_KEY;
    if (!strcmp(conversion_person_value(s,id,kind,current),key)) return CONVERSION_ALREADY_SAME;
    struct conversion_person *e = person_entry(s,id);
    if (!e) return CONVERSION_NO_MEMORY;
    if (kind == CONVERSION_RELIGION) { snprintf(e->religion,sizeof e->religion,"%s",key); e->has_religion=true; }
    else { snprintf(e->culture,sizeof e->culture,"%s",key); e->has_culture=true; }
    ++s->revision;
    return CONVERSION_OK;
}

enum conversion_status conversion_mission_assign(struct conversion_state *s,
    const struct conversion_catalog *catalog, enum conversion_kind kind,
    unsigned id, unsigned actor, const char *target, const char *current)
{
    if (!s || !id || !actor || (unsigned)kind > CONVERSION_CULTURE) return CONVERSION_INVALID_TARGET;
    if (!catalog || !(kind == CONVERSION_RELIGION ? catalog->inquisition_defined : catalog->settle_tribe_defined))
        return CONVERSION_UNAVAILABLE;
    if (!conversion_catalog_has(catalog,kind,target)) return CONVERSION_UNKNOWN_KEY;
    if (!strcmp(conversion_province_value(s,id,kind,current),target)) return CONVERSION_ALREADY_SAME;
    struct conversion_mission *m = &s->missions[kind];
    *m = (struct conversion_mission){.kind=kind,.province_id=id,.actor_id=actor,.active=true};
    snprintf(m->target,sizeof m->target,"%s",target);
    return CONVERSION_OK;
}
enum conversion_status conversion_mission_complete(struct conversion_state *s,
    const struct conversion_catalog *catalog, enum conversion_kind kind, const char *current)
{
    if (!s || (unsigned)kind > CONVERSION_CULTURE) return CONVERSION_INVALID_TARGET;
    struct conversion_mission *m = &s->missions[kind];
    if (!m->active) return CONVERSION_NO_MISSION;
    enum conversion_status status = conversion_province_set(s,catalog,m->province_id,kind,m->target,current);
    if (status == CONVERSION_OK || status == CONVERSION_ALREADY_SAME) m->active=false;
    return status;
}
const char *conversion_status_name(enum conversion_status s)
{
    switch (s) {
    case CONVERSION_OK: return "Applied to runtime campaign (not the original CK2 history).";
    case CONVERSION_UNKNOWN_KEY: return "Unknown culture/religion key in loaded CK2 definitions.";
    case CONVERSION_INVALID_TARGET: return "Invalid province or character target.";
    case CONVERSION_UNAVAILABLE: return "Council action unavailable from loaded CK2 definitions.";
    case CONVERSION_ALREADY_SAME: return "Already has that culture/religion.";
    case CONVERSION_NO_MEMORY: return "Cannot allocate conversion record.";
    case CONVERSION_NO_MISSION: return "No council conversion mission assigned.";
    }
    return "Unknown conversion status.";
}
