#ifndef OPENCK2_CAMPAIGN_H
#define OPENCK2_CAMPAIGN_H

#include <SDL.h>
#include <stdbool.h>
#include "map_view.h"

enum { YORK_PROVINCE = 57, IMPROVEMENT_COST = 25, IMPROVEMENT_LIMIT = 3, TREASURY_TARGET = 100 };

struct campaign_date {
    int year, month, day;
};

struct campaign {
    struct campaign_date date;
    unsigned treasury;
    unsigned improvements;
};

void campaign_start(struct campaign *campaign);
void campaign_advance_day(struct campaign *campaign);
void campaign_advance_month(struct campaign *campaign);
bool campaign_improve(struct campaign *campaign);
unsigned campaign_monthly_income(const struct campaign *campaign);
bool campaign_goal_met(const struct campaign *campaign);

struct province {
    Uint32 color;
    unsigned id;
    char name[128];
    bool land;
};

struct province_map {
    SDL_Surface *pixels;
    struct province *provinces;
    size_t count;
};

int province_map_load(struct province_map *map, const char *root);
void province_map_free(struct province_map *map);
Uint32 province_map_color(const struct province_map *, int, int);
const struct province *province_at(const struct province_map *map, int x, int y);
const struct province *province_by_id(const struct province_map *map, unsigned id);
SDL_Texture *province_highlight(const struct province_map *map, SDL_Renderer *renderer,
                               const struct province *province, const struct map_view *view);

#endif
