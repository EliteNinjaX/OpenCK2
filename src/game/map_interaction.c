#include "map_interaction.h"
#include <stdint.h>

void map_gesture_begin(struct map_gesture *gesture, unsigned button, int x, int y)
{
    if (!gesture) return;
    *gesture = (struct map_gesture){.x = x, .y = y, .button = button, .active = true};
}

void map_gesture_cancel(struct map_gesture *gesture)
{
    if (gesture) *gesture = (struct map_gesture){0};
}

enum map_gesture_result map_gesture_end(struct map_gesture *gesture, unsigned button,
                                        int x, int y, int drag_threshold,
                                        int *delta_x, int *delta_y)
{
    if (delta_x) *delta_x = 0;
    if (delta_y) *delta_y = 0;
    if (!gesture || !gesture->active || gesture->button != button)
        return MAP_GESTURE_NONE;
    const int64_t dx = (int64_t)gesture->x - x, dy = (int64_t)gesture->y - y;
    map_gesture_cancel(gesture);
    if (drag_threshold < 0) drag_threshold = 0;
    if (dx * dx + dy * dy <= (int64_t)drag_threshold * drag_threshold)
        return MAP_GESTURE_CLICK;
    if (delta_x) *delta_x = (int)dx;
    if (delta_y) *delta_y = (int)dy;
    return MAP_GESTURE_DRAG;
}
