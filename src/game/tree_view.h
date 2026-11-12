#ifndef OPENCK2_TREE_VIEW_H
#define OPENCK2_TREE_VIEW_H
#include "character_view.h"
#include "tree_history.h"

enum tree_mode {TREE_DYNASTY, TREE_FAMILY, TREE_REALM};
enum { TREE_VISIBLE_LIMIT = 64 };
struct tree_node {
    unsigned id;
    char name[96], title[96];
    int column, row, parent, other_parent, spouse; /* node indices; -1 if absent */
    int world_x, world_y;
    SDL_Rect area;
    SDL_Texture *portrait;
};
struct tree_view {
    bool open;
    enum tree_mode mode;
    struct setup_widget background, decorative, close, banner, shield_frame, member_frame;
    struct tree_node nodes[TREE_VISIBLE_LIMIT];
    size_t count, total, living;
    SDL_Rect area, shield_area, viewport;
    int scroll_x, scroll_y, world_width, world_height;
    SDL_Texture *shield;
    char heading[128];
};

/* Open one original CK2 tree window from the inspected ruler. C-only, limited
 * graph layout (no CK2 simulation of unrecorded characters or liege changes). */
int tree_view_open(struct tree_view *, enum tree_mode, SDL_Renderer *, const char *root,
                   const struct character_view *, const struct realm_map *, struct campaign_date,
                   character_portrait_fn render, void *portrait_context);
void tree_view_close(struct tree_view *);
/* Wheel / keyboard navigation through larger historical relationship graphs. */
void tree_view_scroll(struct tree_view *, int dx, int dy);
void tree_view_draw(const struct tree_view *, SDL_Renderer *, const struct menu_assets *,
                    panel_text_fn);
/* 0 = no action, UINT_MAX = request close, otherwise a character ID. */
unsigned tree_view_click(const struct tree_view *, SDL_Point);
#endif
