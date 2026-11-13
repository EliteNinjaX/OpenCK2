#ifndef OPENCK2_CAMPAIGN_HUD_H
#define OPENCK2_CAMPAIGN_HUD_H

#include "map_modes.h"
#include "setup_toolbar.h"

struct campaign_hud_ui {
    struct setup_widget alert_background, alert_strip;
    SDL_Point alert_start;
    int alert_step;
    bool alert_assets_ready;
    struct setup_widget topbar_shield, topbar_crown;
    struct setup_widget extension[3];
    struct setup_widget navigation[8];
    struct setup_widget modes[MAP_MODE_COUNT];
    struct setup_widget next_song;
    bool initialized;
};

/* The GUI tree supplies positions, sprite names and map-mode tooltips.
 * Missing optional artwork does not prevent a campaign from launching. */
void campaign_hud_ui_load(struct campaign_hud_ui *, SDL_Renderer *, const char *, SDL_Point);
void campaign_hud_ui_draw_background(const struct campaign_hud_ui *, SDL_Renderer *);
void campaign_hud_ui_draw(const struct campaign_hud_ui *, SDL_Renderer *, SDL_Point,
                          enum map_mode_id);
bool campaign_hud_ui_next_song_hit(const struct campaign_hud_ui *, SDL_Point);
int campaign_hud_ui_mode_hit(const struct campaign_hud_ui *, SDL_Point);
int campaign_hud_ui_alert_hit(const struct campaign_hud_ui *, SDL_Point);
int campaign_hud_ui_nav_hit(const struct campaign_hud_ui *, SDL_Point);
void campaign_hud_ui_free(struct campaign_hud_ui *);
const char *campaign_hud_nav_label(int);

#endif
