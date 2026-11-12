#ifndef OPENCK2_MAIN_H
#define OPENCK2_MAIN_H

#include <SDL.h>
#include "rulers.h"
#include "campaign.h"
#include "realms.h"
#include "construction.h"
#include "character_view.h"

struct wonder_panel {
    struct setup_widget background, close, terrain, frame, completed;
    SDL_Rect button, name_area, description_area, image_area;
};

struct debug_console {
    SDL_Rect rect;
    char buffer[128];
    int cursor;
    bool active;
};

struct menu {
    const struct menu_assets *assets;
    const char *title;
    enum game_state state;
    const struct ruler_profile *ruler;
    unsigned home_province, setup_province, campaign_ruler_id;
    char setup_title[72];
    struct campaign_date start_date;
    struct character_view character;
    struct wonder_panel wonder;
    struct debug_console debug_console;
    struct setup_widget topbar[14];
    SDL_Rect topbar_values[8];
    char topbar_labels[7][64], topbar_help[7][512];
    int campaign_speed, topbar_offset;
    bool topbar_ready;
    struct county_holdings holdings;
    struct construction construction;
    SDL_Texture *building_bg, *building_row, *building_close, *settlement_bg;
    SDL_Texture *settlement_build, *settlement_hover;
    char settlement_headline[64], settlement_build_label[32], settlement_types[3][32];
    SDL_Rect settlement_panel, settlement_title, settlement_list, settlement_close;
    SDL_Rect settlement_art, settlement_name, settlement_time, settlement_cost, settlement_button;
    int settlement_row_height;
    SDL_Rect building_panel, building_list, building_title, building_close_area, building_art_area;
    int building_row_height, holding_columns;
    size_t building_slot, building_scroll;
    bool building_open;
    size_t vassal_scroll;
    int bookmark_scroll, featured_scroll;
    int bookmark_drag_offset;
    bool bookmark_scroll_pressed, bookmark_drag;
    enum lobby_element scroll_list;
    SDL_Cursor *game_cursor;
    SDL_Rect holding_panel, holding_rows[2], holding_capital, holding_templates[2];
    SDL_Rect holding_art_area[2], holding_frame_area[2];
    float holding_art_scale[2];
    bool holding_layout_ready;
    unsigned holding_province;
    SDL_Point drag_start;
    Uint8 drag_button;
    bool realms_visible, campaign_active;
    SDL_Point pointer;
    struct campaign campaign;
    struct province_map provinces;
    const struct province *selected;
};

enum game_state { GAME_SETUP, GAME_CAMPAIGN };

#endif /* OPENCK2_MAIN_H */
