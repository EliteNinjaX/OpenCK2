#ifndef CK2_LAUNCHER_LIFECYCLE_H
#define CK2_LAUNCHER_LIFECYCLE_H

#include <stddef.h>
#include <stdint.h>

enum ck2_launcher_action {
    CK2_LAUNCHER_PLAY = 0,
    CK2_LAUNCHER_CONTINUE = 1,
    CK2_LAUNCHER_QUIT = 2
};

enum ck2_launcher_action ck2_launcher_decode_result(int result);

struct ck2_launcher_save_candidate {
    const char *filename;
    int64_t modified_at;
};

/* Candidates must have passed the game's save checks, in enumeration order. */
const char *ck2_launcher_latest_save(const struct ck2_launcher_save_candidate *saves,
                                     size_t count);

#endif
