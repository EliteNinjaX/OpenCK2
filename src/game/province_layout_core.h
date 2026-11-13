#ifndef OPENCK2_PROVINCE_LAYOUT_CORE_H
#define OPENCK2_PROVINCE_LAYOUT_CORE_H

#include <stdbool.h>
#include <stddef.h>

/* Positions are logical CK2 GUI coordinates, before SDL viewport scaling.
 * The original province GUI deliberately declares settlement boxes wider than
 * the visible background: only entries that fit inside the panel may draw. */
struct province_layout_rect { int x, y, w, h; };
struct province_layout {
    struct province_layout_rect panel;
    struct province_layout_rect capital;
    struct province_layout_rect rows[2];
    int slot_width, slot_height, columns;
};

bool province_layout_init(struct province_layout *, struct province_layout_rect panel,
                          struct province_layout_rect capital,
                          const struct province_layout_rect rows[2], int slot_width, int slot_height);
bool province_layout_slot(const struct province_layout *, size_t slot,
                          struct province_layout_rect *out);

#endif
