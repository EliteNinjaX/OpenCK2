#ifndef OPENCK2_RELIGION_STYLE_CACHE_H
#define OPENCK2_RELIGION_STYLE_CACHE_H

#include <stdbool.h>
#include <stddef.h>

enum { RELIGION_STYLE_CACHE_CAP = 96, RELIGION_STYLE_KEY_CAP = 64 };
struct religion_style_cache_entry {
    char religion[RELIGION_STYLE_KEY_CAP];
    char graphical[RELIGION_STYLE_KEY_CAP];
};
struct religion_style_cache {
    struct religion_style_cache_entry entries[RELIGION_STYLE_CACHE_CAP];
    size_t size, eviction_index;
};

/* A tiny, bounded cache keyed by CK2 religion identifier. The caller still
 * performs the actual loose-file lookup on misses, honoring mods/DLC. */
bool religion_style_cache_get(const struct religion_style_cache *, const char *religion,
                              char *result, size_t capacity);
bool religion_style_cache_put(struct religion_style_cache *, const char *religion,
                              const char *graphical);
void religion_style_cache_clear(struct religion_style_cache *);

#endif
