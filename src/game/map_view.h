#ifndef OPENCK2_MAP_VIEW_H
#define OPENCK2_MAP_VIEW_H

#include <SDL.h>
#include <stdbool.h>

struct map_water;
struct layers;

struct map_view {
    SDL_Texture *texture;
    struct map_water *water;
    struct layers *layers;
    float center_x, center_y, zoom;
    unsigned revision;
    Uint32 *samples;
    int width, height, world_width, world_height;
};

int map_view_build(struct map_view *view, SDL_Renderer *renderer, const char *root,
                   int width, int height, bool campaign);
bool map_view_move(struct map_view *, float dx, float dy, int zoom_steps);
bool map_view_animate(struct map_view *view, Uint64 now);
void map_view_free(struct map_view *view);
bool map_view_pick(const struct map_view *view, int x, int y, SDL_Point *world);
/* Same perspective/terrain heightfield as the main campaign map. */
bool map_view_project_ground(const struct map_view *, float world_x, float world_y, SDL_Point *screen);

#endif
