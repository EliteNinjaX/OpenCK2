#ifndef OPENCK2_TREE_LAYOUT_CORE_H
#define OPENCK2_TREE_LAYOUT_CORE_H

#include <stddef.h>

/* Pure C node geometry. Coordinates are in the tree's scrollable content space.
 * The layout contains no graphics or CK2 data assumptions, so it can be tested
 * independently of SDL and reused by other relationship views. */
enum { TREE_LAYOUT_COLUMNS = 5, TREE_LAYOUT_NODE_W = 118,
       TREE_LAYOUT_NODE_H = 70, TREE_LAYOUT_COLUMN_STEP = 154,
       TREE_LAYOUT_ROW_STEP = 90 };
struct tree_layout_item {
    int column;
    int x, y;
};
struct tree_layout_bounds { int width, height; };
struct tree_layout_bounds tree_layout_build(struct tree_layout_item *items,
                                           size_t count, int columns);
void tree_layout_scroll(int *x, int *y, int dx, int dy,
                        struct tree_layout_bounds world, int viewport_w, int viewport_h);
#endif
