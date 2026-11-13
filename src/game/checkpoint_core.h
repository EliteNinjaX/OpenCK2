#ifndef OPENCK2_CHECKPOINT_CORE_H
#define OPENCK2_CHECKPOINT_CORE_H

#include "campaign.h"
#include "construction.h"
#include "conversion_core.h"
#include "family_gameplay.h"
#include "law_core.h"
#include "realms.h"
#include "warfare_core.h"
#include "government_mechanics_core.h"

/* OpenCK2 native developer checkpoint (not a CK2 .ck2 save).
 * A writer may use read-only shallow views of existing subsystem states.
 * A successfully decoded checkpoint owns all its allocations, and must be
 * released using checkpoint_free(). Do not checkpoint SDL handles or assets. */
struct openck2_checkpoint {
    struct campaign_date bookmark;
    unsigned bookmark_ruler, player, home_province;
    struct campaign campaign;
    struct family_gameplay family;
    struct conversion_state conversion;
    struct warfare_state warfare;
    struct ck2_law_state laws;
    struct ck2_government_mechanics governments;
    struct construction_checkpoint construction;
    struct realm_title_override *titles;
    size_t title_count;
};

void checkpoint_free(struct openck2_checkpoint *);
/* Atomic replacement: writes a neighboring .tmp and renames only after
 * successful close. No partial checkpoint overwrites the last valid save. */
bool checkpoint_write(const char *path, const struct openck2_checkpoint *);
/* Staged decode: on error `out` is untouched, and any temporary allocations
 * are released. Rejects corrupt, truncated, oversized and future versions. */
bool checkpoint_read(const char *path, struct openck2_checkpoint *out);
const char *checkpoint_last_error(void);

#endif
