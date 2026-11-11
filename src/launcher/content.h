#ifndef CK2_LAUNCHER_CONTENT_H
#define CK2_LAUNCHER_CONTENT_H

#include <stdbool.h>
#include <stddef.h>

struct ck2_launcher_descriptor_list {
    const char *const *paths;
    size_t count;
};

struct ck2_launcher_content_settings {
    struct ck2_launcher_descriptor_list last_dlcs;
    struct ck2_launcher_descriptor_list last_mods;
};

/* Resolve saved selections for descriptors the game has already admitted. */
bool ck2_launcher_dlc_enabled(const struct ck2_launcher_content_settings *settings,
                             const char *descriptor);
bool ck2_launcher_mod_enabled(const struct ck2_launcher_content_settings *settings,
                             const char *descriptor);

#endif
