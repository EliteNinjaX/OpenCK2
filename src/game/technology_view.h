#ifndef OPENCK2_TECHNOLOGY_VIEW_H
#define OPENCK2_TECHNOLOGY_VIEW_H

#include "technology_core.h"
#include "setup_toolbar.h"
#include "assets.h"

struct technology_view {
    bool open, loaded;
    struct ck2_technology_catalog catalog;
    struct setup_widget background, close, group_icons[CK2_TECH_GROUP_COUNT];
    SDL_Rect panel, close_area, headings[CK2_TECH_GROUP_COUNT],
             lists[CK2_TECH_GROUP_COUNT], entry_template, title;
    char labels[CK2_TECH_MAX][128];
};

int technology_view_load(struct technology_view *, SDL_Renderer *, const char *root, SDL_Point viewport);
void technology_view_draw(const struct technology_view *, SDL_Renderer *,
                          const struct menu_assets *, SDL_Point,
                          void (*draw_text)(SDL_Renderer *, const struct menu_assets *,
                                            const char *, size_t, int, int));
/* True when a left click was consumed. */
bool technology_view_click(struct technology_view *, SDL_Point);
void technology_view_free(struct technology_view *);
#endif
