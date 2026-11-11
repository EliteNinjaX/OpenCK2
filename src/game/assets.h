#ifndef CK2_GAME_ASSETS_H
#define CK2_GAME_ASSETS_H

#include <SDL.h>
#include <stdbool.h>

struct glyph {
    SDL_Rect area;
    int x_offset, y_offset, advance;
};

struct menu_assets {
    SDL_Texture *map, *title, *logo, *top, *bottom, *button, *button_hover, *credits_frame, *font;
    struct glyph glyphs[256];
    char *credits;
};

SDL_Texture *ck2_load_texture(SDL_Renderer *renderer, const char *root, const char *relative);
SDL_Texture *ck2_load_hover(SDL_Renderer *renderer, const char *root, const char *relative);
int ck2_load_glyphs(struct glyph glyphs[256], const char *root);

int menu_assets_load(struct menu_assets *assets, SDL_Renderer *renderer, const char *root);
void menu_assets_free(struct menu_assets *assets);

#endif
