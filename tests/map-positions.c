#include "../src/game/map_positions.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    struct map_positions map={0};
    assert(map_positions_parse(&map,
       "# province positions\n1={position={42.125 77.5 999.9 999.9}rotation={0 0}}\n"
       "17={rotation={90 90}position={8.5 9.5}}\n19={height={0}}\n"));
    assert(map.count==2);
    assert(map_position_find(&map,1)->x==42.125f);
    assert(map_position_find(&map,17)->y==9.5f);
    assert(!map_position_find(&map,19));
    assert(!map_position_find(&map,1000));
    map_positions_destroy(&map);
    puts("map positions: all assertions passed");
    return 0;
}
