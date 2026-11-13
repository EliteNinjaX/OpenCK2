#ifndef OPENCK2_WARFARE_MAP_H
#define OPENCK2_WARFARE_MAP_H
#include "campaign.h"
#include "warfare_core.h"
/* Extract land-border graph from the original CK2 map/provinces.bmp raster.
 * Does NOT include straits, sea routes, or scripted movement restrictions. */
bool warfare_graph_from_map(struct warfare_graph *, const struct province_map *);
#endif
