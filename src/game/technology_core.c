#include "technology_core.h"
#include "ck2_lexer_core.h"
#include <string.h>

static int parse_group(struct ck2_technology_catalog *out, struct ck2_lexer *lexer,
                       enum ck2_tech_group group)
{
    struct ck2_span key;
    while ((key = ck2_lex(lexer)).length) {
        if (ck2_span_eq(key, "}")) return 0;
        if (!ck2_span_eq(ck2_lex(lexer), "=")) return -1;
        struct ck2_span start = ck2_lex(lexer);
        if (!ck2_span_eq(start, "{")) return -1;
        /* Technology entries are keyed TECH_*. Preserve their order and
         * skip nested effect/unlock scopes without interpreting scripts. */
        if (key.length >= 5 && !memcmp(key.start, "TECH_", 5)) {
            if (out->count >= CK2_TECH_MAX || key.length >= sizeof out->items[0].key)
                return -1;
            struct ck2_technology *entry = &out->items[out->count++];
            ck2_span_copy(entry->key, sizeof entry->key, key);
            entry->group = group;
        }
        ck2_skip_scope(lexer);
    }
    return -1;
}

int ck2_technology_parse(struct ck2_technology_catalog *out, const char *text)
{
    if (!out || !text) return -1;
    *out = (struct ck2_technology_catalog){0};
    struct ck2_lexer lexer = {text};
    struct ck2_span key;
    while ((key = ck2_lex(&lexer)).length) {
        if (!ck2_span_eq(ck2_lex(&lexer), "=")) return -1;
        if (!ck2_span_eq(ck2_lex(&lexer), "{")) return -1;
        enum ck2_tech_group group = CK2_TECH_GROUP_COUNT;
        if (ck2_span_eq(key, "TECH_GROUP_MILITARY")) group = CK2_TECH_MILITARY;
        if (ck2_span_eq(key, "TECH_GROUP_ECONOMY")) group = CK2_TECH_ECONOMY;
        if (ck2_span_eq(key, "TECH_GROUP_CULTURE")) group = CK2_TECH_CULTURE;
        if (group != CK2_TECH_GROUP_COUNT) {
            if (parse_group(out, &lexer, group) < 0) return -1;
        } else ck2_skip_scope(&lexer);
    }
    return out->count ? 0 : -1;
}

size_t ck2_technology_group_count(const struct ck2_technology_catalog *catalog,
                                   enum ck2_tech_group group)
{
    size_t count = 0;
    if (!catalog) return 0;
    for (size_t i = 0; i < catalog->count; ++i)
        count += catalog->items[i].group == group;
    return count;
}

const struct ck2_technology *ck2_technology_find(const struct ck2_technology_catalog *catalog,
                                                  const char *key)
{
    if (!catalog || !key) return NULL;
    for (size_t i = 0; i < catalog->count; ++i)
        if (!strcmp(catalog->items[i].key, key)) return &catalog->items[i];
    return NULL;
}
