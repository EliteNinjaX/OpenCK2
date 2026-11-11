#include "startup.h"

#include <assert.h>

int main(void)
{
    assert(ck2_launcher_bypass_token("startgame"));
    assert(ck2_launcher_bypass_token("skiplauncher"));
    assert(ck2_launcher_bypass_token("nolauncher"));
    assert(!ck2_launcher_bypass_token(""));
    assert(!ck2_launcher_bypass_token("-skiplauncher"));
    assert(!ck2_launcher_bypass_token("SKIPLAUNCHER"));
    assert(!ck2_launcher_bypass_token("skiplauncherextra"));
    assert(!ck2_launcher_bypass_token("overnight"));

    for (unsigned inputs = 0; inputs < 8; ++inputs) {
        bool show = ck2_launcher_should_show(inputs & 4, inputs & 2, inputs & 1);
        assert(show == (inputs != 4));
    }
}
