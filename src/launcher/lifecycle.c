#include "lifecycle.h"

/* CK2game.exe RVA 0x810725–0x810785. */
enum ck2_launcher_action ck2_launcher_decode_result(int result)
{
    switch (result) {
    case CK2_LAUNCHER_QUIT:
        return CK2_LAUNCHER_QUIT;
    case CK2_LAUNCHER_CONTINUE:
        return CK2_LAUNCHER_CONTINUE;
    default:
        return CK2_LAUNCHER_PLAY;
    }
}

/* CK2game.exe starts with -1 and replaces the winner only on a newer timestamp. */
const char *ck2_launcher_latest_save(const struct ck2_launcher_save_candidate *saves,
                                     size_t count)
{
    const char *latest = NULL;
    int64_t latest_time = -1;
    for (size_t i = 0; i < count; ++i) {
        if (saves[i].modified_at > latest_time) {
            latest = saves[i].filename;
            latest_time = saves[i].modified_at;
        }
    }
    return latest;
}
