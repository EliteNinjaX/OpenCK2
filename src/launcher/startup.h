#ifndef CK2_LAUNCHER_STARTUP_H
#define CK2_LAUNCHER_STARTUP_H

#include <stdbool.h>

/* The caller supplies a token after the game's command-line processing. */
bool ck2_launcher_bypass_token(const char *token);

/* The two overrides are recovered inputs whose producers are still under study. */
bool ck2_launcher_should_show(bool bypass_requested, bool global_override,
                              bool startup_check_override);

#endif
