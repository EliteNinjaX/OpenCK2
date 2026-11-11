#include "content.h"
#include "lifecycle.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    assert(ck2_launcher_decode_result(0) == CK2_LAUNCHER_PLAY);
    assert(ck2_launcher_decode_result(1) == CK2_LAUNCHER_CONTINUE);
    assert(ck2_launcher_decode_result(2) == CK2_LAUNCHER_QUIT);
    assert(ck2_launcher_decode_result(3) == CK2_LAUNCHER_PLAY);

    const struct ck2_launcher_save_candidate saves[] = {
        {"unreadable.ck2", -1},
        {"older.ck2", 100},
        {"newer.ck2", 200},
        {"same-time.ck2", 200},
    };
    assert(ck2_launcher_latest_save(NULL, 0) == NULL);
    assert(ck2_launcher_latest_save(saves, 1) == NULL);
    assert(strcmp(ck2_launcher_latest_save(saves, 4), "newer.ck2") == 0);

    const struct ck2_launcher_content_settings empty = {0};
    assert(ck2_launcher_dlc_enabled(&empty, "dlc/dlc074.dlc"));
    assert(!ck2_launcher_mod_enabled(&empty, "mod/codex_probe.mod"));

    const char *disabled_dlcs[] = {"dlc/dlc074.dlc"};
    const char *enabled_mods[] = {"mod/codex_probe.mod"};
    const struct ck2_launcher_content_settings settings = {
        .last_dlcs = {disabled_dlcs, 1},
        .last_mods = {enabled_mods, 1},
    };
    assert(!ck2_launcher_dlc_enabled(&settings, "dlc/dlc074.dlc"));
    assert(ck2_launcher_dlc_enabled(&settings, "dlc/dlc073.dlc"));
    assert(ck2_launcher_mod_enabled(&settings, "mod/codex_probe.mod"));
    assert(!ck2_launcher_mod_enabled(&settings, "mod/codex_probe_other.mod"));
}
