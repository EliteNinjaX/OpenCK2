#include "tree_history.h"
#include "ck2_lexer_core.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int date_of(struct ck2_span key)
{
    if (key.length < 7 || key.length > 12) return 0;
    char buffer[16], extra;
    int year, month, day;
    ck2_span_copy(buffer, sizeof buffer, key);
    return sscanf(buffer, "%d.%d.%d%c", &year, &month, &day, &extra) == 3 &&
        year > 0 && month > 0 && month <= 12 && day > 0 && day <= 31 ?
        year * 10000 + month * 100 + day : 0;
}

static unsigned unsigned_value(struct ck2_span span)
{
    char buffer[24], *end;
    if (!span.length || span.length > 20) return 0;
    ck2_span_copy(buffer, sizeof buffer, span);
    unsigned long n = strtoul(buffer, &end, 10);
    return !*end && n <= 0xffffffffUL ? (unsigned)n : 0;
}

static void read_fields(struct ck2_lexer *lexer, struct tree_person *person,
                        int when, int date, int *dynasty_when)
{
    for (;;) {
        struct ck2_span key = ck2_lex(lexer);
        if (!key.length || ck2_span_eq(key, "}")) return;
        if (!ck2_span_eq(ck2_lex(lexer), "=")) return;
        struct ck2_span value = ck2_lex(lexer);
        if (ck2_span_eq(value, "{")) {
            int event_date = date_of(key);
            if (event_date)
                read_fields(lexer, person, event_date, date, dynasty_when);
            else
                ck2_skip_scope(lexer);
            continue;
        }
        if (ck2_span_eq(key, "birth") && when) {
            if (!person->birth || when < person->birth) person->birth = when;
            continue;
        }
        if (ck2_span_eq(key, "death") && when) { person->death = when; continue; }
        if (when > date) continue;
        if (ck2_span_eq(key, "dynasty") && when >= *dynasty_when) {
            person->dynasty = unsigned_value(value);
            *dynasty_when = when;
        } else if (ck2_span_eq(key, "father")) person->father = unsigned_value(value);
        else if (ck2_span_eq(key, "mother")) person->mother = unsigned_value(value);
        else if (ck2_span_eq(key, "name") && !person->name[0])
            ck2_span_copy(person->name, sizeof person->name, value);
        else if (ck2_span_eq(key, "birth") && when) person->birth = when;
        else if (ck2_span_eq(key, "death") && when) person->death = when;
    }
}

bool tree_history_add_file(struct tree_history *history, const char *content, int date)
{
    if (!content) return false;
    struct ck2_lexer lexer = {.next = content};
    for (;;) {
        struct ck2_span id = ck2_lex(&lexer);
        if (!id.length) break;
        if (!ck2_span_eq(ck2_lex(&lexer), "=")) break;
        if (!ck2_span_eq(ck2_lex(&lexer), "{")) break;
        struct tree_person person = {.id = unsigned_value(id)};
        int dynasty_when = 0;
        read_fields(&lexer, &person, 0, date, &dynasty_when);
        if (!person.id || (person.birth && person.birth > date)) continue;
        if (history->count == history->capacity) {
            size_t next = history->capacity ? history->capacity * 2 : 2048;
            if (next < history->capacity || next > SIZE_MAX / sizeof *history->people) return false;
            struct tree_person *people = realloc(history->people, next * sizeof *people);
            if (!people) return false;
            history->people = people;
            history->capacity = next;
        }
        history->people[history->count++] = person;
    }
    return true;
}

void tree_history_free(struct tree_history *history)
{
    free(history->people);
    *history = (struct tree_history){0};
}

const struct tree_person *tree_history_find(const struct tree_history *history, unsigned id)
{
    if (!history || !id) return NULL;
    for (size_t i = 0; i < history->count; ++i)
        if (history->people[i].id == id) return &history->people[i];
    return NULL;
}

size_t tree_history_dynasty_count(const struct tree_history *history, unsigned dynasty, bool living, int date)
{
    size_t count = 0;
    if (!history || !dynasty) return 0;
    for (size_t i = 0; i < history->count; ++i) {
        const struct tree_person *p = &history->people[i];
        if (p->dynasty == dynasty && (!living || (!p->death || p->death > date))) ++count;
    }
    return count;
}
