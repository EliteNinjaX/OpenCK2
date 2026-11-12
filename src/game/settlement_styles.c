#include "settlement_styles.h"
#include "ck2_lexer_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void first_graphical(struct ck2_lexer *l, char *out, size_t n)
{
    struct ck2_span t;
    while ((t = ck2_lex(l)).length && !ck2_span_eq(t, "}"))
        if (!out[0]) ck2_span_copy(out, n, t);
}

static uint32_t culture_color(struct ck2_lexer *l, bool *valid)
{
    unsigned rgb[3] = {0};
    *valid = true;
    for (int i = 0; i < 3; ++i) {
        struct ck2_span t = ck2_lex(l);
        char value[48], *end;
        ck2_span_copy(value, sizeof value, t);
        const double d = strtod(value, &end);
        if (!t.length || *end || d < 0 || d > 255.0) *valid = false;
        const double scaled = d <= 1.0 ? d * 255.0 : d;
        rgb[i] = (unsigned)(scaled + 0.5);
    }
    ck2_skip_scope(l);
    return (rgb[0] << 16) | (rgb[1] << 8) | rgb[2];
}

static bool culture_field(struct ck2_lexer *l, struct settlement_culture *c)
{
    struct ck2_span field;
    while ((field = ck2_lex(l)).length && !ck2_span_eq(field, "}")) {
        if (!ck2_span_eq(ck2_lex(l), "=")) return false;
        struct ck2_span v = ck2_lex(l);
        if (!v.start) return false;
        if (ck2_span_eq(v, "{")) {
            if (ck2_span_eq(field, "graphical_cultures")) {
                c->graphics[0] = 0;
                first_graphical(l, c->graphics, sizeof c->graphics);
            } else if (ck2_span_eq(field, "color")) c->rgb = culture_color(l, &c->has_color);
            else ck2_skip_scope(l);
        }
    }
    return ck2_span_eq(field, "}");
}

bool settlement_styles_parse_cultures(struct settlement_styles *styles, const char *source)
{
    if (!styles || !source) return false;
    struct ck2_lexer l = {source};
    struct ck2_span group;
    while ((group = ck2_lex(&l)).length) {
        if (!ck2_span_eq(ck2_lex(&l), "=") || !ck2_span_eq(ck2_lex(&l), "{")) return false;
        char base_gfx[64] = {0};
        struct ck2_span name;
        while ((name = ck2_lex(&l)).length && !ck2_span_eq(name, "}")) {
            if (!ck2_span_eq(ck2_lex(&l), "=")) return false;
            struct ck2_span value = ck2_lex(&l);
            if (!value.length) return false;
            if (!ck2_span_eq(value, "{")) continue;
            if (ck2_span_eq(name, "graphical_cultures")) {
                first_graphical(&l, base_gfx, sizeof base_gfx);
                continue;
            }
            if (name.length >= 72 || group.length >= 72) { ck2_skip_scope(&l); continue; }
            struct settlement_culture next = {0};
            ck2_span_copy(next.key, sizeof next.key, name);
            ck2_span_copy(next.group, sizeof next.group, group);
            memcpy(next.graphics, base_gfx, sizeof next.graphics);
            if (!culture_field(&l, &next)) return false;
            const struct settlement_culture *old = settlement_culture_find(styles, next.key);
            if (old) styles->cultures[old - styles->cultures] = next;
            else {
                if (styles->culture_count >= 2048) return false;
                if (styles->culture_count == styles->culture_capacity) {
                    const size_t count = styles->culture_capacity ? styles->culture_capacity * 2 : 128;
                    void *next_mem = realloc(styles->cultures, count * sizeof *styles->cultures);
                    if (!next_mem) return false;
                    styles->cultures = next_mem;
                    styles->culture_capacity = count;
                }
                styles->cultures[styles->culture_count++] = next;
            }
        }
        if (!ck2_span_eq(name, "}")) return false;
    }
    return true;
}

const struct settlement_culture *settlement_culture_find(const struct settlement_styles *styles,
                                                          const char *key)
{
    if (!styles || !key || !*key) return NULL;
    for (size_t i = 0; i < styles->culture_count; ++i)
        if (!strcmp(styles->cultures[i].key, key)) return styles->cultures + i;
    return NULL;
}

bool settlement_styles_parse_actors(struct settlement_styles *styles, const char *source)
{
    if (!styles || !source) return false;
    struct ck2_lexer l = {source};
    struct ck2_span root;
    while ((root = ck2_lex(&l)).length) {
        if (!ck2_span_eq(ck2_lex(&l), "=") || !ck2_span_eq(ck2_lex(&l), "{")) return false;
        struct ck2_span entry;
        while ((entry = ck2_lex(&l)).length && !ck2_span_eq(entry, "}")) {
            if (!ck2_span_eq(ck2_lex(&l), "=") || !ck2_span_eq(ck2_lex(&l), "{")) return false;
            struct settlement_actor actor = {0};
            struct ck2_span field;
            while ((field = ck2_lex(&l)).length && !ck2_span_eq(field, "}")) {
                if (!ck2_span_eq(ck2_lex(&l), "=")) return false;
                const struct ck2_span v = ck2_lex(&l);
                if (!v.start) return false;
                if (ck2_span_eq(v, "{")) ck2_skip_scope(&l);
                else if (ck2_span_eq(field, "name")) ck2_span_copy(actor.key, sizeof actor.key, v);
                else if (ck2_span_eq(field, "actorfile") || ck2_span_eq(field, "file"))
                    ck2_span_copy(actor.path, sizeof actor.path, v);
            }
            if (!ck2_span_eq(field, "}")) return false;
            if (actor.key[0] && actor.path[0] && styles->actor_count < 2048) {
                for (char *p = actor.path; *p; ++p) if (*p == '\\') *p = '/';
                if (styles->actor_count == styles->actor_capacity) {
                    const size_t count = styles->actor_capacity ? styles->actor_capacity * 2 : 128;
                    void *next_mem = realloc(styles->actors, count * sizeof *styles->actors);
                    if (!next_mem) return false;
                    styles->actors = next_mem; styles->actor_capacity = count;
                }
                styles->actors[styles->actor_count++] = actor;
            }
        }
        if (!ck2_span_eq(entry, "}")) return false;
    }
    return true;
}

const struct settlement_actor *settlement_actor_find(const struct settlement_styles *styles,
    const char *graphics, const char *type, unsigned tier)
{
    if (!styles || !graphics || !*graphics || !type || !*type || tier > 9) return NULL;
    char name[112];
    if (snprintf(name, sizeof name, "%s_%s_%u", graphics, type, tier) >= (int)sizeof name)
        return NULL;
    for (size_t i = 0; i < styles->actor_count; ++i)
        if (!strcmp(styles->actors[i].key, name)) return styles->actors + i;
    return NULL;
}

void settlement_styles_destroy(struct settlement_styles *s)
{
    if (!s) return;
    free(s->actors); free(s->cultures);
    *s = (struct settlement_styles){0};
}
