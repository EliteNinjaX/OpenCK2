#include "tree_layout_core.h"
#include <limits.h>

static int clamp_int(int x, int low, int high)
{
    if (x < low) return low;
    if (x > high) return high;
    return x;
}

struct tree_layout_bounds tree_layout_build(struct tree_layout_item *items,
                                           size_t count, int columns)
{
    if (columns < 1) columns = 1;
    if (columns > TREE_LAYOUT_COLUMNS) columns = TREE_LAYOUT_COLUMNS;
    int rows[TREE_LAYOUT_COLUMNS] = {0};
    int max_rows = 0;
    for (size_t i = 0; i < count; ++i) {
        int col = clamp_int(items[i].column, 0, columns - 1);
        items[i].column = col;
        items[i].x = 24 + col * TREE_LAYOUT_COLUMN_STEP;
        items[i].y = 24 + rows[col] * TREE_LAYOUT_ROW_STEP;
        if (++rows[col] > max_rows) max_rows = rows[col];
    }
    return (struct tree_layout_bounds){
        48 + (columns - 1) * TREE_LAYOUT_COLUMN_STEP + TREE_LAYOUT_NODE_W,
        48 + (max_rows ? (max_rows - 1) * TREE_LAYOUT_ROW_STEP + TREE_LAYOUT_NODE_H : 0)
    };
}

void tree_layout_scroll(int *x, int *y, int dx, int dy,
                        struct tree_layout_bounds world, int viewport_w, int viewport_h)
{
    if (!x || !y) return;
    const int limit_x = world.width > viewport_w ? world.width - viewport_w : 0;
    const int limit_y = world.height > viewport_h ? world.height - viewport_h : 0;
    /* Use a wide intermediary to prevent signed overflow when scrolling to edges. */
    long long nx = (long long)*x + dx;
    long long ny = (long long)*y + dy;
    *x = (int)(nx < 0 ? 0 : nx > limit_x ? limit_x : nx);
    *y = (int)(ny < 0 ? 0 : ny > limit_y ? limit_y : ny);
}
