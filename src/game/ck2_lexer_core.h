#ifndef OPENCK2_LEXER_CORE_H
#define OPENCK2_LEXER_CORE_H
#include <stddef.h>
#include <stdbool.h>

/* Small, SDL-independent CK2 assignment lexer. The tokens point into source,
 * which must outlive them. It does not evaluate scripted triggers. */
struct ck2_span { const char *start; size_t length; };
struct ck2_lexer { const char *next; };
struct ck2_span ck2_lex(struct ck2_lexer *lexer);
bool ck2_span_eq(struct ck2_span token, const char *value);
void ck2_span_copy(char *dst, size_t capacity, struct ck2_span token);
/* Opening '{' has already been consumed. Returns the first byte after '}'. */
void ck2_skip_scope(struct ck2_lexer *lexer);
#endif
