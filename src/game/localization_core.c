#include "localization_core.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t hash_string(const char *s)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    while (*s) { hash ^= (unsigned char)*s++; hash *= UINT64_C(1099511628211); }
    return (size_t)hash;
}

void loc_catalog_init(struct loc_catalog *cat, enum ck2_language language)
{
    *cat = (struct loc_catalog){.language = language};
}

void loc_catalog_free(struct loc_catalog *cat)
{
    for (size_t i = 0; i < cat->capacity; ++i) {
        free(cat->slots[i].key);
        free(cat->slots[i].value);
    }
    free(cat->slots);
    *cat = (struct loc_catalog){0};
}

static bool grow(struct loc_catalog *cat)
{
    size_t next = cat->capacity ? cat->capacity * 2 : 2048;
    if (next < cat->capacity || next > SIZE_MAX / sizeof *cat->slots) return false;
    struct loc_entry *slots = calloc(next, sizeof *slots);
    if (!slots) return false;
    for (size_t i = 0; i < cat->capacity; ++i) {
        struct loc_entry entry = cat->slots[i];
        if (!entry.key) continue;
        size_t at = hash_string(entry.key) & (next - 1);
        while (slots[at].key) at = (at + 1) & (next - 1);
        slots[at] = entry;
    }
    free(cat->slots);
    cat->slots = slots;
    cat->capacity = next;
    return true;
}

static char *duplicate_trim(const char *text, size_t length, bool convert_markup)
{
    while (length && (*text == ' ' || *text == '\t')) {++text; --length;}
    while (length && (text[length - 1] == ' ' || text[length - 1] == '\r')) --length;
    if (length >= 2 && *text == '"' && text[length - 1] == '"') {
        ++text;
        length -= 2;
    }
    char *out = malloc(length + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        /* CK2 text uses single-byte section-sign color markers (e.g. §Y and §!). */
        if (convert_markup && c == 0xa7 && i + 1 < length) {++i; continue;}
        if (c == '"' && i + 1 < length && text[i + 1] == '"') {out[j++] = '"'; ++i; continue;}
        if (convert_markup && c == '\\' && i + 1 < length && text[i+1] == 'n') {
            out[j++] = ' '; ++i; continue;
        }
        if (convert_markup && c == 0x92) c = '\'';
        out[j++] = (char)c;
    }
    out[j] = 0;
    return out;
}

bool loc_catalog_add_line(struct loc_catalog *cat, const char *line, size_t length)
{
    if (!line || !length || *line == '#' || *line == ';') return true;
    /* CK2 CSV: KEY;ENGLISH;FRENCH;GERMAN;POLISH;SPANISH;... .
       Preserve the original game's single-byte encoding for the existing glyph atlas.
       CK2's standard CSV is semicolon-delimited, with optional quoted fields. */
    const char *fields[6] = {0};
    size_t sizes[6] = {0};
    size_t column = 0, start = 0;
    bool quoted = false;
    for (size_t i = 0; i <= length && column < 6; ++i) {
        const char c = i == length ? ';' : line[i];
        if (c == '"') {
            if (quoted && i + 1 < length && line[i + 1] == '"') { ++i; continue; }
            quoted = !quoted;
        }
        if (c == ';' && !quoted) {
            fields[column] = line + start;
            sizes[column++] = i - start;
            start = i + 1;
        }
    }
    if (column < 2 || !sizes[0] || sizes[0] > 191) return true;
    char *key = duplicate_trim(fields[0], sizes[0], false);
    if (!key) return false;
    if (!*key) {free(key);return true;}
    size_t preferred = cat->language == CK2_FRENCH ? 2 : cat->language == CK2_GERMAN ? 3 :
                       cat->language == CK2_SPANISH ? 5 : 1;
    if (preferred >= column || !sizes[preferred]) preferred = 1;
    if (preferred >= column || !sizes[preferred]) {free(key);return true;}
    char *value = duplicate_trim(fields[preferred], sizes[preferred], true);
    if (!value) {free(key);return false;}
    if (!*value && preferred != 1 && column > 1) {
        free(value);
        value = duplicate_trim(fields[1], sizes[1], true);
        if (!value) {free(key); return false;}
    }
    if ((cat->count + 1) * 10 >= cat->capacity * 7 && !grow(cat)) {free(key);free(value);return false;}
    size_t at = hash_string(key) & (cat->capacity - 1);
    while (cat->slots[at].key && strcmp(cat->slots[at].key, key)) at = (at + 1) & (cat->capacity - 1);
    if (cat->slots[at].key) {
        /* CK2 overlays may override earlier entries. File order follows mounted asset order. */
        free(cat->slots[at].value);
        cat->slots[at].value = value;
        free(key);
    } else {
        cat->slots[at] = (struct loc_entry){key, value};
        ++cat->count;
    }
    return true;
}

const char *loc_catalog_find(const struct loc_catalog *cat, const char *key)
{
    if (!cat || !cat->capacity || !key || !*key) return NULL;
    size_t at = hash_string(key) & (cat->capacity - 1);
    while (cat->slots[at].key) {
        if (!strcmp(cat->slots[at].key, key)) return cat->slots[at].value;
        at = (at + 1) & (cat->capacity - 1);
    }
    return NULL;
}

const char *loc_language_name(enum ck2_language language)
{
    static const char *const names[] = {"English", "Fran\xe7" "ais", "Deutsch", "Espa\xf1" "ol"};
    return (unsigned)language < CK2_LANGUAGE_COUNT ? names[language] : names[0];
}
