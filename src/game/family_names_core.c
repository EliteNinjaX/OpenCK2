#include "family_names_core.h"
#include "ck2_lexer_core.h"
#include <string.h>

static bool choose_list(struct ck2_lexer *lex, unsigned seed, char *out, size_t capacity)
{
    const char *start = lex->next;
    size_t count = 0;
    for (struct ck2_span name; (name = ck2_lex(lex)).length;) {
        if (ck2_span_eq(name, "}")) break;
        if (!ck2_span_eq(name, "{")) ++count;
        else ck2_skip_scope(lex);
    }
    if (!count) return false;
    const size_t selected = seed % count;
    lex->next = start;
    for (size_t i = 0; i <= selected; ++i) {
        struct ck2_span name = ck2_lex(lex);
        if (!name.length || ck2_span_eq(name, "}")) return false;
        if (i == selected) {
            /* Underscore-separated entries are CK2 name-equivalence aliases.
             * Display the authored first segment, not the underscore token. */
            const char *alias = memchr(name.start, '_', name.length);
            if (alias) name.length = (size_t)(alias - name.start);
            ck2_span_copy(out, capacity, name);
            return out[0] != 0;
        }
    }
    return false;
}
static bool culture_scope(struct ck2_lexer *lex, const char *key, bool female,
                          unsigned seed, char *out, size_t capacity)
{
    const char *names = female ? "female_names" : "male_names";
    for (struct ck2_span field; (field = ck2_lex(lex)).length;) {
        if (ck2_span_eq(field, "}")) break;
        if (!ck2_span_eq(ck2_lex(lex), "=")) return false;
        struct ck2_span value = ck2_lex(lex);
        if (!ck2_span_eq(value, "{")) continue;
        if (ck2_span_eq(field, names) && key)
            return choose_list(lex, seed, out, capacity);
        ck2_skip_scope(lex);
    }
    return false;
}
bool family_name_choose(const char *source, const char *culture, bool female,
                        unsigned seed, char *out, size_t capacity)
{
    if (!source || !culture || !*culture || !out || !capacity) return false;
    out[0] = 0;
    struct ck2_lexer lex = {.next=source};
    for (struct ck2_span group; (group = ck2_lex(&lex)).length;) {
        if (!ck2_span_eq(ck2_lex(&lex), "=")) return false;
        if (!ck2_span_eq(ck2_lex(&lex), "{")) return false;
        for (struct ck2_span child; (child = ck2_lex(&lex)).length;) {
            if (ck2_span_eq(child, "}")) break;
            if (!ck2_span_eq(ck2_lex(&lex), "=")) return false;
            struct ck2_span value = ck2_lex(&lex);
            if (!ck2_span_eq(value, "{")) continue;
            if (ck2_span_eq(child, culture)) {
                if (culture_scope(&lex, culture, female, seed, out, capacity)) return true;
            } else ck2_skip_scope(&lex);
        }
    }
    return false;
}
