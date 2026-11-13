#include "political_map_core.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const int realms[] = {1,1,2, 1,1,2, -1,1,2};
    const uint32_t provinces[] = {11,12,13, 11,12,13, 0,12,13};
    assert(political_map_edge(realms,provinces,3,3,0,0) == POLITICAL_COUNTY_EDGE);
    assert(political_map_edge(realms,provinces,3,3,1,1) == POLITICAL_REALM_EDGE);
    assert(political_map_edge(realms,provinces,3,3,0,2) == POLITICAL_NO_EDGE);
    assert(political_map_edge(realms,provinces,3,3,0,3) == POLITICAL_NO_EDGE);
    assert(political_map_pixel(0xff0000,POLITICAL_REALM_EDGE) !=
           political_map_pixel(0xff0000,POLITICAL_COUNTY_EDGE));
    puts("political map: all assertions passed");
}
