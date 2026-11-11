#ifndef OPENCK2_SETUP_TOOLBAR_H
#define OPENCK2_SETUP_TOOLBAR_H

#include "frontend_gui.h"
#include "assets.h"
#include <SDL.h>
#include <stdbool.h>

enum setup_control {
    SETUP_NO_CONTROL = -1,
    SETUP_TERRAIN, SETUP_REALMS, SETUP_COUNTIES, SETUP_DUCHIES, SETUP_KINGDOMS,
    SETUP_RELIGION, SETUP_CULTURE, SETUP_GOVERNMENTS, SETUP_SHATTERED, SETUP_RANDOM,
    SETUP_LEFT_ORNAMENT, SETUP_RIGHT_ORNAMENT, SETUP_BANNER, SETUP_WIDGET_COUNT
};

struct setup_widget {
    SDL_Texture *texture;
    SDL_Rect area;
    int frames;
    SDL_Keycode shortcut;
};

enum lobby_element {
    LOBBY_LEFT, LOBBY_CHARACTERS, LOBBY_RIGHT, LOBBY_BOTTOM, LOBBY_LOAD,
    LOBBY_BACK, LOBBY_PLAY, LOBBY_DIFFICULTY, LOBBY_DIFFICULTY_LABEL, LOBBY_DIPLOMACY_LABEL,
    LOBBY_BOOKMARKS, LOBBY_FEATURED, LOBBY_VASSALS, LOBBY_BOOKMARK_ROW, LOBBY_BOOKMARK_ART, LOBBY_BOOKMARK_TITLE, LOBBY_BOOKMARK_DATE, LOBBY_CHARACTER_ROW,
    LOBBY_BOOKMARK_LABEL, LOBBY_FEATURED_LABEL, LOBBY_VASSALS_LABEL, LOBBY_VASSALS_AMOUNT,
    LOBBY_DIFFICULTY_TRACK, LOBBY_PORTRAIT_FRAME, LOBBY_CROWN, LOBBY_SHIELD,
    LOBBY_ROW_PORTRAIT, LOBBY_ROW_FRAME, LOBBY_ROW_SHIELD, LOBBY_ROW_NAME,
    LOBBY_RULER, LOBBY_PORTRAIT, LOBBY_DYNASTY, LOBBY_TITLE, LOBBY_STATUS,
    LOBBY_CULTURE, LOBBY_RELIGION, LOBBY_DESCRIPTION, LOBBY_EXPANSIONS, LOBBY_EXPANSION_ROW,
    LOBBY_PROMPT, LOBBY_YEAR, LOBBY_DAYMONTH,
    LOBBY_YEAR_UP1, LOBBY_YEAR_DOWN1, LOBBY_YEAR_UP2, LOBBY_YEAR_DOWN2,
    LOBBY_YEAR_UP3, LOBBY_YEAR_DOWN3, LOBBY_MONTH_UP, LOBBY_MONTH_DOWN,
    LOBBY_DAY_UP, LOBBY_DAY_DOWN, LOBBY_ELEMENT_COUNT
};

enum setup_scroll_part { SETUP_SCROLL_UP, SETUP_SCROLL_DOWN, SETUP_SCROLL_TRACK, SETUP_SCROLL_THUMB,
                         SETUP_SCROLL_PARTS };

struct setup_toolbar {
    struct frontend_gui frontend;
    struct setup_widget widgets[SETUP_WIDGET_COUNT];
    struct setup_widget lobby[LOBBY_ELEMENT_COUNT];
    struct setup_widget scroll[SETUP_SCROLL_PARTS];
    SDL_Texture *detail_font;
    struct glyph detail_glyphs[256];
    int expansion_spacing;
    char bookmarks_label[96], featured_label[96], vassals_label[96], load_label[96], prompt[160];
    char back_label[96], play_label[96], difficulty_label[96], diplomacy_label[96];
};

int setup_toolbar_load(struct setup_toolbar *, SDL_Renderer *, const char *root, int width);
int setup_lobby_load(struct setup_toolbar *, SDL_Renderer *, const char *root, SDL_Point viewport);
void setup_widget_draw(const struct setup_widget *, SDL_Renderer *, SDL_Point);
bool setup_lobby_deferred(const struct setup_toolbar *, SDL_Point);
void setup_toolbar_free(struct setup_toolbar *);
void setup_toolbar_draw(const struct setup_toolbar *, SDL_Renderer *, SDL_Point);
enum setup_control setup_toolbar_hit(const struct setup_toolbar *, SDL_Point);
enum setup_control setup_toolbar_shortcut(const struct setup_toolbar *, SDL_Keycode);

#endif
