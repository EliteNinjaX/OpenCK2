#ifndef CK2_GAME_ASSETS_H
#define CK2_GAME_ASSETS_H

#include "../open_launcher/catalog.h"
#include <SDL.h>
#include <stdbool.h>

struct glyph {
    SDL_Rect area;
    int x_offset, y_offset, advance;
};

struct menu_assets {
    SDL_Texture *title, *logo, *top, *bottom, *button, *button_hover, *credits_frame, *font;
    SDL_Texture *bottom_content, *compact_button, *compact_hover, *version_plate;
    SDL_Texture *dialog_frame, *dialog_heading;
    SDL_Texture *hud_ruler, *hud_resources_left, *hud_resources_right, *hud_date;
    SDL_Texture *hud_minimap, *hud_map, *hud_province, *hud_outliner, *hud_arms;
    SDL_Texture *lobby_row, *lobby_tooltip, *religion_icons;
    SDL_Texture *holding_art[6], *holding_frame, *holding_capital;
    SDL_Texture *hud_tabs[8];
    SDL_Texture *dlc_icons[CK2_DLC_ICON_COUNT];
    struct glyph glyphs[256];
    char *credits;
};

/* Takes ownership of the stream. */
SDL_Surface *ck2_load_dds_stream(SDL_RWops *file);
SDL_Surface *ck2_load_dds_surface(const char *root, const char *relative);
SDL_Texture *ck2_load_texture(SDL_Renderer *renderer, const char *root, const char *relative);
SDL_Texture *ck2_load_hover(SDL_Renderer *renderer, const char *root, const char *relative);
int ck2_load_font_metrics(struct glyph glyphs[256], const char *root, const char *relative);
int ck2_load_glyphs(struct glyph glyphs[256], const char *root);

int menu_assets_load(struct menu_assets *assets, SDL_Renderer *renderer, const char *root);
void menu_assets_free(struct menu_assets *assets);

#endif
