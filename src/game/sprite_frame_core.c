#include "sprite_frame_core.h"

bool ck2_sprite_frame_rect(int sheet_width, int sheet_height, int frame_count,
                           int index, struct ck2_frame_rect *out)
{
    if (!out || sheet_width < 1 || sheet_height < 1 || frame_count < 1 ||
        frame_count > sheet_width || sheet_width % frame_count ||
        index < 0 || index >= frame_count)
        return false;
    const int frame_width = sheet_width / frame_count;
    *out = (struct ck2_frame_rect){frame_width * index, 0, frame_width, sheet_height};
    return true;
}
