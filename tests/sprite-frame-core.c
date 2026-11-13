#include "../src/game/sprite_frame_core.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    /* Both capital and ordinary CK2 owner frames have four horizontal cells. */
    struct ck2_frame_rect frame = {0};
    for (int i = 0; i < 4; ++i) {
        assert(ck2_sprite_frame_rect(160, 36, 4, i, &frame));
        assert(frame.x == i * 40 && frame.y == 0 && frame.w == 40 && frame.h == 36);
    }
    assert(ck2_sprite_frame_rect(108, 108, 1, 0, &frame));
    assert(frame.w == 108 && frame.h == 108);
    assert(!ck2_sprite_frame_rect(161, 36, 4, 0, &frame));
    assert(!ck2_sprite_frame_rect(160, 36, 0, 0, &frame));
    assert(!ck2_sprite_frame_rect(160, 36, 4, 4, &frame));
    assert(!ck2_sprite_frame_rect(160, 36, 4, -1, &frame));
    assert(!ck2_sprite_frame_rect(0, 36, 4, 0, &frame));
    assert(!ck2_sprite_frame_rect(160, 36, 4, 0, NULL));
    puts("sprite frame core: all assertions passed");
}
