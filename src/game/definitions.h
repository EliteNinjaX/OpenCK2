#ifndef OPENCK2_DEFINITIONS_H
#define OPENCK2_DEFINITIONS_H
#include <stddef.h>
#include <stdbool.h>
/* Owned, lossless bodies. Trigger blocks are data, not evaluated predicates. */
struct ck2_definition { char *key, *group, *body; };
struct ck2_definitions { struct ck2_definition *items; size_t count; };
/* Parsing is transactional: malformed input leaves the catalog unchanged.
   Later definitions replace the same key. grouped=true for governments. */
bool ck2_definitions_parse(struct ck2_definitions *, const char *, bool grouped);
bool ck2_definitions_load(struct ck2_definitions *, const char *root, bool governments);
const struct ck2_definition *ck2_definition_get(const struct ck2_definitions *, const char *key);
void ck2_definitions_free(struct ck2_definitions *);
/* Explicit values verified in this executable; UNKNOWN includes missing fields.
   These are parser values, not a complete descendant propagation algorithm. */
enum ck2_bloodline_inheritance { CK2_BLOODLINE_UNKNOWN = -1,
    CK2_BLOODLINE_ALL_DESCENDANTS = 0, CK2_BLOODLINE_MATRILINEAL = 1,
    CK2_BLOODLINE_PATRILINEAL = 2, CK2_BLOODLINE_NO_INHERITANCE = 3 };
enum ck2_bloodline_inheritance ck2_bloodline_inheritance(const struct ck2_definition *);
#endif
