#include "warfare_map.h"
#include <stdint.h>
#include <stdlib.h>

bool warfare_graph_from_map(struct warfare_graph *graph, const struct province_map *map)
{
    if (!graph || !map || !map->pixels || !map->provinces ||
        map->pixels->w <= 0 || map->pixels->h <= 0) return false;
    unsigned max_id = 0;
    for (size_t i = 0; i < map->count; ++i)
        if (map->provinces[i].land && map->provinces[i].id > max_id)
            max_id = map->provinces[i].id;
    if (!max_id || max_id > 100000) return false;
    /* Temporary 64 MiB color table; released as soon as the graph is built.
     * IDs are original province IDs, never color-sorted array positions. */
    uint32_t *colors = calloc(1u << 24, sizeof *colors);
    struct warfare_graph fresh = {0};
    if (!colors || !warfare_graph_init(&fresh, (size_t)max_id + 1)) {
        free(colors); warfare_graph_free(&fresh); return false;
    }
    for (size_t i = 0; i < map->count; ++i)
        if (map->provinces[i].land && map->provinces[i].color < (1u << 24))
            colors[map->provinces[i].color] = map->provinces[i].id;
    const SDL_Surface *p = map->pixels;
    bool ok = true;
    for (int y = 0; ok && y < p->h; ++y) {
        const uint32_t *line = (const uint32_t *)((const unsigned char *)p->pixels +
                                                (size_t)y * (size_t)p->pitch);
        const uint32_t *above = y ? (const uint32_t *)((const unsigned char *)p->pixels +
                                                (size_t)(y - 1) * (size_t)p->pitch) : NULL;
        for (int x = 0; x < p->w; ++x) {
            const unsigned a = colors[line[x] & 0xffffffu];
            if (!a) continue;
            const unsigned left = x ? colors[line[x - 1] & 0xffffffu] : 0;
            const unsigned north = above ? colors[above[x] & 0xffffffu] : 0;
            if (left && left != a && !warfare_graph_connect(&fresh, left, a)) { ok = false; break; }
            if (north && north != a && !warfare_graph_connect(&fresh, north, a)) { ok = false; break; }
        }
    }
    free(colors);
    if (!ok) { warfare_graph_free(&fresh); return false; }
    warfare_graph_free(graph);
    *graph = fresh;
    return true;
}
