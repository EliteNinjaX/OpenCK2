#ifndef OPENCK2_BLOODLINE_H
#define OPENCK2_BLOODLINE_H
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CK2_BLOOD_ALL_DESCENDANTS = 0,
    CK2_BLOOD_MATRILINEAL = 1,
    CK2_BLOOD_PATRILINEAL = 2,
    CK2_BLOOD_NO_INHERITANCE = 3
} ck2_blood_inheritance;

typedef struct {
    char *key, *picture, *script; /* script retains unprocessed modifiers/triggers */
    ck2_blood_inheritance inheritance;
    bool matrilineal_override, allow_bastards;
} ck2_bloodline_type;

typedef struct {
    ck2_bloodline_type *types;
    size_t count, capacity;
} ck2_bloodline_registry;

/* Loads all the common/bloodlines directory via OpenCK2's mounted asset system.
   Returns false on file/read/allocation errors; previously loaded types remain. */
bool ck2_bloodlines_load(ck2_bloodline_registry *registry, const char *root);
/* Pure CK2-script parser entrypoint for validation/fuzzing without SDL assets. */
bool ck2_bloodlines_parse_text(ck2_bloodline_registry *registry, const char *text);
void ck2_bloodlines_free(ck2_bloodline_registry *registry);
const ck2_bloodline_type *ck2_bloodline_find(const ck2_bloodline_registry *registry, const char *key);

/* Membership is separate from the definition. Character IDs come from game state.
   The caller owns the instance and supplies parent membership/bastardy/marriage state. */
typedef struct {
    const ck2_bloodline_type *type;
    unsigned founder_id;
    unsigned *members;
    size_t count, capacity;
} ck2_bloodline;

bool ck2_bloodline_has_member(const ck2_bloodline *line, unsigned character_id);
bool ck2_bloodline_add_member(ck2_bloodline *line, unsigned character_id);
void ck2_bloodline_remove_member(ck2_bloodline *line, unsigned character_id);
void ck2_bloodline_free(ck2_bloodline *line);
/* Basic rule implementation from common/bloodlines/bloodlines.info.
   'matrilineal_marriage' indicates the parents' marriage, not child's sex.
   Historical edge cases and legitimacy must be validated against CK2. */
bool ck2_bloodline_child_inherits(const ck2_bloodline *line,
    unsigned father_id, unsigned mother_id, bool father_known,
    bool matrilineal_marriage, bool child_is_bastard);
#endif
