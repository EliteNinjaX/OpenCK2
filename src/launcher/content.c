#include "content.h"

#include <string.h>

static bool contains_descriptor(struct ck2_launcher_descriptor_list list,
                                const char *descriptor)
{
    for (size_t i = 0; i < list.count; ++i) {
        if (strcmp(list.paths[i], descriptor) == 0)
            return true;
    }
    return false;
}

bool ck2_launcher_dlc_enabled(const struct ck2_launcher_content_settings *settings,
                             const char *descriptor)
{
    return !contains_descriptor(settings->last_dlcs, descriptor);
}

bool ck2_launcher_mod_enabled(const struct ck2_launcher_content_settings *settings,
                             const char *descriptor)
{
    return contains_descriptor(settings->last_mods, descriptor);
}
