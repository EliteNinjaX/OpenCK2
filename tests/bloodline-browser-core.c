#include "../src/game/bloodline_browser_core.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    assert(bloodline_browser_page(0,0,-1)==0);
    assert(bloodline_browser_page(0,4,-1)==0);
    assert(bloodline_browser_page(0,8,-1)==2);
    assert(bloodline_browser_page(2,8,-1)==2);
    assert(bloodline_browser_page(2,8,1)==0);
    assert(bloodline_browser_page(0,9,-1)==2);
    assert(bloodline_browser_page(100,9,1)==2);
    assert(bloodline_browser_page(0,10,-1)==2);
    assert(bloodline_browser_page(2,10,-1)==4);
    puts("bloodline browser: all assertions passed");
    return 0;
}
