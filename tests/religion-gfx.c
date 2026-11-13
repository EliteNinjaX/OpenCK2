#include "../src/game/religion_gfx_core.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    const char *source = "christian = { graphical_culture = westerngfx "
       "catholic = { icon = 1 } orthodox = { graphical_culture = byzantinegfx icon = 2 } } "
       "muslim = { sunni = { icon=1 } graphical_culture = muslimgfx }";
    char style[64];
    assert(religion_gfx_from_text(source,"catholic",style,sizeof style));
    assert(!strcmp(style,"westerngfx"));
    assert(religion_gfx_from_text(source,"orthodox",style,sizeof style));
    assert(!strcmp(style,"byzantinegfx"));
    assert(religion_gfx_from_text(source,"sunni",style,sizeof style));
    assert(!strcmp(style,"muslimgfx"));
    assert(!religion_gfx_from_text(source,"unknown",style,sizeof style));
    puts("religion gfx: all assertions passed");
}
