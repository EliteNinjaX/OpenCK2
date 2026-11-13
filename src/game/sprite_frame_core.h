#ifndef OPENCK2_SPRITE_FRAME_CORE_H
#define OPENCK2_SPRITE_FRAME_CORE_H

#include <stdbool.h>

/* Render one cell from a horizontal CK2 GUI sprite strip. Standalone, SDL-free
 * implementation so the atlas geometry can be tested even without SDL. */
struct ck2_frame_rect { int x, y, w, h; };

bool ck2_sprite_frame_rect(int sheet_width, int sheet_height, int frame_count,
                           int index, struct ck2_frame_rect *out);

#endif
