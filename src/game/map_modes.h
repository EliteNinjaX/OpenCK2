#ifndef OPENCK2_MAP_MODES_H
#define OPENCK2_MAP_MODES_H

#include <stdbool.h>
#include <stddef.h>

/* Identifiers and shortcuts from interface/main.gui, not guessed enum values
 * from the executable. Nine historical-data modes now have color overlays; others still need simulation. */
enum map_mode_id {
    MAP_MODE_TERRAIN, MAP_MODE_REALMS, MAP_MODE_DIPLOMACY, MAP_MODE_RELIGION,
    MAP_MODE_CULTURE, MAP_MODE_ECONOMY, MAP_MODE_DUCHIES, MAP_MODE_KINGDOMS,
    MAP_MODE_EMPIRES, MAP_MODE_REVOLT_RISK, MAP_MODE_DYNASTIES, MAP_MODE_OPINIONS,
    MAP_MODE_DIRECT_VASSALS, MAP_MODE_TRADE_ZONES, MAP_MODE_GOVERNMENTS,
    MAP_MODE_COALITIONS, MAP_MODE_EPIDEMICS, MAP_MODE_PLAYERS, MAP_MODE_COUNT
};

struct map_mode_definition {
    const char *widget;
    const char *label;
    char shortcut;
    bool implemented;
};

struct map_mode_state { enum map_mode_id selected; };

const struct map_mode_definition *map_mode_definition(enum map_mode_id);
int map_mode_shortcut(char key);
bool map_mode_select(struct map_mode_state *, enum map_mode_id);

#endif
