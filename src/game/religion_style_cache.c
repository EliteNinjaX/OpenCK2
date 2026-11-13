#include "religion_style_cache.h"
#include <string.h>

static bool valid(const char *key)
{
    return key && *key && strlen(key) < RELIGION_STYLE_KEY_CAP;
}

bool religion_style_cache_get(const struct religion_style_cache *cache, const char *religion,
                              char *result, size_t capacity)
{
    if (!cache || !valid(religion) || !result || !capacity)
        return false;
    for (size_t i = 0; i < cache->size; ++i) {
        if (strcmp(cache->entries[i].religion, religion))
            continue;
        const size_t size = strlen(cache->entries[i].graphical) + 1;
        if (size > capacity)
            return false;
        memcpy(result, cache->entries[i].graphical, size);
        return true;
    }
    return false;
}

bool religion_style_cache_put(struct religion_style_cache *cache, const char *religion,
                              const char *graphical)
{
    if (!cache || !valid(religion) || !valid(graphical))
        return false;
    size_t index = cache->size;
    for (size_t i = 0; i < cache->size; ++i) {
        if (!strcmp(cache->entries[i].religion, religion)) {
            index = i;
            break;
        }
    }
    if (index == cache->size) {
        if (cache->size < RELIGION_STYLE_CACHE_CAP)
            ++cache->size;
        else {
            index = cache->eviction_index;
            cache->eviction_index = (cache->eviction_index + 1) % RELIGION_STYLE_CACHE_CAP;
        }
    }
    strcpy(cache->entries[index].religion, religion);
    strcpy(cache->entries[index].graphical, graphical);
    return true;
}

void religion_style_cache_clear(struct religion_style_cache *cache)
{
    if (cache)
        *cache = (struct religion_style_cache){0};
}
