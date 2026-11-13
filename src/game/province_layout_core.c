#include "province_layout_core.h"

bool province_layout_init(struct province_layout *layout, struct province_layout_rect panel,
                          struct province_layout_rect capital,
                          const struct province_layout_rect rows[2], int slot_width, int slot_height)
{
    if (!layout || !rows || panel.w <= 0 || panel.h <= 0 || slot_width <= 0 || slot_height <= 0)
        return false;
    int columns = (panel.x + panel.w - rows[0].x) / slot_width;
    int authored = rows[0].w / slot_width;
    if (authored < columns) columns = authored;
    if (columns <= 0) return false;
    *layout = (struct province_layout){.panel = panel, .capital = capital,
        .rows = {rows[0], rows[1]}, .slot_width = slot_width,
        .slot_height = slot_height, .columns = columns};
    return true;
}

bool province_layout_slot(const struct province_layout *layout, size_t slot,
                          struct province_layout_rect *out)
{
    if (!layout || !out || layout->columns < 1) return false;
    struct province_layout_rect result;
    if (slot == 0) {
        result = layout->capital;
    } else {
        const size_t row = (slot - 1) / (size_t)layout->columns;
        if (row >= 2) return false;
        result = layout->rows[row];
        result.x += (int)((slot - 1) % (size_t)layout->columns) * layout->slot_width;
    }
    result.w = layout->slot_width;
    result.h = layout->slot_height;
    if (result.x < layout->panel.x || result.y < layout->panel.y ||
        result.x + result.w > layout->panel.x + layout->panel.w ||
        result.y + result.h > layout->panel.y + layout->panel.h)
        return false;
    *out = result;
    return true;
}
