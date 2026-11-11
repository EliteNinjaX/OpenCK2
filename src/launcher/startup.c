#include "startup.h"

#include <string.h>

/* CK2game.exe RVA 0x80f6d2–0x80f73e */
bool ck2_launcher_bypass_token(const char *token)
{
    return strcmp(token, "startgame") == 0
        || strcmp(token, "skiplauncher") == 0
        || strcmp(token, "nolauncher") == 0;
}

/* CK2game.exe RVA 0x81044c–0x810466 */
bool ck2_launcher_should_show(bool bypass_requested, bool global_override,
                              bool startup_check_override)
{
    return !bypass_requested || global_override || startup_check_override;
}
