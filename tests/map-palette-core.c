#include "../src/game/map_palette_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct map_palette p = {0};
    const char *sample = "christian = { color = { 0.8 0.8 0.6 } "
        " male_names = { Erik Inge Olaf } catholic = { color = { 0.3 0.6 1.0 } "
        "icon = 1 } orthodox = { color = { 140 80 40 } } }";
    assert(map_palette_parse(&p, sample));
    uint32_t color;
    assert(map_palette_find(&p, "christian", &color) && color == 0xcccc99);
    assert(map_palette_find(&p, "catholic", &color) && color == 0x4d99ff);
    assert(map_palette_find(&p, "orthodox", &color) && color == 0x8c5028);
    assert(!map_palette_find(&p, "unknown", NULL));
    /* A second pack replaces duplicate names while retaining the others. */
    assert(map_palette_parse(&p, "catholic = { color = { 1 0 0 } }") &&
           map_palette_find(&p, "catholic", &color) && color == 0xff0000);
    map_palette_free(&p);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb"); assert(f);
        assert(fseek(f, 0, SEEK_END) == 0);
        long size = ftell(f); assert(size > 0 && size < 10000000);
        rewind(f);
        char *data = malloc((size_t)size + 1); assert(data);
        assert(fread(data, 1, (size_t)size, f) == (size_t)size);
        data[size] = 0;
        fclose(f);
        assert(map_palette_parse(&p, data));
        assert(p.count > 25);
        puts("original CK2 palette definitions parsed");
        free(data);
    }
    map_palette_free(&p);
    puts("map palette core: all assertions passed");
}
