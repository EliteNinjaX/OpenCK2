#ifndef OPENCK2_POLITICAL_MAP_CORE_H
#define OPENCK2_POLITICAL_MAP_CORE_H
#include <stddef.h>
#include <stdint.h>
/* Rendered pixel ownership uses independent-title identity, NOT color equality.
 * Two different realms can share CK2's authored RGB value. */
enum political_edge { POLITICAL_NO_EDGE, POLITICAL_COUNTY_EDGE, POLITICAL_REALM_EDGE };
enum political_edge political_map_edge(const int *realm_ids, const uint32_t *province_ids,
                                       size_t width, size_t height, size_t x, size_t y);
/* Approximate color treatment; original Clausewitz border shader is still unknown. */
uint32_t political_map_pixel(uint32_t rgb, enum political_edge edge);
#endif
