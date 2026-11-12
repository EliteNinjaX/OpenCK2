#ifndef OPENCK2_TECHNOLOGY_CORE_H
#define OPENCK2_TECHNOLOGY_CORE_H

#include <stddef.h>

/* Technology definitions, not researched levels or technology point totals.
 * Names and grouping come from common/technology.txt (including mod overrides). */
enum ck2_tech_group { CK2_TECH_MILITARY, CK2_TECH_ECONOMY, CK2_TECH_CULTURE, CK2_TECH_GROUP_COUNT };
enum { CK2_TECH_MAX = 96 };
struct ck2_technology {
    char key[96];
    enum ck2_tech_group group;
};
struct ck2_technology_catalog {
    struct ck2_technology items[CK2_TECH_MAX];
    size_t count;
};
/* Returns 0 for a valid definition set, -1 for malformed or truncated input. */
int ck2_technology_parse(struct ck2_technology_catalog *out, const char *text);
size_t ck2_technology_group_count(const struct ck2_technology_catalog *, enum ck2_tech_group);
const struct ck2_technology *ck2_technology_find(const struct ck2_technology_catalog *, const char *key);
#endif
