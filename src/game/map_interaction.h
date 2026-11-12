#ifndef OPENCK2_MAP_INTERACTION_H
#define OPENCK2_MAP_INTERACTION_H

#include <stdbool.h>

/* This small input state machine deliberately has no SDL dependency.  Both
 * setup and campaign map code can use it without coupling gameplay to UI. */
enum map_gesture_result {
    MAP_GESTURE_NONE,
    MAP_GESTURE_CLICK,
    MAP_GESTURE_DRAG
};

struct map_gesture {
    int x, y;
    unsigned button;
    bool active;
};

void map_gesture_begin(struct map_gesture *, unsigned button, int x, int y);
void map_gesture_cancel(struct map_gesture *);
enum map_gesture_result map_gesture_end(struct map_gesture *, unsigned button,
                                        int x, int y, int drag_threshold,
                                        int *delta_x, int *delta_y);

#endif
