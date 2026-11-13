#include "../src/game/province_layout_core.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const struct province_layout_rect panel = {0, 276, 344, 624};
    const struct province_layout_rect capital = {193, 362, 111, 111};
    const struct province_layout_rect rows[2] = {{6, 676, 520, 128}, {6, 784, 520, 128}};
    struct province_layout layout;
    assert(province_layout_init(&layout, panel, capital, rows, 109, 109));
    assert(layout.columns == 3); /* A wider GUI list is clipped to the panel. */
    const struct province_layout_rect expected[] = {
        {193,362,109,109}, {6,676,109,109}, {115,676,109,109},
        {224,676,109,109}, {6,784,109,109}, {115,784,109,109},
        {224,784,109,109},
    };
    for (size_t i = 0; i < sizeof expected / sizeof expected[0]; ++i) {
        struct province_layout_rect cell;
        assert(province_layout_slot(&layout, i, &cell));
        assert(cell.x == expected[i].x && cell.y == expected[i].y);
    }
    struct province_layout_rect overflow;
    assert(!province_layout_slot(&layout, 7, &overflow));
    /* Do not draw beyond the actual background on a narrower installation. */
    assert(province_layout_init(&layout, (struct province_layout_rect){0,276,274,624},
                                capital, rows, 109,109));
    assert(!province_layout_slot(&layout, 0, &overflow));
    assert(layout.columns == 2);
    assert(!province_layout_slot(&layout, 5, &overflow));
    puts("province layout: all assertions passed");
}
