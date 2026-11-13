#include "map_palette_core.h"
#include "ck2_lexer_core.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool rgb_read(struct ck2_lexer *lexer, uint32_t *color)
{
    double components[3];
    bool valid = true;
    for (int i = 0; i < 3; ++i) {
        const struct ck2_span s = ck2_lex(lexer);
        char raw[32], *end;
        ck2_span_copy(raw, sizeof raw, s);
        components[i] = strtod(raw, &end);
        if (end == raw || *end || !isfinite(components[i]) || components[i] < 0) valid = false;
    }
    ck2_skip_scope(lexer);
    if (!valid) return false;
    uint32_t rgb = 0;
    for (int i = 0; i < 3; ++i) {
        double v = components[i] <= 1.0 ? components[i] * 255.0 : components[i];
        if (v > 255.) return false;
        rgb |= (uint32_t)(v + 0.5) << (16 - i * 8);
    }
    *color = rgb;
    return true;
}

bool map_palette_find(const struct map_palette *p, const char *id, uint32_t *color)
{
    if (!p || !id || !*id) return false;
    for (size_t i = p->count; i > 0; --i)
        if (!strcmp(p->entries[i - 1].id, id)) {
            if (color) *color = p->entries[i - 1].rgb;
            return true;
        }
    return false;
}

static bool insert(struct map_palette *p, const char *id, uint32_t color)
{
    for (size_t i = 0; i < p->count; ++i)
        if (!strcmp(p->entries[i].id, id)) {
            p->entries[i].rgb = color;
            return true;
        }
    if (p->count == p->capacity) {
        size_t next_size = p->capacity ? p->capacity * 2 : 64;
        if (next_size <= p->capacity || next_size > SIZE_MAX / sizeof *p->entries) return false;
        struct map_palette_entry *next = realloc(p->entries, next_size * sizeof *next);
        if (!next) return false;
        p->entries = next;
        p->capacity = next_size;
    }
    struct map_palette_entry *entry = &p->entries[p->count++];
    snprintf(entry->id, sizeof entry->id, "%s", id);
    entry->rgb = color;
    return true;
}

static bool parse_scope(struct map_palette *p, struct ck2_lexer *lex,
                        const char *scope_name, unsigned depth)
{
    if (depth > 32) return false;
    for (struct ck2_span key; (key = ck2_lex(lex)).length;) {
        if (ck2_span_eq(key, "}")) return true;
        const char *before_equal = lex->next;
        if (!ck2_span_eq(ck2_lex(lex), "=")) {
            /* CK2 name lists contain bare tokens; do not swallow a closing
             * brace as their would-be assignment delimiter. */
            lex->next = before_equal;
            continue;
        }
        struct ck2_span value = ck2_lex(lex);
        if (!ck2_span_eq(value, "{")) continue;
        if (ck2_span_eq(key, "color")) {
            uint32_t color = 0;
            const bool valid = rgb_read(lex, &color);
            if (valid && scope_name && *scope_name && !insert(p, scope_name, color))
                return false;
            /* If the color is malformed, rgb_read already skipped its scope. */
            continue;
        }
        if (depth >= 2 || ck2_span_eq(key, "male_names") ||
            ck2_span_eq(key, "female_names")) {
            /* Deep scripted triggers can nest dozens of levels. They do not
             * define a culture/religion palette entry. */
            ck2_skip_scope(lex);
            continue;
        }
        char nested[96];
        ck2_span_copy(nested, sizeof nested, key);
        if (!parse_scope(p, lex, nested, depth + 1)) return false;
    }
    return true;
}

bool map_palette_parse(struct map_palette *p, const char *source)
{
    if (!p || !source) return false;
    struct ck2_lexer lexer = {.next = source};
    return parse_scope(p, &lexer, NULL, 0);
}

void map_palette_free(struct map_palette *p)
{
    if (!p) return;
    free(p->entries);
    *p = (struct map_palette){0};
}
