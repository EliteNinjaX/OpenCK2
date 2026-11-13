#ifndef OPENCK2_MAP_PALETTE_CORE_H
#define OPENCK2_MAP_PALETTE_CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct map_palette_entry { char id[96]; uint32_t rgb; };
struct map_palette {
    struct map_palette_entry *entries;
    size_t count, capacity;
};
/* Reads authored 'color = { r g b }' values nested in CK2 religion/culture
 * blocks. Channel values are decimal 0..1 (or integers 0..255). */
bool map_palette_parse(struct map_palette *, const char *source);
bool map_palette_find(const struct map_palette *, const char *id, uint32_t *color);
void map_palette_free(struct map_palette *);
#endif
