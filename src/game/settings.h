#ifndef OPENCK2_SETTINGS_H
#define OPENCK2_SETTINGS_H

#include "assets.h"
#include "music.h"

struct settings_values {
    int resolution, borderless, master, music, effects, language;
};

struct settings {
    struct settings_values applied, draft;
    SDL_Texture *sprites[5];
    SDL_Point pointer;
    int tab, focus, dragging;
    const char *asset_root;
};

typedef void (*settings_text)(SDL_Renderer *, const struct menu_assets *,
                              const char *, size_t, int, int);

int settings_load(struct settings *, SDL_Renderer *, const char *root);
void settings_free(struct settings *);
void settings_open(struct settings *, SDL_Window *);
void settings_draw(const struct settings *, SDL_Renderer *, const struct menu_assets *,
                   settings_text);
bool settings_event(struct settings *, SDL_Window *, struct menu_music *, const SDL_Event *);
void settings_unavailable_tooltip(SDL_Renderer *, const struct menu_assets *, settings_text,
                                  SDL_Point pointer);

#endif
