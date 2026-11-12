#include "../src/game/tree_layout_core.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    struct tree_layout_item many[50];
    for (int i = 0; i < 50; ++i) many[i].column = i % 3;
    struct tree_layout_bounds world = tree_layout_build(many, 50, 3);
    assert(world.height > 450);
    assert(many[0].x == many[3].x && many[0].y < many[3].y);
    assert(many[46].y > many[1].y); /* late vassals are not discarded */
    assert(many[2].x > many[1].x);
    /* An out-of-range column is clamped, never an out-of-bounds rows[] write. */
    struct tree_layout_item bad[] = {{.column=-20}, {.column=999}};
    struct tree_layout_bounds b = tree_layout_build(bad, 2, 2);
    assert(bad[0].column == 0 && bad[1].column == 1);
    assert(b.width > 0 && b.height > 0);
    int x = 0, y = 0;
    tree_layout_scroll(&x, &y, 999999, 999999, world, 450, 400);
    assert(x == world.width - 450);
    assert(y == world.height - 400);
    tree_layout_scroll(&x, &y, -999999, -999999, world, 450, 400);
    assert(x == 0 && y == 0);
    tree_layout_scroll(&x, &y, -2, -2, world, 9999, 9999);
    assert(x == 0 && y == 0);
    puts("tree layout: all assertions passed");
    return 0;
}
