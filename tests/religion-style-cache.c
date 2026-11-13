#include "../src/game/religion_style_cache.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct religion_style_cache cache = {0};
    char result[64];
    assert(!religion_style_cache_get(&cache, "catholic", result, sizeof result));
    assert(religion_style_cache_put(&cache, "catholic", "westerngfx"));
    assert(religion_style_cache_put(&cache, "orthodox", "westerngfx"));
    assert(religion_style_cache_get(&cache, "catholic", result, sizeof result));
    assert(!strcmp(result, "westerngfx"));
    assert(religion_style_cache_put(&cache, "catholic", "byzantinegfx"));
    assert(cache.size == 2);
    assert(religion_style_cache_get(&cache, "catholic", result, sizeof result));
    assert(!strcmp(result, "byzantinegfx"));
    assert(!religion_style_cache_get(&cache, "catholic", result, 3));
    assert(!religion_style_cache_put(&cache, "", "westerngfx"));
    assert(!religion_style_cache_put(&cache, "abc", ""));
    for (int i = 0; i < RELIGION_STYLE_CACHE_CAP + 12; ++i) {
        char id[32];
        snprintf(id, sizeof id, "religion%d", i);
        assert(religion_style_cache_put(&cache, id, "nomadgfx"));
    }
    assert(cache.size == RELIGION_STYLE_CACHE_CAP);
    assert(religion_style_cache_get(&cache, "religion107", result, sizeof result));
    assert(!strcmp(result, "nomadgfx"));
    religion_style_cache_clear(&cache);
    assert(cache.size == 0 && cache.eviction_index == 0);
    puts("religion style cache: all assertions passed");
}
