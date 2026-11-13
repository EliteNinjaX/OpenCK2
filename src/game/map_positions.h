#ifndef OPENCK2_MAP_POSITIONS_H
#define OPENCK2_MAP_POSITIONS_H
#include <stdbool.h>
#include <stddef.h>

struct map_position { float x, y; bool available, authored_port; };
struct map_positions { struct map_position *by_id; size_t capacity, count; };
/* Reads first authored placement point in each map/positions.txt province block.
 * The caller validates Y orientation and on-province occupancy at runtime. */
bool map_positions_parse(struct map_positions *, const char *contents);
const struct map_position *map_position_find(const struct map_positions *, unsigned province_id);
void map_positions_destroy(struct map_positions *);
#endif
