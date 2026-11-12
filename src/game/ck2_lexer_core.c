#include "ck2_lexer_core.h"
#include <ctype.h>
#include <string.h>

struct ck2_span ck2_lex(struct ck2_lexer *lexer)
{
    if (!lexer || !lexer->next)
        return (struct ck2_span){0};
    const char *p = lexer->next;
    while (*p) {
        if (isspace((unsigned char)*p)) { ++p; continue; }
        if (*p == '#') { while (*p && *p != '\n') ++p; continue; }
        break;
    }
    const char *first = p;
    if (*p == '{' || *p == '}' || *p == '=') ++p;
    else if (*p == '"') {
        first = ++p;
        while (*p && *p != '"') { if (*p == '\\' && p[1]) ++p; ++p; }
        lexer->next = *p ? p + 1 : p;
        return (struct ck2_span){first, (size_t)(p - first)};
    } else {
        while (*p && !isspace((unsigned char)*p) && *p != '{' && *p != '}' &&
               *p != '=' && *p != '#') ++p;
    }
    lexer->next = p;
    return (struct ck2_span){first, (size_t)(p - first)};
}

bool ck2_span_eq(struct ck2_span token, const char *value)
{
    return value && strlen(value) == token.length &&
        (!token.length || !memcmp(token.start, value, token.length));
}

void ck2_span_copy(char *dst, size_t capacity, struct ck2_span token)
{
    if (!dst || !capacity) return;
    const size_t length = token.length < capacity - 1 ? token.length : capacity - 1;
    if (length && token.start) memcpy(dst, token.start, length);
    dst[length] = 0;
}

void ck2_skip_scope(struct ck2_lexer *lexer)
{
    unsigned depth = 1;
    struct ck2_span t;
    while (depth && (t = ck2_lex(lexer)).length) {
        if (ck2_span_eq(t, "{")) ++depth;
        else if (ck2_span_eq(t, "}")) --depth;
    }
}
