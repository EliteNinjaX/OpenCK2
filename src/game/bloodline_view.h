#ifndef OPENCK2_BLOODLINE_VIEW_H
#define OPENCK2_BLOODLINE_VIEW_H

#include "bloodline.h"
#include "setup_toolbar.h"
#include "assets.h"

/* Browses CK2's authored bloodline definitions. Membership / founders are
 * intentionally NOT inferred from the definition keys. */
#define BLOODLINE_VIEW_ROWS 6
struct bloodline_view {
    ck2_bloodline_registry definitions;
    struct setup_widget background, close, symbols[BLOODLINE_VIEW_ROWS];
    SDL_Rect panel, list, title, owner, close_area, rows[BLOODLINE_VIEW_ROWS];
    size_t first, selected;
    bool open, loaded;
    char owner_name[128];
};

int bloodline_view_load(struct bloodline_view *, SDL_Renderer *, const char *, SDL_Point);
void bloodline_view_draw(const struct bloodline_view *, SDL_Renderer *, const struct menu_assets *,
                         SDL_Point, void (*)(SDL_Renderer *, const struct menu_assets *, const char *, size_t, int, int));
bool bloodline_view_click(struct bloodline_view *, SDL_Point);
bool bloodline_view_scroll(struct bloodline_view *, int, SDL_Point,
                           SDL_Renderer *, const char *);
void bloodline_view_free(struct bloodline_view *);
#endif
