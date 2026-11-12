#include "../open_launcher/asset_paths.h"
#include "ck2_script.h"
#include "localization_core.h"
#include "../open_launcher/install.h"
#include <ctype.h>
#include <stdio.h>
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



static struct loc_catalog current_locale;
static bool locale_ready;

int ck2_locale_reload(const char *root, int language)
{
    if (language < 0 || language >= CK2_LANGUAGE_COUNT) return -1;
    struct loc_catalog next;
    loc_catalog_init(&next, (enum ck2_language)language);
    struct ck2_asset_files files = ck2_list_assets(root, "localisation");
    int result = 0;
    for (size_t i = 0; i < files.count && !result; ++i) {
        const char *name = files.names[i];
        size_t length = strlen(name);
        if (length < 4 || strcmp(name + length - 4, ".csv")) continue;
        char *data = load_from_directory(root, "localisation", name);
        for (const char *p = data; p && *p && !result;) {
            const char *end = strchr(p, '\n');
            if (!end) end = p + strlen(p);
            if (!loc_catalog_add_line(&next, p, (size_t)(end - p))) result = -1;
            p = *end ? end + 1 : end;
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
    if (result || !next.count) {loc_catalog_free(&next); return -1;}
    if (locale_ready) loc_catalog_free(&current_locale);
    current_locale = next;
    locale_ready = true;
    return 0;
}

void ck2_locale_free(void)
{
    if (locale_ready) loc_catalog_free(&current_locale);
    locale_ready = false;
}

int ck2_locale_language(void)
{
    return locale_ready ? (int)current_locale.language : CK2_ENGLISH;
}

const char *ck2_localize(const char *key, const char *fallback)
{
    const char *text = locale_ready ? loc_catalog_find(&current_locale, key) : NULL;
    return text && *text ? text : fallback ? fallback : key;
}

void ck2_locale_format(char *out, size_t capacity, const char *key, const char *fallback,
                       const char *variable, const char *value)
{
    if (!out || !capacity) return;
    const char *template = ck2_localize(key, fallback);
    char placeholder[100];
    snprintf(placeholder, sizeof placeholder, "$%s$", variable ? variable : "");
    const size_t pattern = strlen(placeholder);
    size_t j = 0;
    while (*template && j + 1 < capacity) {
        if (pattern > 2 && !strncmp(template, placeholder, pattern)) {
            const char *replacement = value ? value : "";
            while (*replacement && j + 1 < capacity) out[j++] = *replacement++;
            template += pattern;
        } else out[j++] = *template++;
    }
    out[j] = 0;
}

void ck2_translate(const char *root, struct ck2_translation *terms, size_t count)
{
    (void)root;
    for (size_t i = 0; i < count; ++i) {
        const char *text = ck2_localize(terms[i].key, "");
        if (terms[i].text && terms[i].capacity) {
            size_t n = strlen(text);
            if (n >= terms[i].capacity) n = terms[i].capacity - 1;
            memcpy(terms[i].text, text, n);
            terms[i].text[n] = 0;
        }
    }
}
