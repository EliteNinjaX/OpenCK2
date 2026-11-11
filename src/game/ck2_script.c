#include "../open_launcher/asset_paths.h"
#include "ck2_script.h"
#include "../open_launcher/install.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct token token(const char **input)
{
    const char *p = *input;
    while (*p && (isspace((unsigned char)*p) || *p == '#')) {
        if (*p == '#') {
            while (*p && *p != '\n')
                ++p;
        } else {
            ++p;
        }
    }
    const char *start = p;
    if (*p && strchr("{}=", *p)) {
        ++p;
    } else if (*p == '"') {
        start = ++p;
        while (*p && *p != '"') {
            if (*p == '\\' && p[1])
                ++p;
            ++p;
        }
        *input = *p ? p + 1 : p;
        return (struct token){start, (size_t)(p - start)};
    } else {
        while (*p && !isspace((unsigned char)*p) && !strchr("{}=#", *p))
            ++p;
    }
    *input = p;
    return (struct token){start, (size_t)(p - start)};
}

bool is(struct token t, const char *word)
{
    return t.size == strlen(word) && !memcmp(t.start, word, t.size);
}

void copy_token(char *buffer, size_t size, struct token t)
{
    const size_t n = SDL_min(size - 1, t.size);
    if (n)
        memcpy(buffer, t.start, n);
    buffer[n] = '\0';
}

void skip_block(const char **p)
{
    unsigned depth = 1;
    for (struct token t; depth && ((t = token(p)).size || *t.start);) {
        if (is(t, "{"))
            ++depth;
        else if (is(t, "}"))
            --depth;
    }
}

/* Searches one scope and advances past the matching value. Blocks return at their opening. */
struct token find(const char **p, const char *name)
{
    while (true) {
        struct token key = token(p);
        if (!key.size || is(key, "}"))
            return (struct token){0};
        if (!is(token(p), "="))
            return (struct token){0};
        struct token value = token(p);
        if (is(key, name))
            return value;
        if (is(value, "{"))
            skip_block(p);
    }
}

struct token field(const char *p, const char *name)
{
    return find(&p, name);
}

unsigned number(struct token t)
{
    char buffer[24];
    copy_token(buffer, sizeof buffer, t);
    return (unsigned)strtoul(buffer, NULL, 10);
}

char *load_data(const char *root, const char *relative)
{
    char *path = ck2_asset_path(root, relative);
    char *data = path ? SDL_LoadFile(path, NULL) : NULL;
    free(path);
    return data;
}


char *load_from_directory(const char *root, const char *directory, const char *name)
{
    char *relative = ck2_join_path(directory, name);
    char *data = relative ? load_data(root, relative) : NULL;
    free(relative);
    return data;
}



static int compare_translation(const void *a, const void *b)
{
    return strcmp(((const struct ck2_translation *)a)->key,
                  ((const struct ck2_translation *)b)->key);
}

void ck2_translate(const char *root, struct ck2_translation *terms, size_t count)
{
    qsort(terms, count, sizeof *terms, compare_translation);
    struct ck2_asset_files files = ck2_list_assets(root, "localisation");
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 4, ".csv"))
            continue;
        char *data = load_from_directory(root, "localisation", name);
        for (const char *p = data; p && *p;) {
            const char *end = strchr(p, '\n');
            if (!end)
                end = p + strlen(p);
            const char *sep = memchr(p, ';', (size_t)(end - p));
            char key[128];
            if (sep && sep > p && *p != '#' && (size_t)(sep - p) < sizeof key) {
                copy_token(key, sizeof key, (struct token){p, (size_t)(sep - p)});
                const struct ck2_translation match = {.key = key};
                struct ck2_translation *term = bsearch(&match, terms, count, sizeof *terms,
                                                       compare_translation);
                const char *last = memchr(sep + 1, ';', (size_t)(end - (sep + 1)));
                if (term && last && last > sep + 1) {
                    while (term > terms && !strcmp(term[-1].key, key))
                        --term;
                    for (; term < terms + count && !strcmp(term->key, key); ++term) {
                        size_t n = 0;
                        for (const char *s = sep + 1; s < last && n + 1 < term->capacity; ++s) {
                            if (*s == '\\' && s + 1 < last && s[1] == 'n') {
                                term->text[n++] = ' ';
                                ++s;
                            } else if ((unsigned char)*s == 0xa7 && s + 1 < last) {
                                ++s;
                            } else {
                                term->text[n++] = (unsigned char)*s == 0x92 ? '\'' : *s;
                            }
                        }
                        term->text[n] = '\0';
                    }
                }
            }
            p = *end ? end + 1 : end;
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
}
