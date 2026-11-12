#include "../src/game/map_interaction.h"
#include "../src/game/map_modes.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct map_gesture g = {0};
    int dx = 999, dy = 999;
    assert(map_gesture_end(&g, 3, 3, 7, 4, &dx, &dy) == MAP_GESTURE_NONE);
    assert(dx == 0 && dy == 0);
    map_gesture_begin(&g, 3, 20, 30);
    assert(map_gesture_end(&g, 3, 22, 32, 4, &dx, &dy) == MAP_GESTURE_CLICK);
    assert(!g.active);
    map_gesture_begin(&g, 3, 20, 30);
    assert(map_gesture_end(&g, 3, 31, 26, 4, &dx, &dy) == MAP_GESTURE_DRAG);
    assert(dx == -11 && dy == 4);
    map_gesture_begin(&g, 3, 20, 30);
    assert(map_gesture_end(&g, 2, 20, 30, 4, &dx, &dy) == MAP_GESTURE_NONE);
    assert(g.active);
    map_gesture_cancel(&g);
    assert(!g.active);
    struct map_mode_state mode = {.selected = MAP_MODE_TERRAIN};
    for (int i = 0; i < MAP_MODE_COUNT; ++i) {
        const struct map_mode_definition *d = map_mode_definition(i);
        assert(d && d->widget && d->label && d->shortcut);
        assert(map_mode_shortcut(d->shortcut) == i);
        assert(!strcmp(map_mode_definition(i)->widget, d->widget));
        if (!d->implemented) {
            assert(!map_mode_select(&mode, i));
            assert(mode.selected == MAP_MODE_TERRAIN);
        }
    }
    assert(map_mode_select(&mode, MAP_MODE_REALMS));
    assert(mode.selected == MAP_MODE_REALMS);
    assert(!map_mode_select(&mode, -1));
    assert(!map_mode_select(&mode, MAP_MODE_COUNT));
    puts("map core: all assertions passed");
}
