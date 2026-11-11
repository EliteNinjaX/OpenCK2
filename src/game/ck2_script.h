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

#endif
