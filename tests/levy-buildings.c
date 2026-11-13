/* Exercises the actual construction parser against CK2's building file,
 * rather than a second parser invented for tests. This file intentionally
 * includes its production implementation to access its internal file reader. */
#include "../src/game/construction.c"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        puts("usage: levy-buildings-test path/to/common/buildings/00_Castle.txt");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb"); assert(f);
    assert(fseek(f, 0, SEEK_END) == 0);
    long length = ftell(f); assert(length > 0 && length < 10000000);
    rewind(f);
    char *body = malloc((size_t)length + 1); assert(body);
    assert(fread(body, 1, (size_t)length, f) == (size_t)length);
    body[length] = 0; fclose(f);
    struct construction c = {0};
    assert(read_buildings(&c, body));
    assert(c.count > 20);
    bool militia = false, keep = false;
    for (size_t i = 0; i < c.count; ++i) {
        const struct building *b = &c.catalog[i];
        if (!strcmp(b->id, "ca_militia_barracks_1")) {
            militia = true;
            assert(b->type == HOLDING_CASTLE);
            assert(fabs(b->levy.troops[LEVY_LIGHT_INFANTRY] - 80.) < 0.001);
            assert(fabs(b->levy.troops[LEVY_ARCHERS] - 10.) < 0.001);
        }
        if (!strcmp(b->id, "ca_keep_1")) {
            keep = true;
            assert(fabs(b->levy.levy_size - 0.05) < 0.001);
        }
    }
    assert(militia && keep);
    free(c.catalog);
    free(body);
    puts("original CK2 castle levies: production parser passed");
    return 0;
}
