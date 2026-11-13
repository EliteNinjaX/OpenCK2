#include "../src/game/map_theme_core.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
int main(void)
{
    assert(map_theme_key_color(NULL) == 0);
    assert(map_theme_key_color("") == 0);
    assert(map_theme_key_color("catholic") == map_theme_key_color("catholic"));
    assert(map_theme_key_color("catholic") != map_theme_key_color("orthodox"));
    assert(map_theme_economy_color(1, true) != map_theme_economy_color(60, true));
    assert(map_theme_economy_color(60, true) == map_theme_economy_color(100, true));
    assert(map_theme_economy_color(3, false) == 0);
    assert(map_theme_economy_color(NAN, true) == 0);
    puts("map theme core: all assertions passed");
    return 0;
}
