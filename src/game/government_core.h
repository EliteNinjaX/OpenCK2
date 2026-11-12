#ifndef OPENCK2_GOVERNMENT_CORE_H
#define OPENCK2_GOVERNMENT_CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Holding-type bits are deliberately independent of SDL and the UI module. */
enum government_holding_bit {
    GOVERNMENT_CASTLE = 1u << 0, GOVERNMENT_CITY = 1u << 1,
    GOVERNMENT_TEMPLE = 1u << 2, GOVERNMENT_TRIBAL = 1u << 3,
    GOVERNMENT_NOMAD = 1u << 4, GOVERNMENT_FORT = 1u << 5,
    GOVERNMENT_HOSPITAL = 1u << 6, GOVERNMENT_TRADE_POST = 1u << 7,
    GOVERNMENT_FAMILY_PALACE = 1u << 8
};
struct government_definition {
    char id[80], group[64], frame_suffix[64], title_prefix[64];
    uint32_t preferred_holdings, allowed_holdings, color_rgb;
    bool color_defined, has_potential, merchant_republic;
    bool allow_looting, can_build_holdings, can_build_holdings_defined;
    unsigned max_consorts;
    double aggression;
    char *potential_script; /* Raw original CK2 trigger block; NOT evaluated. */
};
struct government_catalog {
    struct government_definition *items;
    size_t count, capacity;
};
/* Reads groups and their definitions; later duplicate IDs replace earlier ones.
 * Returns false on allocation, malformed block or count limit failure. */
bool government_catalog_parse(struct government_catalog *, const char *source);
const struct government_definition *government_find(const struct government_catalog *, const char *id);
void government_catalog_destroy(struct government_catalog *);
uint32_t government_holding_mask(const char *key);
/* Not government assignment: returns candidates by *preferred* holding type only. */
size_t government_holding_candidates(const struct government_catalog *, uint32_t preferred,
                                     const struct government_definition **out, size_t capacity);
#endif
