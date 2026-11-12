#ifndef OPENCK2_SCRIPT_H
#define OPENCK2_SCRIPT_H

#include <SDL.h>
#include <stdbool.h>

struct token {
    const char *start;
    size_t size;
};

struct token token(const char **);
bool is(struct token, const char *);
void copy_token(char *, size_t, struct token);
void skip_block(const char **);
struct token find(const char **, const char *);
struct token field(const char *, const char *);
unsigned number(struct token);
char *load_data(const char *, const char *);
char *load_from_directory(const char *, const char *, const char *);

struct ck2_translation {
    const char *key;
    char *text;
    size_t capacity;
};
void ck2_translate(const char *, struct ck2_translation *, size_t);
/* The selected language is loaded once into a shared catalog. Values are valid
 * until the next ck2_locale_reload(), which must not happen during rendering. */
int ck2_locale_reload(const char *root, int language);
void ck2_locale_free(void);
const char *ck2_localize(const char *key, const char *fallback);
int ck2_locale_language(void);
void ck2_locale_format(char *, size_t, const char *key, const char *fallback,
                       const char *variable, const char *value);


#endif
