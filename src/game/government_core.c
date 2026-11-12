#include "government_core.h"
#include "ck2_lexer_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uint32_t government_holding_mask(const char *key)
{
    static const struct { const char *name; uint32_t bit; } names[] = {
        {"CASTLE", GOVERNMENT_CASTLE}, {"CITY", GOVERNMENT_CITY},
        {"TEMPLE", GOVERNMENT_TEMPLE}, {"TRIBAL", GOVERNMENT_TRIBAL},
        {"NOMAD", GOVERNMENT_NOMAD}, {"FORT", GOVERNMENT_FORT},
        {"HOSPITAL", GOVERNMENT_HOSPITAL}, {"TRADE_POST", GOVERNMENT_TRADE_POST},
        {"FAMILY_PALACE", GOVERNMENT_FAMILY_PALACE}
    };
    if (!key) return 0;
    for (size_t i = 0; i < sizeof names / sizeof names[0]; ++i)
        if (!strcmp(key, names[i].name)) return names[i].bit;
    return 0;
}

static char *copy_source(const char *start, const char *end)
{
    if (!start || !end || end < start) return NULL;
    const size_t n = (size_t)(end - start);
    char *result = malloc(n + 1);
    if (result) { memcpy(result, start, n); result[n] = 0; }
    return result;
}

static uint32_t holdings(struct ck2_lexer *lex)
{
    uint32_t bits = 0;
    struct ck2_span t;
    while ((t = ck2_lex(lex)).length && !ck2_span_eq(t, "}")) {
        char name[64];
        ck2_span_copy(name, sizeof name, t);
        bits |= government_holding_mask(name);
    }
    return bits;
}

static uint32_t read_color(struct ck2_lexer *lex, bool *valid)
{
    int channels[3] = {0};
    *valid = true;
    for (int i = 0; i < 3; ++i) {
        char number[32], *end;
        ck2_span_copy(number, sizeof number, ck2_lex(lex));
        const long val = strtol(number, &end, 10);
        if (!number[0] || *end || val < 0 || val > 255) *valid = false;
        channels[i] = (int)val;
    }
    ck2_skip_scope(lex); /* Remaining color scope, usually just closing brace. */
    return (uint32_t)(channels[0] & 255) << 16 |
           (uint32_t)(channels[1] & 255) << 8 | (uint32_t)(channels[2] & 255);
}

static bool parse_definition(struct government_definition *def, struct ck2_lexer *lex)
{
    struct ck2_span key;
    def->aggression = 1.0;
    while ((key = ck2_lex(lex)).length && !ck2_span_eq(key, "}")) {
        if (!ck2_span_eq(ck2_lex(lex), "=")) return false;
        struct ck2_span value = ck2_lex(lex);
        if (!value.start) return false;
        if (ck2_span_eq(value, "{")) {
            if (ck2_span_eq(key, "preferred_holdings")) def->preferred_holdings = holdings(lex);
            else if (ck2_span_eq(key, "allowed_holdings")) def->allowed_holdings = holdings(lex);
            else if (ck2_span_eq(key, "color"))
                def->color_rgb = read_color(lex, &def->color_defined);
            else if (ck2_span_eq(key, "potential")) {
                const char *first = value.start;
                ck2_skip_scope(lex);
                char *source = copy_source(first, lex->next);
                if (!source) return false;
                free(def->potential_script);
                def->potential_script = source;
                def->has_potential = true;
            } else ck2_skip_scope(lex);
        } else {
            if (ck2_span_eq(key, "frame_suffix")) ck2_span_copy(def->frame_suffix, sizeof def->frame_suffix, value);
            else if (ck2_span_eq(key, "title_prefix")) ck2_span_copy(def->title_prefix, sizeof def->title_prefix, value);
            else if (ck2_span_eq(key, "merchant_republic")) def->merchant_republic = ck2_span_eq(value, "yes");
            else if (ck2_span_eq(key, "allow_looting")) def->allow_looting = ck2_span_eq(value, "yes");
            else if (ck2_span_eq(key, "can_build_holdings")) {
                def->can_build_holdings_defined = true;
                def->can_build_holdings = ck2_span_eq(value, "yes");
            } else if (ck2_span_eq(key, "max_consorts")) {
                char buf[32]; ck2_span_copy(buf, sizeof buf, value);
                def->max_consorts = (unsigned)strtoul(buf, NULL, 10);
            } else if (ck2_span_eq(key, "aggression")) {
                char buf[32]; ck2_span_copy(buf, sizeof buf, value);
                def->aggression = strtod(buf, NULL);
            }
        }
    }
    return ck2_span_eq(key, "}");
}

const struct government_definition *government_find(const struct government_catalog *cat, const char *id)
{
    if (!cat || !id) return NULL;
    for (size_t i = 0; i < cat->count; ++i)
        if (!strcmp(cat->items[i].id, id)) return &cat->items[i];
    return NULL;
}

bool government_catalog_parse(struct government_catalog *cat, const char *source)
{
    if (!cat || !source) return false;
    struct ck2_lexer lex = {source};
    struct ck2_span group;
    while ((group = ck2_lex(&lex)).length) {
        if (!ck2_span_eq(ck2_lex(&lex), "=") || !ck2_span_eq(ck2_lex(&lex), "{")) return false;
        struct ck2_span id;
        while ((id = ck2_lex(&lex)).length && !ck2_span_eq(id, "}")) {
            if (!ck2_span_eq(ck2_lex(&lex), "=") || !ck2_span_eq(ck2_lex(&lex), "{")) return false;
            struct government_definition next = {0};
            ck2_span_copy(next.id, sizeof next.id, id);
            ck2_span_copy(next.group, sizeof next.group, group);
            if (!parse_definition(&next, &lex)) { free(next.potential_script); return false; }
            const struct government_definition *existing = government_find(cat, next.id);
            if (existing) {
                struct government_definition *dest = &cat->items[existing - cat->items];
                free(dest->potential_script);
                *dest = next;
            } else {
                if (cat->count == 256) { free(next.potential_script); return false; }
                if (cat->count == cat->capacity) {
                    size_t capacity = cat->capacity ? cat->capacity * 2 : 16;
                    struct government_definition *items = realloc(cat->items, capacity * sizeof *items);
                    if (!items) { free(next.potential_script); return false; }
                    cat->items = items;
                    cat->capacity = capacity;
                }
                cat->items[cat->count++] = next;
            }
        }
        if (!ck2_span_eq(id, "}")) return false;
    }
    return true;
}

size_t government_holding_candidates(const struct government_catalog *cat, uint32_t bit,
                                     const struct government_definition **out, size_t capacity)
{
    size_t found = 0;
    if (!cat || !bit) return 0;
    for (size_t i = 0; i < cat->count; ++i)
        if (cat->items[i].preferred_holdings & bit) {
            if (out && found < capacity) out[found] = &cat->items[i];
            ++found;
        }
    return found;
}

void government_catalog_destroy(struct government_catalog *cat)
{
    if (!cat) return;
    for (size_t i = 0; i < cat->count; ++i) free(cat->items[i].potential_script);
    free(cat->items);
    *cat = (struct government_catalog){0};
}
