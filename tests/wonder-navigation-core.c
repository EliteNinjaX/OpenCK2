#include "../src/game/wonder_navigation_core.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    const unsigned ids[] = {3, 8, 17};
    assert(wonder_navigation_index(NULL,0,0,1)==0);
    assert(wonder_navigation_index(ids,3,3,0)==0);
    assert(wonder_navigation_index(ids,3,3,1)==1);
    assert(wonder_navigation_index(ids,3,17,1)==0);
    assert(wonder_navigation_index(ids,3,3,-1)==2);
    assert(wonder_navigation_index(ids,3,81,1)==0);
    puts("great works navigation: all assertions passed");
    return 0;
}
