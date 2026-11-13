#include "../src/game/warfare_map.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void)
{
    uint32_t pixels[] = {
        0x11u, 0x11u, 0x22u,
        0x11u, 0x33u, 0x22u,
        0x44u, 0x33u, 0x22u
    };
    SDL_Surface surface = {.w = 3, .h = 3, .pitch = 12, .pixels = pixels};
    struct province provinces[] = {
        {.id = 1, .color = 0x11u, .land = true},
        {.id = 2, .color = 0x22u, .land = true},
        {.id = 3, .color = 0x33u, .land = true},
        {.id = 4, .color = 0x44u, .land = false},
    };
    struct province_map map = {.pixels = &surface, .provinces = provinces,
                               .count = sizeof provinces / sizeof *provinces};
    struct warfare_graph graph = {0};
    assert(warfare_graph_from_map(&graph, &map));
    assert(warfare_graph_adjacent(&graph, 1, 2));
    assert(warfare_graph_adjacent(&graph, 1, 3));
    assert(warfare_graph_adjacent(&graph, 2, 3));
    assert(!warfare_graph_adjacent(&graph, 4, 3));
    assert(!warfare_graph_adjacent(&graph, 4, 1));
    assert(!warfare_graph_adjacent(&graph, 1, 1));
    warfare_graph_free(&graph);
    puts("warfare map: all assertions passed");
}
