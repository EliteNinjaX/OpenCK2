#include "map_modes.h"

static const struct map_mode_definition definitions[MAP_MODE_COUNT] = {
    [MAP_MODE_TERRAIN] = {"mapmode_terrain", "Terrain", 'q', true},
    [MAP_MODE_REALMS] = {"mapmode_realms", "Realms", 'w', true},
    [MAP_MODE_DIPLOMACY] = {"mapmode_diplo", "Diplomacy", 'e', false},
    [MAP_MODE_RELIGION] = {"mapmode_religion", "Religion", 'r', false},
    [MAP_MODE_CULTURE] = {"mapmode_culture", "Culture", 't', false},
    [MAP_MODE_ECONOMY] = {"mapmode_economy", "Economy", 'u', false},
    [MAP_MODE_DUCHIES] = {"mapmode_duchies", "De jure duchies", 'i', false},
    [MAP_MODE_KINGDOMS] = {"mapmode_kingdoms", "De jure kingdoms", 'o', false},
    [MAP_MODE_EMPIRES] = {"mapmode_dejureempires", "De jure empires", 'p', false},
    [MAP_MODE_REVOLT_RISK] = {"mapmode_revoltrisk", "Revolt risk", 'a', false},
    [MAP_MODE_DYNASTIES] = {"mapmode_dynasties", "Dynasties", 's', false},
    [MAP_MODE_OPINIONS] = {"mapmode_opinions", "Opinions", 'd', false},
    [MAP_MODE_DIRECT_VASSALS] = {"mapmode_directvassals", "Direct vassals", 'f', false},
    [MAP_MODE_TRADE_ZONES] = {"mapmode_trade_zones", "Trade zones", 'g', false},
    [MAP_MODE_GOVERNMENTS] = {"mapmode_governments", "Governments", 'h', false},
    [MAP_MODE_COALITIONS] = {"mapmode_coalitions", "Coalitions", 'j', false},
    [MAP_MODE_EPIDEMICS] = {"mapmode_epidemics", "Epidemics", 'k', false},
    [MAP_MODE_PLAYERS] = {"mapmode_players", "Players", 'l', false},
};

const struct map_mode_definition *map_mode_definition(enum map_mode_id id)
{
    return id >= 0 && id < MAP_MODE_COUNT ? &definitions[id] : NULL;
}

int map_mode_shortcut(char key)
{
    for (int i = 0; i < MAP_MODE_COUNT; ++i)
        if (definitions[i].shortcut == key)
            return i;
    return -1;
}

bool map_mode_select(struct map_mode_state *state, enum map_mode_id id)
{
    const struct map_mode_definition *definition = map_mode_definition(id);
    if (!state || !definition || !definition->implemented)
        return false;
    state->selected = id;
    return true;
}
