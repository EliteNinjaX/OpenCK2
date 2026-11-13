#define _POSIX_C_SOURCE 200809L
#include "assets.h"
#include "../open_launcher/asset_paths.h"
#include "campaign.h"
#include "../version.h"
#include "../open_launcher/install.h"
#include "music.h"
#include "settings.h"
#include "setup_toolbar.h"
#include "era.h"
#include "portraits.h"
#include "rules.h"
#include "realms.h"
#include "holdings.h"
#include "character_view.h"
#include "tree_view.h"
#include "bloodline_view.h"
#include "wonder_navigation_core.h"
#include "ck2_script.h"
#include "construction.h"
#include "map_view.h"
#include "map_interaction.h"
#include "map_modes.h"
#include "map_themes.h"
#include "campaign_hud.h"
#include "map_settlements.h"
#include "province_layout_core.h"
#include "sprite_frame_core.h"
#include "religion_style_cache.h"
#include "religion_gfx_core.h"
#include "economy.h"
#include "technology_view.h"
#include "debug_console.h"
#include "family_gameplay.h"
#include "family_names_core.h"
#include "conversion_core.h"
#include "event_core.h"
#include "warfare_core.h"
#include "warfare_map.h"
#include "law_core.h"
#include "election_core.h"
#include "checkpoint_core.h"
#include "government_mechanics_core.h"
#include "map_positions.h"
#include <math.h>

#include <SDL_image.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

enum { WIDTH = 1600, HEIGHT = 900, BUTTON_WIDTH = 232, BUTTON_HEIGHT = 52, VASSAL_DISPLAY_LIMIT = 32,
       CREDIT_LINE = 24 };

enum screen { MAIN_MENU, SINGLE_PLAYER, STARTING_ERA, MAP_SETUP, GAME_RULES, OPTIONS, CREDITS, CAMPAIGN };
enum action { UNAVAILABLE, OPEN_OPTIONS, OPEN_CREDITS, QUIT, MULTIPLAYER,
              BACK, CONTENT_SETTINGS, PARADOX_LINK, PDF_LINK, REDDIT_LINK, GITHUB_LINK,
              OPEN_SINGLE_PLAYER, NEW_GAME, CONTINUE_GAME, LOAD_GAME, TUTORIAL,
              START_ERA, CUSTOM_SETUP, LAUNCH_CAMPAIGN, TOGGLE_REALMS, TOGGLE_TIME, NEXT_MONTH, SELECT_HOME, SPEED_UP, SPEED_DOWN };

extern char **environ;

static const char *const links[] = {
    [PARADOX_LINK] = "https://forum.paradoxplaza.com/forum/forums/crusader-kings-ii.551/",
    [PDF_LINK] = "https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/203770/manuals/PDX7605US_CK%20II_Onlinemanual.pdf?t=1740566844",
    [REDDIT_LINK] = "https://www.reddit.com/r/CrusaderKings/",
    [GITHUB_LINK] = "https://github.com/EliteNinjaX/OpenCK2",
};

struct button {
    SDL_Rect area;
    const char *label;
    enum action action;
};

static const struct button menu_buttons[] = {
    {{0}, "Single Player", OPEN_SINGLE_PLAYER},
    {{0}, "Multiplayer", MULTIPLAYER},
    {{0}, "Options", OPEN_OPTIONS},
    {{0}, "Credits", OPEN_CREDITS},
    {{0}, "Quit", QUIT},
    {{718, 665, 164, 30}, "Content Settings", CONTENT_SETTINGS},
    {{1186, 851, 83, 30}, "Manual", PDF_LINK},
    {{1276, 851, 124, 30}, "Paradox Plaza", PARADOX_LINK},
    {{1407, 851, 83, 30}, "Reddit", REDDIT_LINK},
    {{1497, 851, 83, 30}, "GitHub", GITHUB_LINK},
};
static const struct button single_player_buttons[] = {
    {{684, 385, BUTTON_WIDTH, BUTTON_HEIGHT}, "Continue", CONTINUE_GAME},
    {{684, 442, BUTTON_WIDTH, BUTTON_HEIGHT}, "New Game", NEW_GAME},
    {{684, 499, BUTTON_WIDTH, BUTTON_HEIGHT}, "Load Game", LOAD_GAME},
    {{684, 556, BUTTON_WIDTH, BUTTON_HEIGHT}, "Tutorial", TUTORIAL},
    {{718, 638, 164, 30}, "Back", BACK},
};

static const struct button era_buttons[] = {
    {{360, 742, 220, 52}, "Back", BACK},
    {{769, 742, 220, 52}, "Custom Game Setup", CUSTOM_SETUP},
    {{1015, 742, 220, 52}, "Start Game", START_ERA},
};

static const struct button rules_buttons[] = {
    {{390, 691, 220, 48}, "Close", BACK},
    {{990, 691, 220, 48}, "Start Game", LAUNCH_CAMPAIGN},
};

static const struct button credits_buttons[] = {
    {{684, 720, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", BACK},
};

static const struct button campaign_buttons[] = {
    {{1350, 247, 224, 30}, "Capital", SELECT_HOME},
    /* Developer controls stay together; campaign map modes are on the
     * original minimap, so there must not be a second Terrain toggle. */
    {{580, 846, 206, 34}, "Next Month", NEXT_MONTH},
    {{794, 846, 172, 34}, "Main Menu", BACK},
};

static const SDL_Rect campaign_hud[] = {
    {0, 0, 492, 136}, {1174, 0, 426, 72}, {1324, 213, 276, 67},
    {0, 276, 348, 624}, {1320, 696, 280, 204},
};

static bool campaign_map_area(SDL_Point point)
{
    if (point.x < 0 || point.y < 0 || point.x >= WIDTH || point.y >= HEIGHT)
        return false;
    for (size_t i = 0; i < SDL_arraysize(campaign_hud); ++i) {
        if (SDL_PointInRect(&point, &campaign_hud[i]))
            return false;
    }
    return true;
}

struct vassal_art {
    SDL_Texture *portrait, *shield;
};

struct wonder_panel {
    struct setup_widget button, background, close, terrain, frame, image, completed, next, previous;
    unsigned *locations; /* Historical province IDs containing constructed works. */
    size_t location_count, location_capacity;
    bool catalog_ready;
    SDL_Rect name_area, description_area, image_area, completed_area;
    char id[64], name[96], description[1024];
    char completed_label[64], upgrades[12][72], upgrade_names[12][96];
    size_t upgrade_count;
    unsigned stage;
    bool damaged, open;
};

/* Cached source-GUI widgets and historic title graphics for the selected province. */
struct province_panel_skin {
    struct setup_widget background;
    struct setup_widget portrait_frame, county_crown, region_info, raise_levy,
                        raise_ships, temple_button, capital_icon;
    struct setup_widget levy_progress_bg;
    struct setup_widget garrison_icon, levy_icon, tech_icons[3], tax_icon,
                        strait_icon, route_icon, crown_focus;
    SDL_Rect ruler_portrait, name, kingdom, duchy, culture, religion,
             kingdom_shield, duchy_shield, owner_shield[2][3],
             tax_amount, supply_label, revolt_label, culture_label,
             religion_label, tech_amount[3];
    SDL_Texture *portrait, *kingdom_flag, *duchy_flag, *independent_flag,
                *barony_flags[HOLDING_LIMIT];
    struct province_layout grid;
    bool loaded;
};

struct menu {
    struct menu_music music;
    struct settings settings;
    struct setup_toolbar toolbar;
    struct button setup_buttons[2];
    struct button main_buttons[SDL_arraysize(menu_buttons)];
    struct era_menu era;
    struct rules_menu rules;
    struct realm_map realms;
    struct portrait_source portrait_source;
    SDL_Texture *setup_portrait, *setup_shield, *campaign_portrait, *campaign_title_shield;
    SDL_Texture *province_title_shield;
    SDL_Rect province_shield_area;
    struct map_title_shield_cache {
        char id[72];
        char religion[64];
        SDL_Texture *texture;
        unsigned long last_used;
        bool attempted;
    } map_title_shields[192];
    unsigned long map_title_clock;
    struct vassal_art vassal_art[VASSAL_DISPLAY_LIMIT];
    bool portrait_source_loaded;
    enum screen rules_back;
    struct campaign_date start_date;
    const struct era_ruler *campaign_ruler;
    unsigned home_province, setup_province, campaign_ruler_id, campaign_start_ruler_id;
    char setup_title[72];
    const struct menu_assets *assets;
    struct character_view character;
    struct tree_view genealogy;
    struct bloodline_view bloodlines;
    struct wonder_panel wonder;
    struct setup_widget topbar[14];
    SDL_Rect topbar_values[8];
    char topbar_labels[7][64], topbar_help[7][512];
    int campaign_speed;
    bool topbar_ready;
    int topbar_offset;
    struct county_holdings holdings;
    struct construction construction;
    struct economy_snapshot economy;
    struct ck2_law_catalog law_catalog;
    struct ck2_election_catalog election_catalog;
    struct ck2_government_rules government_rules;
    struct ck2_trade_catalog trade_catalog;
    struct ck2_government_mechanics government_mechanics;
    struct ck2_law_state active_laws;
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
    struct setup_widget province_close;
    struct province_panel_skin province_skin;
    SDL_Rect holding_art_area[2], holding_frame_area[2], holding_owner_frame_area[2];
    bool holding_owner_frame_valid[2];
    struct setup_widget holding_owner_frames[2];
    SDL_Texture *styled_holding_art[HOLDING_TYPE_COUNT];
    SDL_Texture *styled_holding_frame, *styled_holding_capital;
    char holding_style_key[160];
    struct religion_style_cache religion_styles;
    float holding_art_scale[2];
    bool holding_layout_ready;
    unsigned holding_province;
    struct map_gesture map_gesture;
    struct map_mode_state map_modes;
    struct map_theme map_theme;
    struct map_settlement_scene settlement_scene;
    struct campaign_hud_ui hud_ui;
    struct technology_view technology;
    struct debug_console console;
    struct ck2_event pending_event;
    bool event_open;
    unsigned event_target_id;
    struct family_gameplay family;
    struct warfare_state warfare;
    struct warfare_graph warfare_graph;
    struct conversion_state conversion;
    struct conversion_catalog conversion_catalog;
    bool realms_visible, campaign_active;
    SDL_Point pointer;
    struct campaign campaign;
    struct province_map provinces;
    const struct province *selected;
    SDL_Texture *province_overlay;
    struct map_view menu_map, campaign_map;
    SDL_Renderer *renderer;
    Uint64 next_day;
    bool time_running;
    struct content_catalog catalog;
    const char *asset_root;
    char *selection_path;
    pid_t content_editor;
    enum screen screen;
    enum action focus, hover;
    int credit_scroll, credit_scroll_max;
    bool running;
};

/* Map events must never pass through a visible panel or a map-mode button. */
static bool campaign_map_interactive(const struct menu *menu, SDL_Point point)
{
    if (!campaign_map_area(point) ||
        campaign_hud_ui_next_song_hit(&menu->hud_ui, point))
        return false;
    if (menu->selected && !menu->character.open &&
        point.x >= menu->holding_panel.x &&
        point.x < menu->holding_panel.x + menu->holding_panel.w &&
        point.y >= menu->holding_panel.y &&
        point.y < menu->holding_panel.y + menu->holding_panel.h)
        return false;
    /* Both developer controls also occupy map coordinates; they must
     * block province selection and right-click owner navigation. */
    for (size_t i = 0; i < SDL_arraysize(campaign_buttons); ++i)
        if (SDL_PointInRect(&point, &campaign_buttons[i].area))
            return false;
    if ((menu->character.open && SDL_PointInRect(&point, &menu->character.background.area)) ||
        menu->wonder.open ||
        (menu->building_open && SDL_PointInRect(&point, &menu->building_panel)) ||
        campaign_hud_ui_mode_hit(&menu->hud_ui, point) >= 0 ||
        campaign_hud_ui_nav_hit(&menu->hud_ui, point) >= 0 ||
        campaign_hud_ui_alert_hit(&menu->hud_ui, point) >= 0)
        return false;
    return true;
}

static const struct province *campaign_pointer_province(const struct menu *menu, SDL_Point pointer)
{
    if (!campaign_map_interactive(menu, pointer))
        return NULL;
    SDL_Point world;
    if (!map_view_pick(&menu->campaign_map, pointer.x, pointer.y, &world))
        return NULL;
    return province_at(&menu->provinces, world.x, world.y);
}

static void blit(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y)
{
    SDL_Rect target = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &target.w, &target.h);
    SDL_RenderCopy(renderer, texture, NULL, &target);
}

static void centered_text(SDL_Renderer *renderer, const struct menu_assets *assets,
                          const char *text, size_t length, int center, int y)
{
    const unsigned char *chars = (const unsigned char *)text;
    int width = 0;
    for (size_t i = 0; i < length; ++i)
        width += assets->glyphs[chars[i]].advance;

    int x = center - width / 2;
    for (size_t i = 0; i < length; ++i) {
        const struct glyph *glyph = &assets->glyphs[chars[i]];
        if (glyph->area.w > 0 && glyph->area.h > 0) {
            const SDL_Rect target = {x + glyph->x_offset, y + glyph->y_offset,
                                     glyph->area.w, glyph->area.h};
            SDL_RenderCopy(renderer, assets->font, &glyph->area, &target);
        }
        x += glyph->advance;
    }
}

static void draw_credits(SDL_Renderer *renderer, const struct menu_assets *assets, int offset)
{
    const SDL_Rect clip = {568, 165, 464, 535};
    SDL_RenderSetClipRect(renderer, &clip);
    const char *line = assets->credits;
    int y = clip.y - offset;

    while (*line && y < clip.y + clip.h) {
        const char *end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line) : strlen(line);
        if (length && line[length - 1] == '\r')
            --length;
        if (length && (unsigned char)*line == 0xa7) {
            ++line;
            --length;
        }
        if (y + CREDIT_LINE > clip.y)
            centered_text(renderer, assets, line, length, WIDTH / 2, y);
        if (!end)
            break;
        line = end + 1;
        y += CREDIT_LINE;
    }
    SDL_RenderSetClipRect(renderer, NULL);
}

static int load_main_button_layout(struct menu *menu, const struct menu_assets *assets)
{
    static const char *const names[] = {
        "single_player_menu_button", "multi_player_button", "settings_button",
        "credits_button", "exit_button"
    };
    struct frontend_gui *gui = &menu->toolbar.frontend;
    if (frontend_gui_load(gui, menu->asset_root) < 0)
        return -1;
    const int main = frontend_gui_find(gui, -1, "mainmenu_panel");
    const int panel = main < 0 ? -1 : frontend_gui_find(gui, main, "mainmenu_bottom_panel_banner");
    memcpy(menu->main_buttons, menu_buttons, sizeof menu_buttons);
    int result = 0;
    for (size_t i = 0; i < SDL_arraysize(names); ++i) {
        SDL_Rect *area = &menu->main_buttons[i].area;
        const int button = panel < 0 ? -1 : frontend_gui_find(gui, panel, names[i]);
        if (!frontend_gui_rect(gui, button, (SDL_Point){WIDTH, HEIGHT}, area) ||
            SDL_QueryTexture(assets->button, NULL, NULL, &area->w, &area->h) < 0) {
            result = SDL_SetError("interface/frontend.gui: could not position main menu button %s", names[i]);
            break;
        }
    }
    return result;
}

static const struct button *buttons_for(const struct menu *menu, size_t *count)
{
    const enum screen screen = menu->screen;
    if (screen == CAMPAIGN) {
        *count = SDL_arraysize(campaign_buttons);
        return campaign_buttons;
    }
    if (screen == MAP_SETUP) {
        *count = SDL_arraysize(menu->setup_buttons);
        return menu->setup_buttons;
    }
    if (screen == GAME_RULES) {
        *count = SDL_arraysize(rules_buttons);
        return rules_buttons;
    }
    if (screen == STARTING_ERA) {
        *count = SDL_arraysize(era_buttons);
        return era_buttons;
    }
    if (screen == SINGLE_PLAYER) {
        *count = SDL_arraysize(single_player_buttons);
        return single_player_buttons;
    }
    if (screen == CREDITS) {
        *count = SDL_arraysize(credits_buttons);
        return credits_buttons;
    }
    *count = SDL_arraysize(menu_buttons);
    return menu->main_buttons;
}

static bool action_unimplemented(enum action action)
{
    return action == MULTIPLAYER || action == LOAD_GAME || action == TUTORIAL;
}

static bool action_enabled(const struct menu *menu, enum action action)
{
    if (action == START_ERA || action == CUSTOM_SETUP)
        return era_menu_can_start(&menu->era);
    if (action == CONTINUE_GAME)
        return menu->campaign_active;
    if (action == SELECT_HOME)
        return menu->home_province != 0;
    return action != UNAVAILABLE && !action_unimplemented(action);
}

static const char *button_label(const struct button *button,
                                const struct menu *menu)
{
    if (button->action == SELECT_HOME) {
        const struct province *home = province_by_id(&menu->provinces, menu->home_province);
        return home ? home->name : button->label;
    }
    if (button->action == TOGGLE_REALMS)
        return menu->realms_visible ? "Terrain" : "Realms";
    return button->label;
}

static void draw_menu_content(SDL_Renderer *renderer, const struct menu_assets *assets,
                              const struct content_catalog *catalog)
{
    enum { ICON_SIZE = 32, ICON_STEP = 36, FIRST_ROW = 8 };
    for (int icon = 0; icon < CK2_DLC_ICON_COUNT; ++icon) {
        SDL_Texture *texture = assets->dlc_icons[icon];
        if (!texture)
            continue;
        bool selected = false;
        for (size_t i = 0; i < catalog->dlc.count; ++i) {
            const struct content_entry *entry = &catalog->dlc.entries[i];
            if (entry->icon == icon && entry->installed && entry->enabled) {
                selected = true;
                break;
            }
        }
        const int row = icon >= FIRST_ROW;
        const int index = row ? icon - FIRST_ROW : icon;
        const int count = row ? CK2_DLC_ICON_COUNT - FIRST_ROW : FIRST_ROW;
        const int left = (WIDTH - (count - 1) * ICON_STEP - ICON_SIZE) / 2;
        const SDL_Rect source = {selected ? 0 : 64, 0, ICON_SIZE, ICON_SIZE};
        const SDL_Rect target = {left + index * ICON_STEP, 578 + row * ICON_STEP,
                                 ICON_SIZE, ICON_SIZE};
        SDL_RenderCopy(renderer, texture, &source, &target);
    }
}

static void refresh_content(struct menu *menu)
{
    ck2_catalog_free(&menu->catalog);
    ck2_catalog_load(&menu->catalog, menu->asset_root);
    ck2_catalog_restore(&menu->catalog, menu->selection_path);
}

static void open_content_settings(struct menu *menu, SDL_Window *window)
{
    if (menu->content_editor > 0)
        return;

    char *base = SDL_GetBasePath();
    char *path = base ? ck2_join_path(base, "OpenCK2") : NULL;
    SDL_free(base);
    char *const args[] = {path, "--content-settings", "--assets", (char *)menu->asset_root, NULL};
    pid_t child;
    const int error = path ? posix_spawn(&child, path, NULL, NULL, args, environ) : ENOMEM;
    free(path);
    if (error) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Content Settings", strerror(error), window);
        return;
    }
    menu->content_editor = child;
}

static bool poll_content_settings(struct menu *menu, SDL_Window *window)
{
    if (menu->content_editor <= 0)
        return false;
    int status;
    const pid_t child = waitpid(menu->content_editor, &status, WNOHANG);
    if (child == 0 || (child < 0 && errno == EINTR))
        return false;
    menu->content_editor = 0;
    if (child < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Content Settings",
                                 "The content editor closed unexpectedly.", window);
    refresh_content(menu);
    return true;
}

static void campaign_text(SDL_Renderer *renderer, const struct menu_assets *assets,
                          const char *text, int center, int y)
{
    centered_text(renderer, assets, text, strlen(text), center, y);
}

static void format_campaign_date(char *buffer, size_t size, struct campaign_date date)
{
    static const char *const months[] = {"January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"};
    snprintf(buffer, size, "%d %s %d", date.day, months[date.month - 1], date.year);
}

/* Developer-fired events use the CK2 localisation catalog, but not yet the
 * original event shader, chained scripted effects, or automatic triggers. */
static SDL_Rect scripted_option_rect(size_t index)
{
    return (SDL_Rect){484, 487 + (int)index * 35, 632, 32};
}
static void scripted_wrapped_text(SDL_Renderer *renderer, const struct menu_assets *assets,
                                  const char *text, int center, int y, size_t max_lines)
{
    const unsigned char *p = (const unsigned char *)text;
    char line[100];
    size_t row=0;
    while (*p && row < max_lines) {
        size_t used=0, last_space=0;
        while (p[used] && p[used]!='\n' && used < 86) {
            if (p[used]==' ') last_space=used;
            ++used;
        }
        size_t length=used;
        if (used==86 && p[used] && last_space>0) length=last_space;
        if (length >= sizeof line) length=sizeof line-1;
        memcpy(line,p,length); line[length]=0;
        campaign_text(renderer,assets,line,center,y+(int)row*25);
        p+=length;
        while (*p==' ' || *p=='\n') ++p;
        ++row;
    }
}
static void draw_scripted_event(SDL_Renderer *renderer, const struct menu_assets *assets,
                                 const struct menu *menu)
{
    if (!menu->event_open) return;
    SDL_Rect frame = {413,160,774,658};
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,3,5,8,228);
    SDL_RenderFillRect(renderer,&frame);
    SDL_SetRenderDrawColor(renderer,198,170,111,255);
    SDL_RenderDrawRect(renderer,&frame);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    const struct ck2_event *e=&menu->pending_event;
    const char *title=ck2_localize(e->title,e->title);
    if (!strcmp(title,e->title)) title=e->id;
    scripted_wrapped_text(renderer,assets,title,800,195,2);
    const char *desc=ck2_localize(e->description,e->description);
    scripted_wrapped_text(renderer,assets,desc,800,274,7);
    if (e->has_immediate || e->has_trigger || e->conditional_description)
        campaign_text(renderer,assets,"Console-fired: event triggers and scripted immediate effects bypassed",800,458);
    if (e->option_count == 0) campaign_text(renderer,assets,"No supported option entries; Esc closes",800,525);
    for (size_t i=0;i<e->option_count;i++) {
        SDL_Rect area=scripted_option_rect(i);
        SDL_SetRenderDrawColor(renderer,39,35,32,255);
        SDL_RenderFillRect(renderer,&area);
        SDL_SetRenderDrawColor(renderer,153,121,74,255);
        SDL_RenderDrawRect(renderer,&area);
        const char *label=e->options[i].name[0] ?
            ck2_localize(e->options[i].name,e->options[i].name) : "Unnamed option";
        char message[164];
        snprintf(message,sizeof message,"%zu. %.135s%s",i+1,label,
            e->options[i].ignored_effects || e->options[i].conditional ? " *" : "");
        campaign_text(renderer,assets,message,800,area.y+6);
    }
    campaign_text(renderer,assets,"Click an option or use ` eventchoose 1    |    Esc to close",800,783);
}

static void draw_debug_console(SDL_Renderer *renderer, const struct menu_assets *assets,
                               const struct debug_console *console)
{
    if (!console || !console->open) return;
    const SDL_Rect frame = {30, 492, 1050, 376};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 9, 13, 228);
    SDL_RenderFillRect(renderer, &frame);
    SDL_SetRenderDrawColor(renderer, 211, 176, 102, 255);
    SDL_RenderDrawRect(renderer, &frame);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    campaign_text(renderer, assets, "OpenCK2 Developer Console (` to close)", 555, 501);
    for (unsigned i = 0; i < console->count; ++i) {
        char line[112];
        snprintf(line, sizeof line, "%.106s", console->lines[i]);
        campaign_text(renderer, assets, line, 551, 529 + (int)i * 26);
    }
    char input[140];
    snprintf(input, sizeof input, "> %.127s_", console->input);
    campaign_text(renderer, assets, input, 551, 838);
}

static void draw_single_player(SDL_Renderer *renderer, const struct menu_assets *assets,
                               const struct menu *menu)
{
    const SDL_Rect frame = {600, 179, 400, 550};
    const SDL_Rect heading = {710, 276, 180, 38};
    SDL_RenderCopy(renderer, assets->dialog_frame, NULL, &frame);
    SDL_RenderCopy(renderer, assets->dialog_heading, NULL, &heading);
    campaign_text(renderer, assets, "Single Player", 800, 284);

    if (menu->campaign_active) {
        char date[48];
        format_campaign_date(date, sizeof date, menu->campaign.date);
        campaign_text(renderer, assets, "Current campaign", 800, 320);
        const struct ruler_profile *ruler = realm_ruler(&menu->realms, menu->campaign_ruler_id);
        campaign_text(renderer, assets, menu->campaign_ruler ? menu->campaign_ruler->name
                      : ruler && ruler->name[0] ? ruler->name : "County of York", 800, 340);
        campaign_text(renderer, assets, date, 800, 360);
    } else {
        campaign_text(renderer, assets, "No campaign to continue", 800, 340);
    }
}

static void setup_description(SDL_Renderer *, const struct menu_assets *, const char *, SDL_Rect);

static void clear_wonder(struct wonder_panel *wonder)
{
    SDL_DestroyTexture(wonder->image.texture);
    wonder->image = (struct setup_widget){0};
    wonder->id[0] = 0;
    wonder->stage = 0;
    wonder->upgrade_count = 0;
    wonder->damaged = wonder->open = false;
}

static void read_wonder(struct wonder_panel *wonder, const char *p, struct campaign_date date)
{
    clear_wonder(wonder);
    const int cutoff = date.year * 10000 + date.month * 100 + date.day;
    int built = 0, stage = 0, damaged = 0;
    while (p && *p) {
        struct token key = token(&p);
        if (!key.size || !is(token(&p), "="))
            break;
        struct token value = token(&p);
        if (!is(value, "{"))
            continue;
        char raw[24];
        int y, m, d;
        copy_token(raw, sizeof raw, key);
        if (sscanf(raw, "%4d.%2d.%2d", &y, &m, &d) == 3) {
            const int when = y * 10000 + m * 100 + d;
            if (when <= cutoff) {
                const struct token id = field(p, "build_wonder");
                const struct token level = field(p, "set_wonder_stage");
                const struct token damage = field(p, "set_wonder_damaged");
                if (id.size && when >= built) {
                    built = when;
                    copy_token(wonder->id, sizeof wonder->id, id);
                }
                if (level.size && when >= stage) {
                    stage = when;
                    wonder->stage = SDL_min(4u, number(level));
                }
                const char *fields = p;
                while (wonder->upgrade_count < SDL_arraysize(wonder->upgrades)) {
                    const struct token upgrade = find(&fields, "build_wonder_upgrade");
                    if (!upgrade.size || is(upgrade, "{"))
                        break;
                    copy_token(wonder->upgrades[wonder->upgrade_count], sizeof wonder->upgrades[0], upgrade);
                    ++wonder->upgrade_count;
                }
                if (damage.size && when >= damaged) {
                    damaged = when;
                    wonder->damaged = is(damage, "yes");
                }
            }
        }
        skip_block(&p);
    }
}

static bool bind_wonder_widget(struct menu *menu, const struct frontend_gui *gui, int parent,
                               const char *name, struct setup_widget *widget)
{
    const int node = frontend_gui_find(gui, parent, name);
    SDL_Rect area;
    return frontend_gui_rect(gui, node, (SDL_Point){WIDTH, HEIGHT}, &area) &&
        setup_widget_load_node(widget, menu->renderer, menu->asset_root, gui, node, area) == 0;
}

static bool load_wonder_view(struct menu *menu)
{
    struct wonder_panel *wonder = &menu->wonder;
    if (!wonder->background.texture) {
        struct frontend_gui gui = {0};
        if (frontend_gui_load_file(&gui, menu->asset_root, "interface/wonder.gui") < 0)
            return false;
        const int panel = frontend_gui_find(&gui, -1, "wonder_view");
        const SDL_Point viewport = {WIDTH, HEIGHT};
        const bool ok = panel >= 0 &&
            bind_wonder_widget(menu, &gui, panel, "background", &wonder->background) &&
            bind_wonder_widget(menu, &gui, panel, "button_close", &wonder->close) &&
            bind_wonder_widget(menu, &gui, panel, "wonder_image_bg", &wonder->terrain) &&
            bind_wonder_widget(menu, &gui, panel, "wonder_image_frame", &wonder->frame) &&
            bind_wonder_widget(menu, &gui, panel, "owned_upgrades_button", &wonder->completed) &&
            frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "owned_upgrades_button_label"), viewport,
                              &wonder->completed_area) &&
            frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "wonder_name"), viewport, &wonder->name_area) &&
            frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "wonder_description"), viewport, &wonder->description_area) &&
            frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "wonder_image"), viewport, &wonder->image_area);
        /* Optional navigation defined by the original wonder.gui. */
        (void)bind_wonder_widget(menu, &gui, panel, "next_wonder_button", &wonder->next);
        (void)bind_wonder_widget(menu, &gui, panel, "previous_wonder_button", &wonder->previous);
        frontend_gui_free(&gui);
        if (!ok) {
            struct setup_widget *parts[] = {&wonder->background, &wonder->close, &wonder->terrain, &wonder->frame, &wonder->completed, &wonder->next, &wonder->previous};
            for (size_t i = 0; i < SDL_arraysize(parts); ++i) {
                SDL_DestroyTexture(parts[i]->texture);
                *parts[i] = (struct setup_widget){0};
            }
            return false;
        }
    }
    if (wonder->image.texture)
        return true;
    struct ck2_asset_files files = ck2_list_assets(menu->asset_root, "common/wonders");
    char picture[96] = "";
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t length = strlen(name);
        if (length < 4 || strcmp(name + length - 4, ".txt"))
            continue;
        char *data = load_from_directory(menu->asset_root, "common/wonders", name);
        const char *body = data;
        if (body && is(find(&body, wonder->id), "{"))
            copy_token(picture, sizeof picture, field(body, "picture"));
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
    if (!*picture)
        return false;
    char sprite[112], description_key[80];
    snprintf(sprite, sizeof sprite, "%s_large", picture);
    if (setup_widget_load_sprite(&wonder->image, menu->renderer, menu->asset_root, "interface/wonder.gfx",
                                 sprite, wonder->image_area) < 0)
        return false;
    snprintf(description_key, sizeof description_key, "%s_desc", wonder->id);
    SDL_strlcpy(wonder->name, wonder->id, sizeof wonder->name);
    wonder->description[0] = 0;
    struct ck2_translation labels[3 + SDL_arraysize(wonder->upgrades)] = {
        {wonder->id, wonder->name, sizeof wonder->name},
        {description_key, wonder->description, sizeof wonder->description},
        {"COMPLETED_CAP", wonder->completed_label, sizeof wonder->completed_label},
    };
    for (size_t i = 0; i < wonder->upgrade_count; ++i) {
        SDL_strlcpy(wonder->upgrade_names[i], wonder->upgrades[i], sizeof wonder->upgrade_names[i]);
        labels[3 + i] = (struct ck2_translation){wonder->upgrades[i], wonder->upgrade_names[i],
                                                  sizeof wonder->upgrade_names[i]};
    }
    ck2_translate(menu->asset_root, labels, 3 + wonder->upgrade_count);
    return true;
}

static void draw_wonder(SDL_Renderer *renderer, const struct menu_assets *assets, const struct menu *menu)
{
    const struct wonder_panel *wonder = &menu->wonder;
    if (!wonder->open)
        return;
    setup_widget_draw(&wonder->background, renderer, menu->pointer);
    setup_widget_draw(&wonder->terrain, renderer, (SDL_Point){-1, -1});
    const struct setup_widget *image = &wonder->image;
    const int frame = wonder->damaged ? 0 : SDL_clamp((int)wonder->stage, 0, image->frames - 1);
    const SDL_Rect source = {image->area.w * frame, 0, image->area.w, image->area.h};
    SDL_RenderCopy(renderer, image->texture, &source, &image->area);
    setup_widget_draw(&wonder->frame, renderer, menu->pointer);
    setup_widget_draw(&wonder->close, renderer, menu->pointer);
    if (wonder->next.texture) setup_widget_draw(&wonder->next, renderer, menu->pointer);
    if (wonder->previous.texture) setup_widget_draw(&wonder->previous, renderer, menu->pointer);
    setup_widget_draw(&wonder->completed, renderer, wonder->completed.area.w > 0
                      ? (SDL_Point){wonder->completed.area.x, wonder->completed.area.y} : menu->pointer);
    const SDL_Rect tab = wonder->completed_area;
    campaign_text(renderer, assets, wonder->completed_label, tab.x + tab.w / 2, tab.y);
    for (size_t i = 0; i < wonder->upgrade_count && i < 9; ++i)
        campaign_text(renderer, assets, wonder->upgrade_names[i], wonder->name_area.x + wonder->name_area.w / 2,
                      tab.y + 30 + (int)i * 22);
    SDL_SetTextureColorMod(assets->font, 40, 28, 17);
    campaign_text(renderer, assets, wonder->name, wonder->name_area.x + wonder->name_area.w / 2, wonder->name_area.y);
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    struct menu_assets small = {.font = menu->era.small_font};
    memcpy(small.glyphs, menu->era.small_glyphs, sizeof small.glyphs);
    setup_description(renderer, small.font ? &small : assets, wonder->description, wonder->description_area);
    char status[96];
    snprintf(status, sizeof status, "Stage %u%s", wonder->stage, wonder->damaged ? "  |  Damaged" : "");
    campaign_text(renderer, assets, status, wonder->name_area.x + wonder->name_area.w / 2,
                  wonder->description_area.y + wonder->description_area.h + 34);
}

/* This list describes built Great Works at the loaded bookmark date. It
 * deliberately does not invent construction or ownership state beyond history. */
static void wonder_catalog_build(struct menu *menu)
{
    struct wonder_panel *wonder = &menu->wonder;
    if (wonder->catalog_ready) return;
    wonder->catalog_ready = true;
    for (size_t i = 0; i < menu->provinces.count; ++i) {
        const struct province *province = &menu->provinces.provinces[i];
        if (!province->land || !province->history_path) continue;
        char *history = province_history_read(province, menu->asset_root);
        if (!history) continue;
        if (strstr(history, "build_wonder")) {
            struct wonder_panel found = {0};
            read_wonder(&found, history, menu->start_date);
            if (found.id[0]) {
                if (wonder->location_count == wonder->location_capacity) {
                    size_t next_capacity = wonder->location_capacity ? wonder->location_capacity * 2 : 32;
                    if (next_capacity > SIZE_MAX / sizeof *wonder->locations) {
                        SDL_free(history);
                        break;
                    }
                    unsigned *next = realloc(wonder->locations, next_capacity * sizeof *next);
                    if (!next) {
                        SDL_free(history);
                        break;
                    }
                    wonder->locations = next;
                    wonder->location_capacity = next_capacity;
                }
                wonder->locations[wonder->location_count++] = province->id;
            }
        }
        SDL_free(history);
    }
    SDL_Log("Historical Great Works: %zu constructed provinces at bookmark date",
            wonder->location_count);
}

/* Return GUI coordinates as SDL rectangles but distinguish widget-local
 * template offsets from viewport-relative positions.  The CK2 province
 * window's declared 1024-wide list is intentionally wider than its sprite. */
static bool province_bind_rect(const struct frontend_gui *gui, int parent,
                               const char *name, SDL_Rect *rect)
{
    return frontend_gui_rect(gui, frontend_gui_find(gui, parent, name),
                             (SDL_Point){WIDTH, HEIGHT}, rect);
}

static void province_bind_widget(struct menu *menu, const struct frontend_gui *gui,
                                 int parent, const char *name, struct setup_widget *widget)
{
    SDL_Rect area;
    const int node = frontend_gui_find(gui, parent, name);
    if (province_bind_rect(gui, parent, name, &area) &&
        setup_widget_load_node(widget, menu->renderer, menu->asset_root, gui, node, area) < 0)
        SDL_ClearError();
}

static int load_holding_layout(struct menu *menu, SDL_Texture *background)
{
    struct frontend_gui gui = {0};
    if (frontend_gui_load_file(&gui, menu->asset_root, "interface/province.gui") < 0)
        return -1;
    const SDL_Point viewport = {WIDTH, HEIGHT};
    const int panel = frontend_gui_find(&gui, -1, "province");
    const int capital = frontend_gui_find(&gui, -1, "capital_position");
    static const char *const boxes[] = {"settlements_1_box", "settlements_2_box"};
    static const char *const templates[] = {"capital_entry", "settlement_entry"};
    bool valid = frontend_gui_rect(&gui, panel, viewport, &menu->holding_panel) &&
                 frontend_gui_rect(&gui, capital, viewport, &menu->holding_capital);
    if (!valid) { frontend_gui_free(&gui); return SDL_SetError("Missing province or capital GUI position"); }

    /* The original panel's size={1024,224} is a container, NOT the visible
     * background. The close button at x=320 and 3x109px holdings establish
     * its 344px playable width. Stretch the matching CK2 sprite only when
     * an installed mod provides a smaller copy. */
    menu->holding_panel.w = 344;
    menu->holding_panel.h = HEIGHT - menu->holding_panel.y;
    if (!background) valid = false;
    if (setup_widget_load_sprite(&menu->province_skin.background, menu->renderer,
          menu->asset_root, "interface/province.gfx", "GFX_province_bg",
          menu->holding_panel) < 0) SDL_ClearError();

    province_bind_rect(&gui, panel, "pv_county_shield", &menu->province_shield_area);
    const int close_id = frontend_gui_find(&gui, panel, "button_close");
    SDL_Rect close_area = {0};
    if (frontend_gui_rect(&gui, close_id, viewport, &close_area)) {
        if (setup_widget_load_node(&menu->province_close, menu->renderer,
                menu->asset_root, &gui, close_id, close_area) < 0)
            SDL_ClearError();
        menu->province_close.area = close_area;
    }
    struct province_panel_skin *skin = &menu->province_skin;
    province_bind_rect(&gui, panel, "ruler_portrait", &skin->ruler_portrait);
    province_bind_rect(&gui, panel, "province_name", &skin->name);
    province_bind_rect(&gui, panel, "kingdom", &skin->kingdom);
    province_bind_rect(&gui, panel, "duchy", &skin->duchy);
    province_bind_rect(&gui, panel, "culture", &skin->culture);
    province_bind_rect(&gui, panel, "religion", &skin->religion);
    province_bind_rect(&gui, panel, "label_culture", &skin->culture_label);
    province_bind_rect(&gui, panel, "label_religion", &skin->religion_label);
    province_bind_rect(&gui, panel, "total_tax_value", &skin->tax_amount);
    province_bind_rect(&gui, panel, "label_supply_limit", &skin->supply_label);
    province_bind_rect(&gui, panel, "label_revoltrisk", &skin->revolt_label);
    static const char *const tech_value_nodes[3] = {
        "miltech_value", "ecotech_value", "cultech_value"
    };
    for (int t=0; t<3; ++t)
        province_bind_rect(&gui, panel, tech_value_nodes[t], &skin->tech_amount[t]);
    province_bind_rect(&gui, panel, "kingdom_shield", &skin->kingdom_shield);
    province_bind_rect(&gui, panel, "duchy_shield", &skin->duchy_shield);
    province_bind_widget(menu, &gui, panel, "ruler_portrait_frame", &skin->portrait_frame);
    province_bind_widget(menu, &gui, panel, "county_crown", &skin->county_crown);
    province_bind_widget(menu, &gui, panel, "show_region_info", &skin->region_info);
    province_bind_widget(menu, &gui, panel, "button_raise_levy", &skin->raise_levy);
    province_bind_widget(menu, &gui, panel, "button_raise_ships", &skin->raise_ships);
    province_bind_widget(menu, &gui, panel, "hospital_level_button", &skin->temple_button);
    province_bind_widget(menu, &gui, panel, "capital_icon", &skin->capital_icon);
    province_bind_widget(menu, &gui, panel, "make_crown_focus", &skin->crown_focus);
    province_bind_widget(menu, &gui, panel, "icon_tax", &skin->tax_icon);
    province_bind_widget(menu, &gui, panel, "icon_strait", &skin->strait_icon);
    province_bind_widget(menu, &gui, panel, "icon_route", &skin->route_icon);
    static const char *const tech_icon_nodes[3] = {
        "icon_miltech", "icon_ecotech", "icon_cultech"
    };
    for (int t=0; t<3; ++t)
        province_bind_widget(menu,&gui,panel,tech_icon_nodes[t],&skin->tech_icons[t]);
    if (setup_widget_load_sprite(&skin->levy_progress_bg, menu->renderer, menu->asset_root,
               "interface/province.gfx", "GFX_levy_progress_bg", (SDL_Rect){0}) < 0)
        SDL_ClearError();
    if (setup_widget_load_sprite(&skin->garrison_icon, menu->renderer, menu->asset_root,
               "interface/end_of_combat.gfx", "GFX_combat_siege_end_garrison", (SDL_Rect){0}) < 0)
        SDL_ClearError();
    if (setup_widget_load_sprite(&skin->levy_icon, menu->renderer, menu->asset_root,
               "interface/end_of_combat.gfx", "GFX_combat_siege_end_levy", (SDL_Rect){0}) < 0)
        SDL_ClearError();

    for (int i = 0; valid && i < 2; ++i) {
        const int box = frontend_gui_find(&gui, panel, boxes[i]);
        const int row = frontend_gui_find(&gui, -1, templates[i]);
        const int art = frontend_gui_find(&gui, row, "settlement_placeholder");
        const int frame = frontend_gui_find(&gui, row, "province_settlement_frame");
        const int owner_frame = frontend_gui_find(&gui, row, "settlement_owner_frame");
        valid = frontend_gui_rect(&gui, box, viewport, &menu->holding_rows[i]) &&
                frontend_gui_rect(&gui, row, viewport, &menu->holding_templates[i]) &&
                frontend_gui_rect(&gui, art, viewport, &menu->holding_art_area[i]) &&
                frontend_gui_rect(&gui, frame, viewport, &menu->holding_frame_area[i]) &&
                menu->holding_templates[i].w > 0;
        menu->holding_owner_frame_valid[i] = frontend_gui_rect(&gui, owner_frame, viewport,
                                                              &menu->holding_owner_frame_area[i]);
        menu->holding_art_scale[i] = 1;
        if (valid) {
            char scale[24], *end;
            copy_token(scale, sizeof scale, field(gui.blocks[art].body, "scale"));
            const float value = strtof(scale, &end);
            if (*scale && !*end && value > 0 && value <= 4)
                menu->holding_art_scale[i] = value;
        }
        static const char *const shields[3] = {"holder_shield", "control_shield", "realm_shield"};
        for (int s = 0; s < 3; ++s)
            province_bind_rect(&gui, row, shields[s], &skin->owner_shield[i][s]);
    }
    if (valid)
        bind_wonder_widget(menu, &gui, panel, "open_wonder_view", &menu->wonder.button);
    /* Capital and ordinary settlement owner frames are distinct four-frame
     * CK2 sprite sheets. Retain frames and frame width; copying the entire
     * texture produces the large overlapping black/color blocks. */
    static const char *const owner_sprites[2] = {
        "GFX_province_capitol_owner_frame", "GFX_province_settlement_owner_frame"
    };
    for (size_t i = 0; i < SDL_arraysize(owner_sprites); ++i) {
        if (setup_widget_load_sprite(&menu->holding_owner_frames[i], menu->renderer,
              menu->asset_root, "interface/province.gfx", owner_sprites[i],
              (SDL_Rect){0}) < 0)
            SDL_ClearError();
    }
    frontend_gui_free(&gui);
    if (!valid) return SDL_SetError("interface/province.gui: missing campaign holding layout");
    /* capital_position is an independent positionType: it has no province
     * parent. The settlement box positions do have that parent. */
    struct province_layout_rect region = {menu->holding_panel.x, menu->holding_panel.y,
                                          menu->holding_panel.w, menu->holding_panel.h};
    struct province_layout_rect cap = {region.x + menu->holding_capital.x,
        region.y + menu->holding_capital.y,
        menu->holding_templates[0].w, menu->holding_templates[0].h};
    struct province_layout_rect rows[2];
    for (int i = 0; i < 2; ++i)
        rows[i] = (struct province_layout_rect){menu->holding_rows[i].x, menu->holding_rows[i].y,
                         menu->holding_rows[i].w, menu->holding_rows[i].h};
    if (!province_layout_init(&skin->grid, region, cap, rows,
                              menu->holding_templates[1].w, menu->holding_templates[1].h))
        return SDL_SetError("Could not fit original CK2 holding slots inside province panel");
    menu->holding_columns = skin->grid.columns;
    skin->loaded = true;
    menu->holding_layout_ready = true;
    return 0;
}

static SDL_Rect place_holding_texture(SDL_Texture *texture, SDL_Rect origin, SDL_Rect offset, float scale)
{
    int w = 0, h = 0;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    return (SDL_Rect){origin.x + offset.x, origin.y + offset.y, (int)(w * scale), (int)(h * scale)};
}

static bool holding_origin(const struct menu *menu, size_t index, SDL_Rect *origin)
{
    struct province_layout_rect rect;
    if (!province_layout_slot(&menu->province_skin.grid, index, &rect))
        return false;
    *origin = (SDL_Rect){rect.x, rect.y, rect.w, rect.h};
    return true;
}

/* Drawing, tooltip hit tests, and building-click hit tests must agree on the
 * same rectangle, including the culture-specific frame if one was loaded. */
static SDL_Rect holding_frame_rect(const struct menu *menu, size_t index, SDL_Rect origin)
{
    SDL_Texture *frame = index ?
        (menu->styled_holding_frame ? menu->styled_holding_frame : menu->assets->holding_frame) :
        (menu->styled_holding_capital ? menu->styled_holding_capital : menu->assets->holding_capital);
    return frame ? place_holding_texture(frame, origin, menu->holding_frame_area[index != 0], 1)
                 : (SDL_Rect){0};
}


static void clear_holding_style(struct menu *menu)
{
    for (size_t i = 0; i < SDL_arraysize(menu->styled_holding_art); ++i) {
        SDL_DestroyTexture(menu->styled_holding_art[i]);
        menu->styled_holding_art[i] = NULL;
    }
    SDL_DestroyTexture(menu->styled_holding_frame);
    SDL_DestroyTexture(menu->styled_holding_capital);
    menu->styled_holding_frame = NULL;
    menu->styled_holding_capital = NULL;
    menu->holding_style_key[0] = '\0';
}

static void refresh_holding_style(struct menu *menu)
{
    const struct map_settlement_marker *marker = map_settlement_by_id(&menu->settlement_scene,
                                                   menu->selected ? menu->selected->id : 0);
    const char *style = marker && marker->graphical[0] ? marker->graphical : "westerngfx";
    const char *religion = menu->holdings.religion_key;
    char religious_graphic[64] = "westerngfx";
    if (*religion && !religion_style_cache_get(&menu->religion_styles, religion,
                                                religious_graphic, sizeof religious_graphic)) {
        struct ck2_asset_files files = ck2_list_assets(menu->asset_root,"common/religions");
        for (size_t k=0; k<files.count; ++k) {
            const char *file=files.names[k];
            const size_t length=strlen(file);
            if (length < 4 || strcmp(file+length-4,".txt")) continue;
            char *data=load_from_directory(menu->asset_root,"common/religions",file);
            if (data) {
                char candidate[64];
                if (religion_gfx_from_text(data,religion,candidate,sizeof candidate))
                    SDL_strlcpy(religious_graphic,candidate,sizeof religious_graphic);
                SDL_free(data);
            }
        }
        ck2_free_asset_files(&files);
        religion_style_cache_put(&menu->religion_styles, religion, religious_graphic);
    }
    /* The image depends on graphical style, not religion ID. Catholic and
     * Orthodox provinces can reuse the same temple sprite without a reload. */
    char style_key[160];
    SDL_snprintf(style_key,sizeof style_key,"%s|%s",style,religious_graphic);
    if (!strcmp(style_key,menu->holding_style_key)) return;
    clear_holding_style(menu);
    SDL_strlcpy(menu->holding_style_key,style_key,sizeof menu->holding_style_key);
    static const char *const names[HOLDING_TYPE_COUNT] = {
        NULL, "GFX_settlement_castle", "GFX_settlement_city", NULL,
        "GFX_settlement_tribal", "GFX_settlement_nomad"
    };
    for (int i = HOLDING_CASTLE; i < HOLDING_TYPE_COUNT; ++i) {
        if (!names[i]) continue;
        char sprite[160];
        SDL_snprintf(sprite, sizeof sprite, "%s_%s", names[i], style);
        struct setup_widget widget = {0};
        if (setup_widget_load_sprite(&widget, menu->renderer, menu->asset_root,
                      "interface/province.gfx", sprite, (SDL_Rect){0}) < 0) {
            SDL_ClearError();
            if (setup_widget_load_sprite(&widget, menu->renderer, menu->asset_root,
                      "interface/province.gfx", names[i], (SDL_Rect){0}) < 0)
                SDL_ClearError();
        }
        menu->styled_holding_art[i] = widget.texture;
    }
    /* Temple art is a combination of settlement and religious graphical
     * styles; use the original western-gfx fallback pending religion gfx. */
    char temple[160];
    SDL_snprintf(temple, sizeof temple, "GFX_settlement_temple_%s_%s", style,
                 religious_graphic);
    struct setup_widget widget = {0};
    if (setup_widget_load_sprite(&widget, menu->renderer, menu->asset_root,
          "interface/province.gfx", temple, (SDL_Rect){0}) < 0) {
        SDL_ClearError();
        setup_widget_load_sprite(&widget, menu->renderer, menu->asset_root,
             "interface/province.gfx", "GFX_settlement_temple_westerngfx_westerngfx",
             (SDL_Rect){0});
        SDL_ClearError();
    }
    menu->styled_holding_art[HOLDING_TEMPLE] = widget.texture;
    SDL_snprintf(temple, sizeof temple, "GFX_province_settlement_frame_%s", style);
    widget = (struct setup_widget){0};
    if (setup_widget_load_sprite(&widget, menu->renderer, menu->asset_root,
          "interface/province.gfx", temple, (SDL_Rect){0}) == 0)
        menu->styled_holding_frame = widget.texture;
    else SDL_ClearError();
    SDL_snprintf(temple, sizeof temple, "GFX_province_capital_settlement_frame_%s", style);
    widget = (struct setup_widget){0};
    if (setup_widget_load_sprite(&widget, menu->renderer, menu->asset_root,
          "interface/province.gfx", temple, (SDL_Rect){0}) == 0)
        menu->styled_holding_capital = widget.texture;
    else SDL_ClearError();
}

static void draw_small_title_flag(SDL_Renderer *renderer, SDL_Texture *flag, SDL_Rect rect)
{
    if (!flag || rect.w <= 0 || rect.h <= 0) return;
    /* The title art is itself a shield silhouette. Do not stretch it into a
     * square or substitute a dynasty flag for a barony owner. */
    int w=0,h=0;
    SDL_QueryTexture(flag, NULL, NULL, &w, &h);
    if (w <= 0 || h <= 0) return;
    const int bound_w = rect.w > 60 ? 60 : rect.w;
    const int bound_h = rect.h > 70 ? 70 : rect.h;
    const float scale = SDL_min((float)bound_w/w, (float)bound_h/h);
    const SDL_Rect target = {rect.x + (bound_w - (int)(w*scale))/2,
        rect.y + (bound_h-(int)(h*scale))/2, (int)(w*scale), (int)(h*scale)};
    if (target.w > 0 && target.h > 0) SDL_RenderCopy(renderer, flag, NULL, &target);
}

static void draw_campaign_holdings(SDL_Renderer *renderer, const struct menu_assets *assets,
                                   const struct menu *menu)
{
    const struct county_holdings *county = &menu->holdings;
    const struct province_panel_skin *skin = &menu->province_skin;
    /* Original CK2 has a wider virtual list for the DLC-extended view. That
     * list MUST NOT paint through the map in the ordinary province window. */
    SDL_Rect previous_clip;
    const SDL_bool clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetClipRect(renderer, &previous_clip);
    SDL_RenderSetClipRect(renderer, &menu->holding_panel);
    for (size_t i = 0; i < county->capacity; ++i) {
        SDL_Rect origin;
        if (!holding_origin(menu, i, &origin)) break;
        const int template = i ? 1 : 0;
        SDL_Texture *frame = i ? (menu->styled_holding_frame ? menu->styled_holding_frame
                                    : assets->holding_frame) :
            (menu->styled_holding_capital ? menu->styled_holding_capital : assets->holding_capital);
        const struct holding *slot = i < county->count ? &county->slots[i] : NULL;
        const enum holding_type kind = slot ? slot->type : HOLDING_EMPTY;
        SDL_Texture *art = menu->styled_holding_art[kind] ?
                           menu->styled_holding_art[kind] : assets->holding_art[kind];
        if (!frame) continue;
        const SDL_Rect frame_rect = holding_frame_rect(menu, i, origin);
        SDL_RenderCopy(renderer, frame, NULL, &frame_rect);
        /* Render only ONE of the four original CK2 owner-frame sprite cells.
         * The frame is a backdrop; it belongs underneath settlement artwork.
         * CK2's exact color/state selection is still being investigated, so
         * use frame zero instead of inventing an ownership state. */
        const struct setup_widget *owner_frame = &menu->holding_owner_frames[template];
        if (slot && owner_frame->texture && menu->holding_owner_frame_valid[template]) {
            int sheet_w = 0, sheet_h = 0;
            struct ck2_frame_rect cell;
            if (SDL_QueryTexture(owner_frame->texture, NULL, NULL, &sheet_w, &sheet_h) == 0 &&
                ck2_sprite_frame_rect(sheet_w, sheet_h, owner_frame->frames, 0, &cell)) {
                const SDL_Rect source = {cell.x, cell.y, cell.w, cell.h};
                const SDL_Rect target = {origin.x + menu->holding_owner_frame_area[template].x,
                    origin.y + menu->holding_owner_frame_area[template].y,
                    source.w, source.h};
                SDL_RenderCopy(renderer, owner_frame->texture, &source, &target);
            }
        }
        if (art) {
            const SDL_Rect art_rect = place_holding_texture(art, origin,
                menu->holding_art_area[template], menu->holding_art_scale[template]);
            SDL_RenderCopy(renderer, art, NULL, &art_rect);
        }
        if (slot && skin->barony_flags[i]) {
            SDL_Rect label = skin->owner_shield[template][0];
            label.x += origin.x;
            label.y += origin.y;
            label.w = 30; label.h = 34;
            draw_small_title_flag(renderer, skin->barony_flags[i], label);
            /* Only the actual barony title is shown until distinct controller
             * and realm ownership are available; never invent their crests. */
        }
        if (slot && skin->levy_progress_bg.texture) {
            for (int j = 0; j < 2; ++j) {
                SDL_Rect bar = {origin.x + (template ? (j ? 68 : 16) : (j ? 81 : 24)),
                                origin.y + (template ? 90 : 118), 0, 0};
                SDL_QueryTexture(skin->levy_progress_bg.texture, NULL, NULL, &bar.w, &bar.h);
                SDL_RenderCopy(renderer, skin->levy_progress_bg.texture, NULL, &bar);
            }
        }
    }
    SDL_RenderSetClipRect(renderer, clipped ? &previous_clip : NULL);
}

static void setup_label(SDL_Renderer *, const struct menu_assets *, const char *, SDL_Rect, int);

static bool load_settlement_layout(struct menu *menu, const struct frontend_gui *gui, SDL_Point viewport)
{
    const int panel = frontend_gui_find(gui, -1, "build_settlement_view");
    const int entry = frontend_gui_find(gui, -1, "build_settlement_entry");
    const struct { int parent; const char *name; SDL_Rect *area; } items[] = {
        {panel, "headline", &menu->settlement_title},
        {panel, "settlement_list", &menu->settlement_list},
        {panel, "button_close", &menu->settlement_close},
        {entry, "settlement_type", &menu->settlement_art},
        {entry, "name", &menu->settlement_name},
        {entry, "buildtime", &menu->settlement_time},
        {entry, "cost", &menu->settlement_cost},
        {entry, "build_banner", &menu->settlement_button},
    };
    SDL_Rect row;
    if (!frontend_gui_rect(gui, panel, viewport, &menu->settlement_panel) ||
        !frontend_gui_rect(gui, entry, viewport, &row) || row.h <= 0)
        return false;
    for (size_t i = 0; i < SDL_arraysize(items); ++i) {
        if (!frontend_gui_rect(gui, frontend_gui_find(gui, items[i].parent, items[i].name),
                               viewport, items[i].area))
            return false;
    }
    menu->settlement_row_height = row.h;
    return true;
}

static int load_building_layout(struct menu *menu)
{
    struct frontend_gui gui = {0};
    if (frontend_gui_load_file(&gui, menu->asset_root, "interface/buildview.gui") < 0)
        return -1;
    const SDL_Point viewport = {WIDTH, HEIGHT};
    const int panel = frontend_gui_find(&gui, -1, "build_buildings_view");
    const int entry = frontend_gui_find(&gui, -1, "build_building_entry");
    const int art = frontend_gui_find(&gui, panel, "settlement_icon");
    SDL_Rect row;
    const bool valid = frontend_gui_rect(&gui, panel, viewport, &menu->building_panel) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "building_list"), viewport, &menu->building_list) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "title_label"), viewport, &menu->building_title) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "cancel_button"), viewport, &menu->building_close_area) &&
        frontend_gui_rect(&gui, art, viewport, &menu->building_art_area) &&
        frontend_gui_rect(&gui, entry, viewport, &row) && row.h > 0 && menu->building_list.h >= row.h;
    const bool settlement_ready = load_settlement_layout(menu, &gui, viewport);
    frontend_gui_free(&gui);
    if (!valid)
        return SDL_SetError("interface/buildview.gui: missing construction layout");
    menu->building_row_height = row.h;
    SDL_Texture *background = ck2_load_texture(menu->renderer, menu->asset_root,
                                              "gfx/interface/buildview_bg.dds");
    SDL_Texture *item = ck2_load_texture(menu->renderer, menu->asset_root,
                                        "gfx/interface/buildview_listitem.dds");
    SDL_Texture *close = ck2_load_texture(menu->renderer, menu->asset_root,
                                         "gfx/interface/main_close_button.dds");
    if (!background || !item || !close) {
        SDL_DestroyTexture(background);
        SDL_DestroyTexture(item);
        SDL_DestroyTexture(close);
        return SDL_SetError("Missing construction window sprites");
    }
    menu->building_bg = background;
    menu->building_row = item;
    menu->building_close = close;
    if (settlement_ready)
        menu->settlement_bg = ck2_load_texture(menu->renderer, menu->asset_root,
                                              "gfx/interface/build_settlement_bg.dds");
    if (menu->settlement_bg) {
        SDL_QueryTexture(menu->settlement_bg, NULL, NULL, &menu->settlement_panel.w, &menu->settlement_panel.h);
        const char *button = "gfx/interface/standard_button_112.dds";
        menu->settlement_build = ck2_load_texture(menu->renderer, menu->asset_root, button);
        menu->settlement_hover = ck2_load_hover(menu->renderer, menu->asset_root, button);
        SDL_QueryTexture(menu->settlement_build ? menu->settlement_build : menu->assets->compact_button,
                         NULL, NULL, &menu->settlement_button.w, &menu->settlement_button.h);
        SDL_strlcpy(menu->settlement_headline, "Create Holding", sizeof menu->settlement_headline);
        SDL_strlcpy(menu->settlement_build_label, "Build", sizeof menu->settlement_build_label);
        SDL_strlcpy(menu->settlement_types[0], "Castle", sizeof menu->settlement_types[0]);
        SDL_strlcpy(menu->settlement_types[1], "City", sizeof menu->settlement_types[1]);
        SDL_strlcpy(menu->settlement_types[2], "Temple", sizeof menu->settlement_types[2]);
        struct ck2_translation labels[] = {
            {"FOUND_SETTLEMENT", menu->settlement_headline, sizeof menu->settlement_headline},
            {"SM_BUILD", menu->settlement_build_label, sizeof menu->settlement_build_label},
            {"castle", menu->settlement_types[0], sizeof menu->settlement_types[0]},
            {"city", menu->settlement_types[1], sizeof menu->settlement_types[1]},
            {"temple", menu->settlement_types[2], sizeof menu->settlement_types[2]},
        };
        ck2_translate(menu->asset_root, labels, SDL_arraysize(labels));
    }
    SDL_QueryTexture(background, NULL, NULL, &menu->building_panel.w, &menu->building_panel.h);
    SDL_QueryTexture(close, NULL, NULL, &menu->building_close_area.w, &menu->building_close_area.h);
    menu->settlement_close.w = menu->building_close_area.w;
    menu->settlement_close.h = menu->building_close_area.h;
    return 0;
}

static const struct holding *viewed_holding(const struct menu *menu)
{
    return menu->building_slot < menu->holdings.count ? &menu->holdings.slots[menu->building_slot] : NULL;
}

static bool owns_county(const struct menu *menu, struct realm_title_info *title)
{
    return menu->campaign_ruler_id && menu->selected &&
        realm_county_info(&menu->realms, (size_t)(menu->selected - menu->provinces.provinces), false, title) &&
        title->holder == menu->campaign_ruler_id;
}

static bool can_build_at(const struct menu *menu, const struct holding *holding)
{
    struct realm_title_info title;
    if (!menu->campaign_ruler_id || !menu->selected)
        return false;
    if (construction_owns_settlement(&menu->construction, holding->id))
        return true;
    if (realm_title_info(&menu->realms, holding->id, &title) && title.holder)
        return title.holder == menu->campaign_ruler_id;
    return menu->building_slot == 0 && owns_county(menu, &title);
}

static bool government_player_can_found(const struct menu *menu)
{
    const struct ruler_profile *r=realm_ruler(&menu->realms,menu->campaign_ruler_id);
    const struct government_definition *gov=r ? government_find(
        &menu->settlement_scene.governments,r->government_key) : NULL;
    return government_can_found_holding(gov);
}

static bool settlement_candidate(const struct menu *menu, struct realm_title_info *barony)
{
    struct realm_title_info county;
    return owns_county(menu, &county) && realm_vacant_barony(&menu->realms, county.id, &menu->holdings, barony);
}

static SDL_Rect settlement_row_area(const struct menu *menu, SDL_Rect area, enum holding_type type)
{
    area.x += menu->settlement_list.x;
    area.y += menu->settlement_list.y + (type - HOLDING_CASTLE) * menu->settlement_row_height;
    return area;
}

static SDL_Rect settlement_build_area(const struct menu *menu, enum holding_type type)
{
    return settlement_row_area(menu, menu->settlement_button, type);
}

static bool settlement_view(const struct menu *menu)
{
    return menu->settlement_bg && !viewed_holding(menu);
}

static void draw_settlement(SDL_Renderer *renderer, const struct menu_assets *assets, const struct menu *menu)
{
    SDL_RenderCopy(renderer, menu->settlement_bg, NULL, &menu->settlement_panel);
    SDL_RenderCopy(renderer, menu->building_close, NULL, &menu->settlement_close);
    const SDL_Rect title = menu->settlement_title;
    campaign_text(renderer, assets, menu->settlement_headline, title.x + title.w / 2, title.y);
    const struct holding *pending;
    const unsigned remaining = construction_settlement_pending(&menu->construction, menu->selected->id, &pending);
    char line[160];
    if (remaining) {
        snprintf(line, sizeof line, "%.96s: %u days left", pending->name, remaining);
        campaign_text(renderer, assets, line, title.x + title.w / 2, menu->settlement_list.y + 24);
        return;
    }
    if (!government_player_can_found(menu)) {
        campaign_text(renderer, assets, "Your CK2 government cannot found new holdings",
                      title.x + title.w / 2, menu->settlement_list.y + 24);
        return;
    }
    struct realm_title_info barony;
    if (!settlement_candidate(menu, &barony)) {
        campaign_text(renderer, assets, "Requires direct ownership and a vacant barony",
                      title.x + title.w / 2, menu->settlement_list.y + 24);
        return;
    }
    snprintf(line, sizeof line, "%.96s   |   %zu / %zu slots", barony.name,
             menu->holdings.count, menu->holdings.capacity);
    campaign_text(renderer, assets, line, title.x + title.w / 2, title.y + 28);
    for (enum holding_type type = HOLDING_CASTLE; type <= HOLDING_TEMPLE; ++type) {
        unsigned gold, days;
        const bool valid = construction_settlement_quote(&menu->construction, &menu->holdings,
                                                          menu->selected->id, type, &gold, &days);
        SDL_Rect art = settlement_row_area(menu, menu->settlement_art, type);
        SDL_Texture *texture = assets->holding_art[type];
        SDL_QueryTexture(texture, NULL, NULL, &art.w, &art.h);
        art.w /= 2;
        art.h /= 2;
        SDL_RenderCopy(renderer, texture, NULL, &art);
        SDL_SetTextureColorMod(assets->font, 35, 25, 15);
        setup_label(renderer, assets, menu->settlement_types[type - HOLDING_CASTLE],
                    settlement_row_area(menu, menu->settlement_name, type), 0);
        SDL_SetTextureColorMod(assets->font, 255, 255, 255);
        if (valid) {
            snprintf(line, sizeof line, "%u Days", days);
            setup_label(renderer, assets, line, settlement_row_area(menu, menu->settlement_time, type), 0);
            snprintf(line, sizeof line, "%u gold", gold);
            setup_label(renderer, assets, line, settlement_row_area(menu, menu->settlement_cost, type), 0);
        } else {
            setup_label(renderer, assets, "Unavailable", settlement_row_area(menu, menu->settlement_time, type), 0);
        }
        const SDL_Rect button = settlement_build_area(menu, type);
        const bool enabled = valid && menu->campaign.treasury >= gold;
        SDL_Texture *sprite = menu->settlement_build ? menu->settlement_build : assets->compact_button;
        if (enabled && SDL_PointInRect(&menu->pointer, &button))
            sprite = menu->settlement_hover ? menu->settlement_hover : assets->compact_hover;
        SDL_SetTextureColorMod(sprite, enabled ? 255 : 110, enabled ? 255 : 110, enabled ? 255 : 110);
        SDL_RenderCopy(renderer, sprite, NULL, &button);
        SDL_SetTextureColorMod(sprite, 255, 255, 255);
        setup_label(renderer, assets, menu->settlement_build_label, button, 0);
    }
}

static size_t building_count(const struct menu *menu, const struct holding *holding)
{
    size_t count = 0;
    for (size_t i = 0; holding && i < menu->construction.count; ++i)
        count += menu->construction.catalog[i].type == holding->type;
    return count;
}

static void scroll_buildings(struct menu *menu, int amount)
{
    const int count = (int)building_count(menu, viewed_holding(menu));
    const int rows = menu->building_list.h / menu->building_row_height;
    menu->building_scroll = (size_t)SDL_clamp((int)menu->building_scroll - amount, 0, SDL_max(0, count - rows));
}

static SDL_Rect building_row(const struct menu *menu, size_t row)
{
    SDL_Rect rect = menu->building_list;
    rect.y += (int)row * menu->building_row_height;
    rect.h = menu->building_row_height;
    return rect;
}

static void draw_buildings(SDL_Renderer *renderer, const struct menu_assets *assets,
                           const struct menu *menu)
{
    if (!menu->building_open)
        return;
    if (settlement_view(menu)) {
        draw_settlement(renderer, assets, menu);
        return;
    }
    const SDL_Rect panel = menu->building_panel, title = menu->building_title;
    const struct holding *holding = viewed_holding(menu);
    SDL_RenderCopy(renderer, menu->building_bg, NULL, &panel);
    SDL_RenderCopy(renderer, menu->building_close, NULL, &menu->building_close_area);
    campaign_text(renderer, assets, holding ? (*holding->name ? holding->name : holding->id)
                                            : "Empty holding slot", title.x + title.w / 2, title.y);
    if (!holding) {
        char slots[96];
        snprintf(slots, sizeof slots, "%zu of %zu county holding slots occupied",
                 menu->holdings.count, menu->holdings.capacity);
        campaign_text(renderer, assets, slots, panel.x + panel.w / 2, panel.y + 102);
        campaign_text(renderer, assets, "New holding construction pending vanilla research",
                      panel.x + panel.w / 2, panel.y + 132);
        return;
    }

    SDL_Texture *art = assets->holding_art[holding->type];
    SDL_Rect art_area = menu->building_art_area;
    SDL_QueryTexture(art, NULL, NULL, &art_area.w, &art_area.h);
    SDL_RenderCopy(renderer, art, NULL, &art_area);
    const bool owns = can_build_at(menu, holding);
    campaign_text(renderer, assets, owns ? "Select an available building to construct it"
                                        : "You do not directly control this holding",
                  panel.x + panel.w / 2, panel.y + 122);
    const size_t rows = (size_t)(menu->building_list.h / menu->building_row_height);
    int width = 0, height = 0;
    SDL_QueryTexture(menu->building_row, NULL, NULL, &width, &height);
    static const char *const labels[] = {
        "Build", "Built", "", "Holding busy", "Requirements", "Inspect only", "Needs gold"
    };
    for (size_t i = 0, index = 0; i < menu->construction.count && index < menu->building_scroll + rows; ++i) {
        const struct building *building = &menu->construction.catalog[i];
        if (building->type != holding->type)
            continue;
        if (index++ < menu->building_scroll)
            continue;
        unsigned days;
        const enum building_status status = construction_status(&menu->construction, holding,
            i, owns, menu->campaign.treasury, &days);
        const SDL_Rect area = building_row(menu, index - 1 - menu->building_scroll);
        const bool hover = status == BUILD_AVAILABLE && SDL_PointInRect(&menu->pointer, &area);
        const SDL_Rect source = {hover ? width / 3 : 0, 0, width / 3, height};
        SDL_RenderCopy(renderer, menu->building_row, &source, &area);
        char detail[96];
        if (status == BUILD_RUNNING)
            snprintf(detail, sizeof detail, "%u days left", days);
        else if (status == BUILD_AVAILABLE || status == BUILD_UNAFFORDABLE)
            snprintf(detail, sizeof detail, "%s (%ug, %ud)", labels[status], building->gold, building->days);
        else
            snprintf(detail, sizeof detail, "%s", labels[status]);
        setup_label(renderer, assets, *building->name ? building->name : building->id,
                    (SDL_Rect){area.x + 4, area.y, area.w * 5 / 8 - 4, area.h}, 0);
        setup_label(renderer, assets, detail,
                    (SDL_Rect){area.x + area.w * 5 / 8, area.y, area.w * 3 / 8, area.h}, 0);
    }
}

static bool building_click(struct menu *menu, SDL_Point point)
{
    if (menu->building_open && settlement_view(menu)) {
        if (SDL_PointInRect(&point, &menu->settlement_close)) {
            menu->building_open = false;
            return true;
        }
        if (SDL_PointInRect(&point, &menu->settlement_panel)) {
            struct realm_title_info barony;
            if (!government_player_can_found(menu) || !settlement_candidate(menu, &barony))
                return true;
            for (enum holding_type type = HOLDING_CASTLE; type <= HOLDING_TEMPLE; ++type) {
                const SDL_Rect button = settlement_build_area(menu, type);
                if (!SDL_PointInRect(&point, &button))
                    continue;
                struct holding holding = {.type = type};
                SDL_strlcpy(holding.id, barony.id, sizeof holding.id);
                SDL_strlcpy(holding.name, barony.name, sizeof holding.name);
                if (construction_settlement_start(&menu->construction, &menu->holdings, menu->selected->id,
                                                  &holding, &menu->campaign.treasury))
                    menu_sound_play(&menu->music, CUE_GENERIC);
                break;
            }
            return true;
        }
    }
    if (menu->building_open && !settlement_view(menu) && SDL_PointInRect(&point, &menu->building_close_area)) {
        menu->building_open = false;
        return true;
    }
    if (menu->building_open && !settlement_view(menu) && SDL_PointInRect(&point, &menu->building_panel)) {
        const struct holding *holding = viewed_holding(menu);
        SDL_Rect clickable = menu->building_list;
        clickable.h -= clickable.h % menu->building_row_height;
        if (holding && SDL_PointInRect(&point, &clickable)) {
            const size_t clicked = menu->building_scroll +
                (size_t)((point.y - menu->building_list.y) / menu->building_row_height);
            for (size_t i = 0, index = 0; i < menu->construction.count; ++i) {
                if (menu->construction.catalog[i].type != holding->type || index++ != clicked)
                    continue;
                if (construction_start(&menu->construction, holding, i, can_build_at(menu, holding),
                                       &menu->campaign.treasury))
                    menu_sound_play(&menu->music, CUE_GENERIC);
                break;
            }
        }
        return true;
    }
    if (!menu->selected)
        return false;
    for (size_t i = 0; i < menu->holdings.capacity; ++i) {
        SDL_Rect origin;
        if (!holding_origin(menu, i, &origin))
            break;
        const SDL_Rect area = holding_frame_rect(menu, i, origin);
        if (area.w > 0 && SDL_PointInRect(&point, &area)) {
            menu->building_slot = i;
            menu->building_open = true;
            menu->building_scroll = 0;
            menu_sound_play(&menu->music, CUE_GENERIC);
            return true;
        }
    }
    return false;
}

/* Warfare uses the INDEPENDENT political ruler as each side's identity.
 * County/barony ownership remains a separate historical title relationship. */
static unsigned campaign_warfare_owner(void *userdata, unsigned province_id)
{
    const struct menu *menu = userdata;
    const struct province *p = province_by_id(&menu->provinces, province_id);
    if (!p) return 0;
    struct realm_province_ownership ownership;
    if (!realm_province_ownership(&menu->realms,
                                  (size_t)(p - menu->provinces.provinces), &ownership)) return 0;
    return ownership.has_independent ? ownership.independent.holder : 0;
}
static void campaign_warfare_day(struct menu *menu)
{
    if (!menu->warfare.war_count && !menu->warfare.army_count) return;
    struct warfare_day_report report;
    warfare_day(&menu->warfare, campaign_warfare_owner, menu, &report);
    for (size_t i = 0; i < report.count; ++i) {
        const struct warfare_event *e = report.events + i;
        char line[220];
        switch (e->kind) {
        case WARFARE_ARRIVED:
            snprintf(line, sizeof line, "Army %u arrived in province %u", e->army_id, e->province_id);
            break;
        case WARFARE_BATTLE:
            snprintf(line, sizeof line, "Battle at %u: ruler %u defeated ruler %u; casualties %u/%u",
                     e->province_id, e->actor_id, e->opponent_id,
                     e->actor_losses, e->opponent_losses);
            break;
        case WARFARE_SIEGE_STARTED:
            snprintf(line, sizeof line, "Siege begun in province %u (provisional 30 days)", e->province_id);
            break;
        case WARFARE_OCCUPIED:
            snprintf(line, sizeof line, "Province %u occupied by ruler %u (no title transfer)",
                     e->province_id, e->actor_id);
            break;
        case WARFARE_ARMY_DESTROYED:
            snprintf(line, sizeof line, "Army %u destroyed", e->army_id);
            break;
        default: continue;
        }
        debug_console_output(&menu->console, line);
    }
}

static void name_runtime_newborn(struct menu *menu, unsigned newborn)
{
    const struct ruler_profile *child = family_gameplay_find(&menu->family, newborn);
    if (child && child->culture_key[0]) {
        const char *directory = "common/cultures";
        struct ck2_asset_files files = ck2_list_assets(menu->asset_root, directory);
        for (size_t i = 0; i < files.count; ++i) {
            const char *file = files.names[i];
            const size_t len = strlen(file);
            if (len < 4 || strcmp(file + len - 4, ".txt")) continue;
            char *data = load_from_directory(menu->asset_root, directory, file);
            if (!data) continue;
            char name[96];
            const bool named = family_name_choose(data, child->culture_key, child->female,
                                                   newborn ^ child->father, name, sizeof name);
            SDL_free(data);
            if (named) {
                (void)family_gameplay_name_child(&menu->family, newborn, name);
                break;
            }
        }
        ck2_free_asset_files(&files);
    }
}

static void refresh_law_income(struct menu *menu)
{
    economy_snapshot_apply_laws(&menu->economy,&menu->construction,
                                &menu->law_catalog,&menu->active_laws);
    /* Respect `no_vassal_tax` lists in original common/governments. In
     * particular, nomad lieges do not collect ordinary nomad-vassal taxes.
     * Do not change other vassals until their obligations are implemented. */
    const struct ruler_profile *tax_liege=realm_ruler(&menu->realms,menu->campaign_ruler_id);
    const struct government_definition *liege_gov=tax_liege ? government_find(
        &menu->settlement_scene.governments,tax_liege->government_key) : NULL;
    if (liege_gov && liege_gov->no_vassal_tax.count)
        for (size_t i=0;i<menu->economy.vassal_count;++i) {
            const struct taxable_vassal_holding *v=menu->economy.vassal_holdings+i;
            const struct ruler_profile *vassal=realm_ruler(&menu->realms,v->holder_id);
            const struct government_definition *type=vassal ? government_find(
                &menu->settlement_scene.governments,vassal->government_key) : NULL;
            if (!government_vassal_tax_exempt(liege_gov,type)) continue;
            double annual,rate;
            if (ck2_law_holding_tax(&menu->active_laws,&menu->law_catalog,
                                   v->holding.type,&rate) &&
                construction_holding_annual_tax(&menu->construction,&v->holding,&annual))
                menu->economy.annual_law_vassal_tax-=annual*rate;
        }
    if (menu->economy.annual_law_vassal_tax<0)
        menu->economy.annual_law_vassal_tax=0;
    menu->campaign.monthly_tax=ck2_tax_monthly(menu->economy.annual_direct_tax+
                                                menu->economy.annual_law_vassal_tax);
    /* Source-defined nomad population tax is monthly. Only apply it to a
     * registered player clan: the initial unverified population is UNKNOWN. */
    menu->campaign.monthly_tax+=ck2_nomad_monthly_tax(&menu->government_mechanics,
                                              &menu->government_rules,menu->campaign_ruler_id);
    /* CK2 trade-post buildings contain explicit annual tax_income modifiers.
     * Trade *value* and trade-zone calculations are not equivalent to income. */
    for (size_t post=0;post<menu->government_mechanics.post_count;++post) {
        const struct ck2_trade_post *p=menu->government_mechanics.posts+post;
        if (p->patrician==menu->campaign_ruler_id)
            menu->campaign.monthly_tax+=ck2_trade_annual_tax(p,&menu->trade_catalog)/12.0;
    }
    menu->campaign.monthly_tax_known=menu->economy.missing_tax_definitions==0 &&
                                     menu->economy.vassal_tax_skipped==0;
    menu->campaign.demesne_limit=menu->economy.demesne_limit;
    menu->campaign.demesne_limit_known=menu->economy.has_demesne_limit;
    const struct ck2_law *centralization=ck2_law_selected(&menu->active_laws,
                                  &menu->law_catalog,"centralization");
    if (centralization && centralization->has_demesne_size &&
        menu->economy.has_demesne_limit) {
        const double limit=(double)menu->economy.demesne_limit+centralization->demesne_size;
        menu->campaign.demesne_limit=limit<=0 ? 0 : limit>=UINT_MAX ? UINT_MAX : (unsigned)limit;
    }
}

static void advance_campaign_day(struct menu *menu)
{
    const bool just_completed = construction_day(&menu->construction);
    if (just_completed && menu->map_modes.selected == MAP_MODE_ECONOMY)
        map_theme_free(&menu->map_theme);
    /* Recalculate immediately when construction finishes, not only near
     * month end; the monthly receipt still happens at the month boundary. */
    if (just_completed || menu->campaign.date.day >= 28) {
        economy_snapshot_refresh(&menu->economy, &menu->construction);
        refresh_law_income(menu);
        menu->campaign.demesne_count = menu->economy.direct_count;
    }
    const int previous_month=menu->campaign.date.month;
    const int previous_year=menu->campaign.date.year;
    campaign_advance_day(&menu->campaign);
    ck2_trade_day(&menu->government_mechanics);
    if (menu->campaign.date.month!=previous_month || menu->campaign.date.year!=previous_year) {
        ck2_nomad_month(&menu->government_mechanics,&menu->government_rules);
        refresh_law_income(menu);
    }
    campaign_warfare_day(menu);
    /* Several pregnancies can finish on the same date. Process each without
     * arbitrarily delaying later births until tomorrow. */
    for (unsigned delivered = 0; delivered < 64; ++delivered) {
        const unsigned newborn = family_gameplay_day(&menu->family, menu->campaign.date);
        if (!newborn) break;
        name_runtime_newborn(menu, newborn);
        char message[160];
        snprintf(message, sizeof message, "Birth: new child ID %u (runtime family history)", newborn);
        debug_console_output(&menu->console, message);
        /* Rebuild the open family panel on its next inspection. */
        menu->character.ruler.id = 0;
    }
    if (menu->selected)
        construction_settlement_apply(&menu->construction, menu->selected->id, &menu->holdings);
}

enum { BAR_EXTENSION, BAR_PAUSE, BAR_SPEED, BAR_UP, BAR_DOWN, BAR_RESOURCE };

static void load_campaign_topbar(struct menu *menu)
{
    if (menu->topbar_ready)
        return;
    static const char *const sprites[] = {
        "topbar_right_bg_extension", "icon_topbar_pause_big", "strip_gamespeed",
        "icon_topbar_speedup", "icon_topbar_speeddown", "icon_topbar_gold", "icon_topbar_prestige",
        "icon_topbar_piety", "icon_topbar_demesnesize", "icon_topbar_vassallimit", "icon_topbar_vassalnr",
        "icon_score",
    };
    static const char *const labels[] = {
        "gold", "prestige", "piety", "demesnesize", "vassallimit", "vassalnr", "score", "topbar_date"
    };
    static const int shifts[] = {2, 2, 2, 1, 0, 0, 0, 2};
    struct frontend_gui gui = {0};
    const SDL_Point viewport = {WIDTH, HEIGHT};
    if (frontend_gui_load_file(&gui, menu->asset_root, "interface/main.gui") < 0)
        return;
    SDL_Rect extension, shift;
    bool ok = frontend_gui_rect(&gui, frontend_gui_find(&gui, -1, "topbar_right_bg_extension_position"),
                                viewport, &extension) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, -1, "topbar_right_bg_offset"), viewport, &shift);
    for (size_t i = 0; ok && i < SDL_arraysize(sprites); ++i) {
        const int node = frontend_gui_find(&gui, -1, sprites[i]);
        SDL_Rect area;
        ok = frontend_gui_rect(&gui, node, viewport, &area) &&
            setup_widget_load_node(&menu->topbar[i], menu->renderer, menu->asset_root, &gui, node, area) == 0;
        if (ok && i >= BAR_RESOURCE)
            menu->topbar[i].area.x += shift.x * shifts[i - BAR_RESOURCE];
    }
    for (size_t i = 0; ok && i < SDL_arraysize(labels); ++i) {
        ok = frontend_gui_rect(&gui, frontend_gui_find(&gui, -1, labels[i]), viewport, &menu->topbar_values[i]);
        menu->topbar_values[i].x += shift.x * shifts[i];
    }
    if (ok) {
        menu->topbar[BAR_EXTENSION].area.x = WIDTH + extension.x;
        menu->topbar[BAR_EXTENSION].area.y = extension.y;
        menu->topbar_offset = shift.x;
    }
    if (ok) {
        static const char *const keys[7][2] = {
            {"STAT_WEALTH", "STAT_WEALTH_DESC"}, {"STAT_PRESTIGE", "STAT_PRESTIGE_DESC"},
            {"STAT_PIETY", "STAT_PIETY_DESC"}, {"STAT_DEMESNE_SIZE", "STAT_DEMESNE_SIZE_DESC"},
            {"STAT_VASSAL_LIMIT", "STAT_VASSAL_LIMIT_DESC"}, {"STAT_REALM_SIZE", "STAT_REALM_SIZE_DESC"},
            {"STAT_SCORE", "STAT_SCORE_DESC"},
        };
        struct ck2_translation terms[14];
        for (size_t i = 0; i < SDL_arraysize(keys); ++i) {
            terms[2 * i] = (struct ck2_translation){keys[i][0], menu->topbar_labels[i], sizeof menu->topbar_labels[i]};
            terms[2 * i + 1] = (struct ck2_translation){keys[i][1], menu->topbar_help[i], sizeof menu->topbar_help[i]};
        }
        ck2_translate(menu->asset_root, terms, SDL_arraysize(terms));
    }
    frontend_gui_free(&gui);
    menu->topbar_ready = ok;
    if (!ok) {
        SDL_Log("Campaign top bar: %s", SDL_GetError());
        for (size_t i = 0; i < SDL_arraysize(menu->topbar); ++i) {
            SDL_DestroyTexture(menu->topbar[i].texture);
            menu->topbar[i] = (struct setup_widget){0};
        }
    }
}

static unsigned campaign_day_interval(int speed)
{
    static const unsigned delay[] = {1000, 500, 250, 125, 62};
    return delay[SDL_clamp(speed, 1, 5) - 1];
}

static void draw_campaign_topbar(SDL_Renderer *renderer, const struct menu_assets *assets,
                                 const struct menu *menu)
{
    char date[48], gold[24];
    format_campaign_date(date, sizeof date, menu->campaign.date);
    snprintf(gold, sizeof gold, "%u", menu->campaign.treasury);
    if (!menu->topbar_ready) {
        campaign_text(renderer, assets, gold, 1408, 19);
        SDL_RenderCopy(renderer, assets->hud_date, NULL, &(SDL_Rect){1331, 40, 216, 24});
        campaign_text(renderer, assets, date, 1439, 41);
        return;
    }
    struct setup_widget extension = menu->topbar[BAR_EXTENSION];
    setup_widget_draw(&extension, renderer, menu->pointer);
    extension.area.x += menu->topbar_offset;
    setup_widget_draw(&extension, renderer, menu->pointer);
    for (size_t i = BAR_PAUSE; i < BAR_RESOURCE + 7; ++i) {
        if (i == BAR_SPEED) {
            const struct setup_widget *widget = &menu->topbar[i];
            const int frame = menu->time_running ? SDL_min(menu->campaign_speed, widget->frames - 1) : 0;
            const SDL_Rect source = {widget->area.w * frame, 0, widget->area.w, widget->area.h};
            SDL_RenderCopy(renderer, widget->texture, &source, &widget->area);
        } else {
            setup_widget_draw(&menu->topbar[i], renderer, menu->pointer);
        }
    }
    const struct ruler_profile *ruler = realm_ruler(&menu->realms, menu->campaign_ruler_id);
    for (size_t i = 0; i < 7; ++i) {
        const SDL_Rect area = menu->topbar_values[i];
        char value[32] = "-";
        if (!i)
            SDL_strlcpy(value, gold, sizeof value);
        else if (i == RULER_PRESTIGE)
            snprintf(value, sizeof value, "%.0f", menu->campaign.prestige);
        else if (i == RULER_PIETY)
            snprintf(value, sizeof value, "%.0f", menu->campaign.piety);
        else if (i == 3 && menu->campaign.demesne_limit_known)
            snprintf(value, sizeof value, "%zu/%u", menu->campaign.demesne_count,
                     menu->campaign.demesne_limit);
        else if (i < RULER_RESOURCE_COUNT && ruler && (ruler->known_resources & (1u << i)))
            snprintf(value, sizeof value, "%.0f", ruler->resources[i]);
        campaign_text(renderer, assets, value, area.x + area.w / 2, area.y - 2);
    }
    const SDL_Rect area = menu->topbar_values[7];
    int width = 0;
    for (const unsigned char *p = (const unsigned char *)date; *p; ++p)
        width += assets->glyphs[*p].advance;
    campaign_text(renderer, assets, date, area.x + width / 2, area.y - 2);
}

static void draw_topbar_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets,
                                const struct menu *menu)
{
    if (!menu->topbar_ready)
        return;
    for (size_t i = 0; i < SDL_arraysize(menu->topbar_labels); ++i) {
        SDL_Rect area = menu->topbar_values[i];
        area.y = 0;
        area.h = menu->topbar[BAR_PAUSE].area.y;
        if (!SDL_PointInRect(&menu->pointer, &area))
            continue;
        if (menu->topbar_labels[i][0]) {
            char details[900];
            SDL_strlcpy(details, menu->topbar_help[i], sizeof details);
            const size_t used = strlen(details);
            if (i == 0)
                snprintf(details + used, sizeof details - used,
                         "\nDirect %.2f + modeled vassal %.2f gold/year\n"
                         "Incomplete: other obligations, taxes, exemptions and modifiers%s",
                         menu->economy.annual_direct_tax,menu->economy.annual_law_vassal_tax,
                         menu->economy.holder_fallbacks ? "\nSome capital holders are inferred" : "");
            else if (i == 1)
                snprintf(details + used, sizeof details - used,
                         "\nKnown monthly prestige: %+.2f (partial sources)", menu->campaign.monthly_prestige);
            else if (i == 2)
                snprintf(details + used, sizeof details - used,
                         "\nKnown monthly piety: %+.2f (partial sources)", menu->campaign.monthly_piety);
            else if (i == 3)
                snprintf(details + used, sizeof details - used,
                         "\nCounted personal holdings: %zu\n"
                         "Limit includes selected centralization, excludes traits and game rules",
                         menu->campaign.demesne_count);
            panel_tooltip(renderer, assets, menu->pointer, menu->topbar_labels[i],
                          details, centered_text);
        }
        return;
    }
}

/* Culture-aware 2D settlement indicators. Original model names/paths are
 * discovered from settlements.gfx, but actual 3D meshes are deferred. */
static const char *map_holding_type(enum holding_type type)
{
    static const char *const names[] = {"Empty", "Castle", "City", "Temple", "Tribal", "Nomad"};
    return type > HOLDING_EMPTY && type < HOLDING_TYPE_COUNT ? names[type] : "Holding";
}

static unsigned holding_government_bit(enum holding_type type)
{
    static const unsigned bits[] = {0, GOVERNMENT_CASTLE, GOVERNMENT_CITY,
        GOVERNMENT_TEMPLE, GOVERNMENT_TRIBAL, GOVERNMENT_NOMAD};
    return type < HOLDING_TYPE_COUNT ? bits[type] : 0;
}

static bool marker_area(const struct menu *menu, const struct map_settlement_marker *m,
                        SDL_Rect *area)
{
    SDL_Point screen;
    if (!m || !map_view_project_ground(&menu->campaign_map, m->x, m->y, &screen)) return false;
    *area = (SDL_Rect){screen.x - 13, screen.y - 13, 26, 26};
    return true;
}

static const struct map_settlement_marker *hovered_map_holding(const struct menu *menu)
{
    if (menu->campaign_map.zoom < 1.55f ||
        !campaign_map_interactive(menu, menu->pointer)) return NULL;
    const struct map_settlement_marker *nearest = NULL;
    int distance = 14 * 14;
    for (size_t i = 0; i < menu->settlement_scene.count; ++i) {
        SDL_Rect rect;
        if (!marker_area(menu, menu->settlement_scene.markers + i, &rect)) continue;
        const int dx = menu->pointer.x - (rect.x + rect.w / 2);
        const int dy = menu->pointer.y - (rect.y + rect.h / 2);
        if (dx * dx + dy * dy < distance) {
            distance = dx * dx + dy * dy;
            nearest = menu->settlement_scene.markers + i;
        }
    }
    return nearest;
}

static void map_title_shields_clear(struct menu *menu)
{
    for (size_t i = 0; i < SDL_arraysize(menu->map_title_shields); ++i) {
        SDL_DestroyTexture(menu->map_title_shields[i].texture);
        menu->map_title_shields[i] = (struct map_title_shield_cache){0};
    }
    menu->map_title_clock = 0;
}

/* Resolve the county/title shield, not a settlement thumbnail. Cache only
 * visible map shields: parsing/rendering an entire world of flags each frame
 * would be prohibitively expensive. A failed title asset is cached too. */
static SDL_Texture *map_title_shield(struct menu *menu,
                                    const struct map_settlement_marker *marker)
{
    if (!menu->realms.history) return NULL;
    const struct province *province = province_by_id(&menu->provinces, marker->province_id);
    if (!province) return NULL;
    const size_t index = (size_t)(province - menu->provinces.provinces);
    struct realm_province_ownership owners;
    if (!realm_province_ownership(&menu->realms, index, &owners) ||
        !owners.has_county || !owners.county.id[0]) return NULL;
    const struct ruler_profile *ruler = realm_ruler(&menu->realms, owners.county.holder);
    const char *religion = ruler ? ruler->religion_key : "";
    struct map_title_shield_cache *candidate = &menu->map_title_shields[0];
    for (size_t i = 0; i < SDL_arraysize(menu->map_title_shields); ++i) {
        struct map_title_shield_cache *entry = &menu->map_title_shields[i];
        if (entry->attempted && !strcmp(entry->id, owners.county.id) &&
            !strcmp(entry->religion, religion)) {
            entry->last_used = ++menu->map_title_clock;
            return entry->texture;
        }
        if (!entry->attempted || entry->last_used < candidate->last_used)
            candidate = entry;
    }
    SDL_DestroyTexture(candidate->texture);
    *candidate = (struct map_title_shield_cache){0};
    SDL_strlcpy(candidate->id, owners.county.id, sizeof candidate->id);
    SDL_strlcpy(candidate->religion, religion, sizeof candidate->religion);
    candidate->texture = era_title_shield(menu->renderer, menu->asset_root,
                                           owners.county.id, religion);
    candidate->last_used = ++menu->map_title_clock;
    candidate->attempted = true;
    return candidate->texture;
}

static void draw_map_settlements(SDL_Renderer *renderer, const struct menu_assets *assets,
                                 struct menu *menu)
{
    const float zoom = menu->campaign_map.zoom;
    if (zoom < 1.55f) return;
    SDL_Rect occupied[160];
    size_t occupied_count = 0;
    for (size_t i = 0; i < menu->settlement_scene.count; ++i) {
        const struct map_settlement_marker *m = menu->settlement_scene.markers + i;
        SDL_Rect area;
        if (!marker_area(menu, m, &area)) continue;
        if (area.x < 0 || area.y < 60 || area.x + area.w >= WIDTH || area.y + area.h >= HEIGHT - 60)
            continue;
        /* Deconflict neighboring provinces. A clicked/hovered marker still
         * uses the same projected geometry as the visible marker. */
        SDL_Rect label_area = {area.x - 44, area.y - 8, 115, 54};
        bool overlap = false;
        for (size_t j = 0; j < occupied_count; ++j)
            if (SDL_HasIntersection(&label_area, occupied + j)) { overlap = true; break; }
        if (overlap) continue;
        if (occupied_count < SDL_arraysize(occupied)) occupied[occupied_count++] = label_area;
        SDL_Texture *shield = map_title_shield(menu, m);
        if (shield) {
            /* The province's actual c_ title flag takes priority over the
             * previous castle thumbnail. A real 3D model renderer is future work. */
            SDL_RenderCopy(renderer, shield, NULL, &area);
        } else {
            /* CK2 can procedurally create title arms without a flag file.
             * Preserve the old settlement fallback for these unsupported cases. */
            SDL_Texture *art = m->type < HOLDING_TYPE_COUNT ? assets->holding_art[m->type] : NULL;
            if (art) SDL_RenderCopy(renderer, art, NULL, &area);
        }
        if (zoom > 2.05f) {
            /* The original map has model-specific labels. This is a readable
             * interim 2D label using verified localization. */
            char short_name[25];
            SDL_strlcpy(short_name, *m->name ? m->name : m->holding_id, sizeof short_name);
            SDL_SetTextureColorMod(assets->font, 26, 22, 19);
            campaign_text(renderer, assets, short_name, area.x + area.w / 2, area.y + 29);
            if (zoom > 2.65f && m->owner[0]) {
                char owner[25];
                SDL_strlcpy(owner, m->owner, sizeof owner);
                campaign_text(renderer, assets, owner, area.x + area.w / 2, area.y + 49);
            }
            SDL_SetTextureColorMod(assets->font, 255, 255, 255);
        }
    }
}

static void holding_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets,
                            const struct menu *menu, const struct map_settlement_marker *m)
{
    char heading[160], info[620];
    SDL_snprintf(heading, sizeof heading, "%s: %s", map_holding_type(m->type),
                 m->name[0] ? m->name : m->holding_id);
    const struct ruler_profile *ruler = m->holder_id ? realm_ruler(&menu->realms, m->holder_id) : NULL;
    const bool authored = ruler && ruler->government_key[0];
    const struct government_definition *candidate = NULL;
    const size_t count = authored ? 0 : government_holding_candidates(&menu->settlement_scene.governments,
        holding_government_bit(m->type), &candidate, 1);
    const char *government = authored ? ruler->government_key :
        count == 1 && candidate ? candidate->id : "Not assigned (potential rules pending)";
    SDL_snprintf(info, sizeof info, "Owner: %s%s\nHolding ID: %s\nProvince culture: %s\n"
        "Province religion: %s\nGraphic: %s\nGovernment: %s%s",
        m->owner[0] ? m->owner : "Not recorded",
        m->county_holder_fallback ? " (county holder fallback)" : "",
        m->holding_id, m->culture[0] ? m->culture : "Not recorded",
        m->religion[0] ? m->religion : "Not recorded",
        m->actor_known ? m->actor : "No matching defined model",
        government, authored ? (ruler->government_inferred ?
            (!strcmp(ruler->government_key,"roman_imperial_government") ?
                " (CK2 imperial potential)" : " (holding-backed, provisional)") :
                " (authored character history)") :
                    count == 1 ? " (candidate only)" : "");
    panel_tooltip(renderer, assets, menu->pointer, heading, info, centered_text);
}

/* Individual baronies in the province window have distinct title holders.
 * Never silently substitute the emperor for a missing barony owner. */
static const struct holding *hovered_panel_holding(const struct menu *menu)
{
    if (menu->character.open || !menu->selected || !menu->holding_layout_ready)
        return NULL;
    for (size_t i = 0; i < menu->holdings.count; ++i) {
        SDL_Rect origin;
        if (!holding_origin(menu, i, &origin)) continue;
        const SDL_Rect bounds = holding_frame_rect(menu, i, origin);
        if (bounds.w > 0 && SDL_PointInRect(&menu->pointer, &bounds))
            return menu->holdings.slots + i;
    }
    return NULL;
}

static void panel_holding_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets,
                                  const struct menu *menu, const struct holding *holding)
{
    const struct realm_title_info title = {0};
    struct realm_title_info barony = title;
    const bool recorded = realm_title_info(&menu->realms, holding->id, &barony) && barony.holder;
    const struct ruler_profile *owner = recorded ? realm_ruler(&menu->realms, barony.holder) : NULL;
    char heading[176], details[640];
    SDL_snprintf(heading, sizeof heading, "%s: %s", map_holding_type(holding->type),
                 holding->name[0] ? holding->name : holding->id);
    double annual_tax = 0;
    const bool tax_known = construction_holding_annual_tax(&menu->construction, holding, &annual_tax);
    char tax_line[96];
    if (tax_known)
        SDL_snprintf(tax_line, sizeof tax_line, "%.2f / year before modifiers", annual_tax);
    else SDL_strlcpy(tax_line, "No installed tax definition", sizeof tax_line);
    SDL_snprintf(details, sizeof details,
                 "Barony: %s\nOwner: %s\nTitle holder: %s\nProvince: %s\n"
                 "Culture: %s\nReligion: %s\nHistorical buildings: %zu\nBase + buildings: %s",
                 holding->id, owner && owner->name[0] ? owner->name : "Not recorded",
                 recorded ? "Recorded in title history" : "No explicit barony holder",
                 menu->selected ? menu->selected->name : "Unknown",
                 menu->holdings.culture_name[0] ? menu->holdings.culture_name : "Not recorded",
                 menu->holdings.religion_name[0] ? menu->holdings.religion_name : "Not recorded",
                 holding->building_count, tax_line);
    panel_tooltip(renderer, assets, menu->pointer, heading, details, centered_text);
}

/* CK2-authored building troops are available without a battle simulator;
 * current raised troops and garrison manpower are NOT. */
static bool province_potential_levy(const struct menu *menu, double *total,
                                   double units[LEVY_TROOP_COUNT])
{
    if (!menu || !menu->selected || !total) return false;
    *total = 0;
    if (units) memset(units, 0, sizeof(double) * LEVY_TROOP_COUNT);
    bool known = false;
    for (size_t h = 0; h < menu->holdings.count; ++h) {
        struct levy_potential potential;
        if (!construction_holding_levy(&menu->construction, &menu->holdings.slots[h],
                                       &potential)) continue;
        known = true;
        *total += potential.total;
        if (units) for (size_t t = 0; t < LEVY_TROOP_COUNT; ++t)
            units[t] += potential.troops[t];
    }
    return known;
}

static bool province_levy_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets,
                                 const struct menu *menu)
{
    if (!menu->selected || menu->character.open || !menu->province_skin.raise_levy.texture ||
        !SDL_PointInRect(&menu->pointer, &menu->province_skin.raise_levy.area)) return false;
    double total = 0, troops[LEVY_TROOP_COUNT] = {0};
    const bool known = province_potential_levy(menu, &total, troops);
    char details[720], line[100];
    if (known)
        snprintf(details, sizeof details, "Unraised base potential: %.0f troops\n"
                 "Uses historical holdings and completed building effects.\n", total);
    else
        snprintf(details, sizeof details,
                 "Troop definitions are not available for this province.\n");
    if (known) for (size_t i = 0; i < LEVY_TROOP_COUNT; ++i) {
        if (troops[i] < 0.5) continue;
        snprintf(line, sizeof line, "\n%s: %.0f", levy_troop_names[i], troops[i]);
        if (strlen(details) + strlen(line) < sizeof details) strcat(details, line);
    }
    const char *notice = "\n\nRaising armies, garrisons, laws, "
                         "martial, technology and reinforcement are not simulated.";
    if (strlen(details) + strlen(notice) < sizeof details) strcat(details, notice);
    panel_tooltip(renderer, assets, menu->pointer, ck2_localize("LEVY", "Levy"), details,
                  centered_text);
    return true;
}

static bool character_government_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets,
                                          const struct menu *menu)
{
    if (!menu->character.open || !menu->character.extra_icons[6].texture ||
        !SDL_PointInRect(&menu->pointer, &menu->character.extra_icons[6].area)) return false;
    struct realm_title_info title;
    const bool found = realm_primary_title(&menu->realms, menu->character.ruler.id, &title);
    const struct map_settlement_marker *marker = found ?
        map_settlement_by_id(&menu->settlement_scene, title.capital) : NULL;
    const struct government_definition *candidate = NULL;
    const size_t n = marker ? government_holding_candidates(&menu->settlement_scene.governments,
        holding_government_bit(marker->type), &candidate, 1) : 0;
    char details[500];
    const char *authored = menu->character.ruler.government_key;
    const struct government_definition *definition = authored[0] ?
        government_find(&menu->settlement_scene.governments, authored) : NULL;
    if (authored[0]) {
        SDL_snprintf(details, sizeof details,
            "Assigned government: %s\nGroup: %s\n"
            "Source: %s",
            authored, definition ? definition->group : "Not in loaded government definitions",
            menu->character.ruler.government_inferred ?
                (!strcmp(authored,"roman_imperial_government") ?
                 "CK2 imperial title/law potential" :
                 "Verified holding type (potential conditions pending)") :
                 "dated character history");
    } else if (candidate && n == 1) {
        SDL_snprintf(details, sizeof details,
            "Possible type: %s\nGroup: %s\nColor: #%06x\n"
            "Assignment status: unverified; potential rules not evaluated",
            candidate->id, candidate->group, candidate->color_rgb);
    } else {
        SDL_snprintf(details, sizeof details,
            "Loaded definitions: %zu\nGovernment selection is not yet calculated.\n"
            "CK2 evaluates potential scripts, ruler status, religion, titles, and holding types.",
            menu->settlement_scene.governments.count);
    }
    panel_tooltip(renderer, assets, menu->pointer, "Government", details, centered_text);
    return true;
}

static void province_draw_header(SDL_Renderer *, const struct menu_assets *, const struct menu *);
/* A minimal 2D counter. Projection comes from the same authored settlement
 * anchor as the provincial map, not a separate hardcoded screen coordinate. */
static void draw_campaign_armies(SDL_Renderer *renderer, const struct menu_assets *assets,
                                 const struct menu *menu)
{
    for (size_t i = 0; i < menu->warfare.army_count; ++i) {
        const struct warfare_army *army = menu->warfare.armies + i;
        const struct map_settlement_marker *marker =
            map_settlement_by_id(&menu->settlement_scene, army->province_id);
        if (!marker) continue;
        SDL_Point screen;
        if (!map_view_project_ground(&menu->campaign_map, marker->x, marker->y, &screen)) continue;
        if (screen.x < 10 || screen.x > WIDTH - 50 ||
            screen.y < 60 || screen.y > HEIGHT - 36) continue;
        SDL_Rect counter = {screen.x - 18, screen.y - 24, 46, 21};
        if (army->ruler_id == menu->campaign_ruler_id)
            SDL_SetRenderDrawColor(renderer, 25, 75, 132, 235);
        else SDL_SetRenderDrawColor(renderer, 145, 45, 43, 235);
        SDL_RenderFillRect(renderer, &counter);
        SDL_SetRenderDrawColor(renderer, 226, 205, 154, 255);
        SDL_RenderDrawRect(renderer, &counter);
        char troops[24];
        snprintf(troops, sizeof troops, "%u", army->troops);
        campaign_text(renderer, assets, troops, counter.x + 5, counter.y + 3);
    }
}

static void draw_campaign(SDL_Renderer *renderer, const struct menu_assets *assets,
                          const struct menu *menu)
{
    if (menu->province_overlay)
        SDL_RenderCopy(renderer, menu->province_overlay, NULL, NULL);
    draw_campaign_armies(renderer, assets, menu);

    blit(renderer, assets->hud_resources_left, 1260 + (menu->topbar_ready ? 2 * menu->topbar_offset : 0), 0);
    blit(renderer, assets->hud_resources_right, 1422, 0);
    blit(renderer, assets->hud_outliner, 1324, 213);
    if (!menu->character.open && menu->selected) {
        /* Province background is rendered to the authored control span, rather
         * than relying on an incorrectly small installed DDS variant. */
        SDL_RenderCopy(renderer, menu->province_skin.background.texture ?
        menu->province_skin.background.texture : assets->hud_province,
        NULL, &menu->holding_panel);
        if (menu->province_close.texture)
            setup_widget_draw(&menu->province_close, renderer, menu->pointer);
        if (menu->province_title_shield) {
            SDL_Rect shield = menu->province_shield_area;
            SDL_QueryTexture(menu->province_title_shield, NULL, NULL, &shield.w, &shield.h);
            SDL_RenderCopy(renderer, menu->province_title_shield, NULL, &shield);
        }
    }
    blit(renderer, assets->hud_minimap, 1320, 696);
    blit(renderer, assets->hud_map, 1329, 705);

    draw_campaign_topbar(renderer, assets, menu);
    campaign_text(renderer, assets, ck2_localize("OUTLINER", "Outliner"), 1420, 220);

    campaign_hud_ui_draw_background(&menu->hud_ui, renderer);
    if (menu->character.open) {
        character_view_draw(&menu->character, renderer, assets, menu->character.ruler.id == menu->campaign_ruler_id,
                            &menu->campaign, menu->pointer, centered_text);
    } else if (menu->selected) {
        province_draw_header(renderer, assets, menu);
        /* Counts belong on the bottom parchment, not over the castle image. */
        if (menu->selected)
            draw_campaign_holdings(renderer, assets, menu);
        if (menu->wonder.id[0] && menu->wonder.button.texture)
            setup_widget_draw(&menu->wonder.button, renderer, menu->pointer);
    }
    blit(renderer, assets->hud_ruler, 2, 0);
    if (menu->campaign_portrait)
        SDL_RenderCopy(renderer, menu->campaign_portrait, NULL, &(SDL_Rect){16, 18, 106, 106});
    else if (menu->campaign_ruler && menu->campaign_ruler->portrait)
        SDL_RenderCopy(renderer, menu->campaign_ruler->portrait, NULL, &(SDL_Rect){16, 18, 106, 106});
    /* Title shield is its own GUI widget (main/topbar_ruler_shield). */
    if (menu->campaign_title_shield) {
        SDL_Rect shield = menu->hud_ui.topbar_shield.area;
        if (!shield.x && !shield.y) shield = (SDL_Rect){0, 87, 55, 62};
        SDL_QueryTexture(menu->campaign_title_shield, NULL, NULL, &shield.w, &shield.h);
        SDL_RenderCopy(renderer, menu->campaign_title_shield, NULL, &shield);
        if (menu->hud_ui.topbar_crown.texture)
            setup_widget_draw(&menu->hud_ui.topbar_crown, renderer, (SDL_Point){-1,-1});
    }
    /* The normal path uses original main.gui rectangles and GFX sprite frames.
     * Raw icons remain a graceful fallback when optional GUI definitions fail. */
    for (size_t i = 0; i < SDL_arraysize(assets->hud_tabs); ++i) {
        if (menu->hud_ui.navigation[i].texture) continue;
        SDL_Texture *icon = assets->hud_tabs[i];
        const SDL_Rect fallback = {146 + (int)i * 41, 15, 32, 36};
        SDL_SetTextureColorMod(icon, 175, 175, 175);
        SDL_RenderCopy(renderer, icon, &(SDL_Rect){0, 0, 32, 36}, &fallback);
        SDL_SetTextureColorMod(icon, 255, 255, 255);
    }
    campaign_hud_ui_draw(&menu->hud_ui, renderer, menu->pointer, menu->map_modes.selected);
    draw_buildings(renderer, assets, menu);
    draw_wonder(renderer, assets, menu);
    draw_topbar_tooltip(renderer, assets, menu);
    const int hovered_mode = campaign_hud_ui_mode_hit(&menu->hud_ui, menu->pointer);
    if (province_levy_tooltip(renderer, assets, menu)) {
        /* Potential troops are informational until armies are implemented. */
    } else if (character_government_tooltip(renderer, assets, menu)) {
        /* Original government sprite is already drawn by character_view. */
    } else if (campaign_hud_ui_next_song_hit(&menu->hud_ui, menu->pointer)) {
        panel_tooltip(renderer, assets, menu->pointer,
                      ck2_localize("MINIMAP_NEXTSONG", "Next Song"),
                      "Skip to another track from the installed CK2 music files",
                      centered_text);
    } else if (hovered_mode >= 0) {
        const struct map_mode_definition *mode = map_mode_definition(hovered_mode);
        panel_tooltip(renderer, assets, menu->pointer, mode->label,
                      mode->implemented ? (hovered_mode != MAP_MODE_TERRAIN && hovered_mode != MAP_MODE_REALMS ?
                          "Historical-data overlay; color palette approximate" : "Select map mode")
                          : "Requires gameplay simulation; currently unavailable",
                      centered_text);
    } else if (campaign_hud_ui_alert_hit(&menu->hud_ui, menu->pointer) >= 0) {
        panel_tooltip(renderer, assets, menu->pointer, "Alert (visual placeholder)",
                      "CK2 alert eligibility and ordering are not implemented yet",
                      centered_text);
    } else {
        const int nav = campaign_hud_ui_nav_hit(&menu->hud_ui, menu->pointer);
        if (nav >= 0) {
            panel_tooltip(renderer, assets, menu->pointer, campaign_hud_nav_label(nav),
                          "Navigation screen not implemented yet", centered_text);
        } else if (!menu->map_gesture.active && !menu->wonder.open) {
            const struct holding *panel_holding = hovered_panel_holding(menu);
            if (panel_holding) {
                panel_holding_tooltip(renderer, assets, menu, panel_holding);
            } else {
            const struct map_settlement_marker *marker = hovered_map_holding(menu);
            if (marker) {
                holding_tooltip(renderer, assets, menu, marker);
            } else {
            const struct province *province = campaign_pointer_province(menu, menu->pointer);
            if (province) {
                const size_t index = (size_t)(province - menu->provinces.provinces);
                struct realm_province_ownership owners;
                realm_province_ownership(&menu->realms, index, &owners);
                const bool owned = owners.has_county, realm = owners.has_independent;
                const struct realm_title_info county = owners.county, independent = owners.independent;
                const struct ruler_profile *holder = owned ? realm_ruler(&menu->realms, county.holder) : NULL;
                const struct ruler_profile *liege = realm ? realm_ruler(&menu->realms, independent.holder) : NULL;
                char heading[224], details[768];
                snprintf(heading, sizeof heading, "%s (province %u)", province->name, province->id);
                const struct map_province_facts *facts =
                    index < menu->settlement_scene.province_fact_count ?
                    &menu->settlement_scene.province_facts[index] : NULL;
                snprintf(details, sizeof details,
                         "County: %s\nOwner: %s\nIndependent realm: %s\nTop liege: %s\n"
                         "Culture: %s\nReligion: %s%s%s",
                         owned ? county.name : "Not available",
                         holder && holder->name[0] ? holder->name : "Not available",
                         realm ? independent.name : "Not available",
                         liege && liege->name[0] ? liege->name : "Not available",
                         facts && facts->culture[0] ? facts->culture : "Not recorded",
                         facts && facts->religion[0] ? facts->religion : "Not recorded",
                         owned && county.holder ? "\nRight-click: view county holder" : "",
                         realm && independent.holder ? "\nShift + right-click: view top liege" : "");
                const unsigned occupier = warfare_occupation_holder(&menu->warfare, province->id);
                if (occupier) {
                    const size_t used = strlen(details);
                    snprintf(details + used, sizeof details - used,
                             "\nOccupied by ruler %u (military control only)", occupier);
                }
                for (size_t a = 0; a < menu->warfare.army_count; ++a) {
                    const struct warfare_army *army = menu->warfare.armies + a;
                    if (army->province_id != province->id) continue;
                    const size_t used = strlen(details);
                    if (sizeof details - used < 70) break;
                    snprintf(details + used, sizeof details - used,
                             "\nArmy %u: ruler %u, %u troops", army->id, army->ruler_id, army->troops);
                }
                panel_tooltip(renderer, assets, menu->pointer, heading, details, centered_text);
            }
            }
            }
        }
    }
}

static SDL_Texture *setup_character_portrait(struct menu *, const struct ruler_profile *);

static void province_clear_history_art(struct menu *menu)
{
    struct province_panel_skin *skin = &menu->province_skin;
    SDL_DestroyTexture(skin->portrait); skin->portrait = NULL;
    SDL_DestroyTexture(skin->kingdom_flag); skin->kingdom_flag = NULL;
    SDL_DestroyTexture(skin->duchy_flag); skin->duchy_flag = NULL;
    SDL_DestroyTexture(skin->independent_flag); skin->independent_flag = NULL;
    for (size_t i=0; i<SDL_arraysize(skin->barony_flags); ++i) {
        SDL_DestroyTexture(skin->barony_flags[i]); skin->barony_flags[i] = NULL;
    }
}

static void province_refresh_history_art(struct menu *menu)
{
    struct province_panel_skin *skin = &menu->province_skin;
    province_clear_history_art(menu);
    if (!menu->selected || !menu->realms.history) return;
    const size_t index = (size_t)(menu->selected - menu->provinces.provinces);
    struct realm_province_ownership ownership;
    if (!realm_province_ownership(&menu->realms, index, &ownership) ||
        !ownership.has_county) return;
    const struct ruler_profile *ruler = realm_ruler(&menu->realms, ownership.county.holder);
    skin->portrait = setup_character_portrait(menu, ruler);
    if (ownership.has_independent) {
        const struct ruler_profile *top = realm_ruler(&menu->realms, ownership.independent.holder);
        skin->independent_flag = era_title_shield(menu->renderer, menu->asset_root,
                ownership.independent.id, top ? top->religion_key : "");
    }
    /* Follow dated de-jure title relationships, not the current independent
     * ruler. CK2 keeps duchy/kingdom hierarchy separate from allegiance. */
    struct realm_title_info t = ownership.county;
    for (unsigned depth=0; depth<8; ++depth) {
        struct realm_title_info parent;
        if (!realm_dejure_parent_info(&menu->realms, t.id, &parent)) break;
        if (parent.id[0] == 'd' && !skin->duchy_flag) {
            skin->duchy_flag = era_title_shield(menu->renderer, menu->asset_root,
                                                parent.id, ruler ? ruler->religion_key : "");
        } else if (parent.id[0] == 'k' && !skin->kingdom_flag) {
            skin->kingdom_flag = era_title_shield(menu->renderer, menu->asset_root,
                                                 parent.id, ruler ? ruler->religion_key : "");
        }
        if (!strcmp(t.id, parent.id)) break;
        t = parent;
    }
    for (size_t i=0; i<menu->holdings.count && i<HOLDING_LIMIT; ++i) {
        struct realm_title_info barony;
        if (!realm_title_info(&menu->realms, menu->holdings.slots[i].id, &barony) ||
            !barony.holder) continue;
        const struct ruler_profile *baron = realm_ruler(&menu->realms, barony.holder);
        struct realm_title_info primary;
        /* CK2's holding owner crest is the holder's primary title arms, not
         * necessarily the barony title. This is why a king's holdings show
         * their kingdom arms repeatedly in the original province screen. */
        const char *arms = realm_primary_title(&menu->realms, barony.holder, &primary)
            ? primary.id : barony.id;
        skin->barony_flags[i] = era_title_shield(menu->renderer, menu->asset_root,
                                  arms, baron ? baron->religion_key : "");
    }
}

enum province_text_alignment { PROVINCE_TEXT_LEFT, PROVINCE_TEXT_CENTER, PROVINCE_TEXT_RIGHT };

/* CK2 province.gui distinguishes left-aligned names/labels from right-aligned
 * values. The old code centered everything, causing Culture/Religion overlap.
 * Bound the glyphs to each original GUI text field and retain the caller's
 * panel clip. This is independent of language/text length. */
static void province_field_text(SDL_Renderer *renderer, const struct menu_assets *assets,
                                const char *text, SDL_Rect field,
                                enum province_text_alignment align)
{
    if (!assets || !assets->font || !text || !*text || field.w <= 0 || field.h <= 0)
        return;
    int text_width = 0;
    for (const unsigned char *c = (const unsigned char *)text; *c; ++c)
        text_width += assets->glyphs[*c].advance;
    const int center = align == PROVINCE_TEXT_LEFT ? field.x + text_width / 2 :
        align == PROVINCE_TEXT_RIGHT ? field.x + field.w - text_width / 2 :
        field.x + field.w / 2;
    const SDL_bool clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_Rect previous, clip = field;
    SDL_RenderGetClipRect(renderer, &previous);
    if (clipped && !SDL_IntersectRect(&previous, &field, &clip))
        return;
    SDL_RenderSetClipRect(renderer, &clip);
    centered_text(renderer, assets, text, strlen(text), center, field.y);
    SDL_RenderSetClipRect(renderer, clipped ? &previous : NULL);
}

static void province_draw_header(SDL_Renderer *renderer, const struct menu_assets *assets,
                                 const struct menu *menu)
{
    const struct province_panel_skin *skin = &menu->province_skin;
    if (!menu->selected) return;
    const SDL_bool clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_Rect previous;
    SDL_RenderGetClipRect(renderer, &previous);
    SDL_RenderSetClipRect(renderer, &menu->holding_panel);
    if (skin->portrait) {
        SDL_Rect dest = skin->ruler_portrait;
        dest.w = dest.h = 100;
        SDL_RenderCopy(renderer, skin->portrait, NULL, &dest);
    }
    if (skin->portrait_frame.texture) setup_widget_draw(&skin->portrait_frame, renderer, (SDL_Point){-1,-1});
    if (skin->county_crown.texture) setup_widget_draw(&skin->county_crown, renderer, (SDL_Point){-1,-1});
    if (skin->region_info.texture) setup_widget_draw(&skin->region_info, renderer, menu->pointer);
    if (skin->crown_focus.texture) setup_widget_draw(&skin->crown_focus, renderer, menu->pointer);
    if (skin->capital_icon.texture) setup_widget_draw(&skin->capital_icon, renderer, (SDL_Point){-1,-1});
    if (skin->raise_levy.texture) setup_widget_draw(&skin->raise_levy, renderer, menu->pointer);
    if (skin->raise_ships.texture) setup_widget_draw(&skin->raise_ships, renderer, menu->pointer);
    /* Header name and landed hierarchy follow province.gui, not guessed
     * center coordinates from the OpenCK2 development panel. */
    province_field_text(renderer, assets, menu->selected->name, skin->name,
                        PROVINCE_TEXT_CENTER);
    struct menu_assets small = {.font = menu->era.small_font};
    memcpy(small.glyphs, menu->era.small_glyphs, sizeof small.glyphs);
    const struct menu_assets *stat_font = small.font ? &small : assets;
    SDL_SetTextureColorMod(stat_font->font, 43, 34, 25);
    const size_t ix=(size_t)(menu->selected-menu->provinces.provinces);
    struct realm_province_ownership ownership;
    if (realm_province_ownership(&menu->realms, ix, &ownership) && ownership.has_county) {
        struct realm_title_info t = ownership.county;
        for (unsigned depth=0; depth<8; ++depth) {
            struct realm_title_info parent;
            if (!realm_dejure_parent_info(&menu->realms, t.id, &parent)) break;
            if (parent.id[0]=='d') province_field_text(renderer, stat_font, parent.name,
                skin->duchy, PROVINCE_TEXT_LEFT);
            if (parent.id[0]=='k') province_field_text(renderer, stat_font, parent.name,
                skin->kingdom, PROVINCE_TEXT_LEFT);
            if (!strcmp(t.id,parent.id)) break;
            t=parent;
        }
    }
    if (skin->independent_flag) {
        /* province_tier_shield3 at (260,22), authored in province.gui */
        draw_small_title_flag(renderer,skin->independent_flag,
          (SDL_Rect){menu->holding_panel.x+260,menu->holding_panel.y+22,54,63});
    }
    draw_small_title_flag(renderer,skin->duchy_flag,
          (SDL_Rect){skin->duchy_shield.x,skin->duchy_shield.y,30,34});
    draw_small_title_flag(renderer,skin->kingdom_flag,
          (SDL_Rect){skin->kingdom_shield.x,skin->kingdom_shield.y,30,34});
    if (skin->tax_icon.texture) setup_widget_draw(&skin->tax_icon,renderer,(SDL_Point){-1,-1});
    for (int t=0; t<3; ++t)
        if (skin->tech_icons[t].texture)
            setup_widget_draw(&skin->tech_icons[t],renderer,(SDL_Point){-1,-1});
    /* Tax is a known sum of authored base incomes and built upgrades. It is
     * NOT labelled a levy, revolt risk or advanced-CK2 final tax amount. */
    double annual = 0;
    bool known = true;
    for (size_t j=0; j<menu->holdings.count; ++j) {
        double part = 0;
        if (!construction_holding_annual_tax(&menu->construction,&menu->holdings.slots[j],&part)) {
            known=false; break;
        }
        annual += part;
    }
    if (known) {
        char amount[48];
        SDL_snprintf(amount,sizeof amount,"%.1f / yr",annual);
        province_field_text(renderer, stat_font, amount, skin->tax_amount,
                            PROVINCE_TEXT_RIGHT);
    }
    province_field_text(renderer, stat_font, ck2_localize("CULTURE","Culture"),
                        skin->culture_label, PROVINCE_TEXT_LEFT);
    province_field_text(renderer, stat_font, ck2_localize("RELIGION","Religion"),
                        skin->religion_label, PROVINCE_TEXT_LEFT);
    province_field_text(renderer, stat_font, ck2_localize("SUPPLY","Supply"),
                        skin->supply_label, PROVINCE_TEXT_LEFT);
    province_field_text(renderer, stat_font, ck2_localize("PLOTS_REVOLTRISK","Revolt Risk"),
                        skin->revolt_label, PROVINCE_TEXT_LEFT);
    if (menu->holdings.culture_name[0])
        province_field_text(renderer, stat_font, menu->holdings.culture_name,
                            skin->culture, PROVINCE_TEXT_RIGHT);
    if (menu->holdings.religion_name[0])
        province_field_text(renderer, stat_font, menu->holdings.religion_name,
                            skin->religion, PROVINCE_TEXT_RIGHT);
    SDL_SetTextureColorMod(stat_font->font, 255, 255, 255);
    SDL_RenderSetClipRect(renderer,clipped ? &previous : NULL);
}

static void refresh_province_title_shield(struct menu *menu)
{
    SDL_DestroyTexture(menu->province_title_shield);
    menu->province_title_shield = NULL;
    if (!menu->selected || !menu->realms.history) return;
    const size_t index = (size_t)(menu->selected - menu->provinces.provinces);
    struct realm_province_ownership owners;
    if (!realm_province_ownership(&menu->realms, index, &owners) || !owners.has_county)
        return;
    const struct ruler_profile *ruler = realm_ruler(&menu->realms, owners.county.holder);
    menu->province_title_shield = era_title_shield(menu->renderer, menu->asset_root,
         owners.county.id, ruler ? ruler->religion_key : "");
}

/* Conversion effects live in campaign memory, never in CK2 history files.
 * Keep selected province, map-mode facts and settlement graphical culture in sync. */
static void conversion_replay_province(struct menu *menu, unsigned province_id)
{
    const struct province *p = province_by_id(&menu->provinces, province_id);
    if (!p || !menu->settlement_scene.province_facts) return;
    const size_t index = (size_t)(p - menu->provinces.provinces);
    if (index >= menu->settlement_scene.province_fact_count) return;
    struct map_province_facts *facts = &menu->settlement_scene.province_facts[index];
    const char *culture = conversion_province_value(&menu->conversion, province_id,
                                                     CONVERSION_CULTURE, facts->culture);
    const char *faith = conversion_province_value(&menu->conversion, province_id,
                                                  CONVERSION_RELIGION, facts->religion);
    if (culture != facts->culture) SDL_strlcpy(facts->culture, culture, sizeof facts->culture);
    if (faith != facts->religion) SDL_strlcpy(facts->religion, faith, sizeof facts->religion);
    for (size_t i = 0; i < menu->settlement_scene.count; ++i) {
        struct map_settlement_marker *m = &menu->settlement_scene.markers[i];
        if (m->province_id != province_id) continue;
        SDL_strlcpy(m->culture, facts->culture, sizeof m->culture);
        SDL_strlcpy(m->religion, facts->religion, sizeof m->religion);
        const struct settlement_culture *style = settlement_culture_find(&menu->settlement_scene.styles, m->culture);
        if (style) {
            SDL_strlcpy(m->graphical, style->graphics, sizeof m->graphical);
            m->rgb = style->rgb;
            m->culture_color_known = style->has_color;
        }
    }
}

static void conversion_replay_rulers(struct menu *menu)
{
    for (size_t i = 0; i < menu->conversion.person_count; ++i) {
        const struct conversion_person *change = menu->conversion.people + i;
        struct ruler_profile *person = realm_ruler_mutable(&menu->realms, change->id);
        if (!person) continue;
        if (change->has_culture) {
            SDL_strlcpy(person->culture_key, change->culture, sizeof person->culture_key);
            SDL_strlcpy(person->culture, ck2_localize(change->culture, change->culture), sizeof person->culture);
        }
        if (change->has_religion) {
            SDL_strlcpy(person->religion_key, change->religion, sizeof person->religion_key);
            SDL_strlcpy(person->religion, ck2_localize(change->religion, change->religion), sizeof person->religion);
        }
    }
}

static void conversion_catalog_load_files(struct menu *menu)
{
    conversion_catalog_free(&menu->conversion_catalog);
    for (int kind = CONVERSION_RELIGION; kind <= CONVERSION_CULTURE; ++kind) {
        const char *directory = kind == CONVERSION_RELIGION ? "common/religions" : "common/cultures";
        struct ck2_asset_files files = ck2_list_assets(menu->asset_root, directory);
        for (size_t i = 0; i < files.count; ++i) {
            const char *name = files.names[i];
            const size_t n = strlen(name);
            if (n < 4 || strcmp(name + n - 4, ".txt")) continue;
            char *data = load_from_directory(menu->asset_root, directory, name);
            if (!data) continue;
            if (!conversion_catalog_parse(&menu->conversion_catalog, kind, data))
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Conversion keys could not parse %s/%s", directory, name);
            SDL_free(data);
        }
        ck2_free_asset_files(&files);
    }
    struct ck2_asset_files job_files = ck2_list_assets(menu->asset_root, "common/job_actions");
    for (size_t i = 0; i < job_files.count; ++i) {
        const char *name = job_files.names[i];
        const size_t n = strlen(name);
        if (n < 4 || strcmp(name + n - 4, ".txt")) continue;
        char *jobs = load_from_directory(menu->asset_root, "common/job_actions", name);
        if (!jobs) continue;
        if (!conversion_catalog_jobs(&menu->conversion_catalog, jobs))
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Conversion council actions could not parse %s", name);
        SDL_free(jobs);
    }
    ck2_free_asset_files(&job_files);
    SDL_Log("Conversion definitions: %zu faiths, %zu cultures; Inquisition %d, Settle Tribe %d",
        menu->conversion_catalog.names[CONVERSION_RELIGION].count,
        menu->conversion_catalog.names[CONVERSION_CULTURE].count,
        menu->conversion_catalog.inquisition_defined, menu->conversion_catalog.settle_tribe_defined);
}

static void conversion_selected_labels(struct menu *menu)
{
    if (!menu->selected) return;
    const unsigned id = menu->selected->id;
    const char *culture = conversion_province_value(&menu->conversion, id, CONVERSION_CULTURE,
                                                     menu->holdings.culture_key);
    const char *religion = conversion_province_value(&menu->conversion, id, CONVERSION_RELIGION,
                                                      menu->holdings.religion_key);
    char c[64], r[64];
    SDL_strlcpy(c, culture, sizeof c);
    SDL_strlcpy(r, religion, sizeof r);
    SDL_strlcpy(menu->holdings.culture_key, c, sizeof menu->holdings.culture_key);
    SDL_strlcpy(menu->holdings.religion_key, r, sizeof menu->holdings.religion_key);
    SDL_strlcpy(menu->holdings.culture_name, ck2_localize(c, c), sizeof menu->holdings.culture_name);
    SDL_strlcpy(menu->holdings.religion_name, ck2_localize(r, r), sizeof menu->holdings.religion_name);
}

static void conversion_map_refresh(struct menu *menu)
{
    map_theme_free(&menu->map_theme);
    const enum map_mode_id mode = menu->map_modes.selected;
    if (mode != MAP_MODE_REALMS && mode != MAP_MODE_TERRAIN)
        (void)map_theme_build(&menu->map_theme, menu->renderer, &menu->campaign_map,
                              &menu->provinces, &menu->realms, &menu->settlement_scene,
                              &menu->construction, menu->asset_root, menu->start_date, mode);
}

static void select_province(struct menu *menu, const struct province *province)
{
    if (menu->selected == province)
        return;
    if (menu->province_overlay)
        SDL_DestroyTexture(menu->province_overlay);
    menu->province_overlay = NULL;
    menu->building_open = false;
    menu->selected = province;
    refresh_province_title_shield(menu);
    const unsigned id = province ? province->id : 0;
    if (menu->holding_province != id) {
        if (!province) {
            menu->holdings = (struct county_holdings){0};
            menu->holding_province = 0;
            province_clear_history_art(menu);
            return;
        }
        holdings_load(&menu->holdings, menu->asset_root, province, menu->start_date);
        conversion_selected_labels(menu);
        if (menu->settlement_scene.count) refresh_holding_style(menu);
        char *history = province_history_read(province, menu->asset_root);
        construction_history(&menu->construction, history, menu->start_date);
        read_wonder(&menu->wonder, history, menu->start_date);
        SDL_free(history);
        construction_settlement_apply(&menu->construction, id, &menu->holdings);
        menu->holding_province = id;
    }
    province_refresh_history_art(menu);
    menu->province_overlay = province ? province_highlight(&menu->provinces, menu->renderer,
                                                            province, &menu->campaign_map) : NULL;
}

static bool load_campaign_map(struct menu *menu, SDL_Window *window)
{
    if (!menu->provinces.pixels) {
        if (province_map_load(&menu->provinces, menu->asset_root) < 0) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Single Player", SDL_GetError(), window);
            province_map_free(&menu->provinces);
            return false;
        }
        if (map_view_build(&menu->campaign_map, menu->renderer, menu->asset_root,
                           WIDTH, HEIGHT, true) < 0 ||
            menu->campaign_map.world_width != menu->provinces.pixels->w ||
            menu->campaign_map.world_height != menu->provinces.pixels->h) {
            const char *message = menu->campaign_map.texture
                ? "Province data and terrain have different dimensions." : SDL_GetError();
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Campaign map", message, window);
            map_view_free(&menu->campaign_map);
            province_map_free(&menu->provinces);
            return false;
        }
    }
    return true;
}

static bool load_realms(struct menu *menu, SDL_Window *window, struct campaign_date date)
{
    if (!load_campaign_map(menu, window))
        return false;
    if (realm_map_build(&menu->realms, menu->renderer, menu->asset_root,
                        &menu->provinces, &menu->campaign_map, date) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Realms map", SDL_GetError(), window);
        return false;
    }
    if (menu->campaign_active) conversion_replay_rulers(menu);
    return true;
}

/* CK2 defines Roman imperial government through a title/law potential,
 * not through castle type. Evaluate the documented bootstrap path only for
 * holders of titles named by the installed government script. */
static void government_assign_bookmark(struct menu *menu)
{
    const struct government_definition *roman=government_find(
        &menu->settlement_scene.governments,"roman_imperial_government");
    if (!roman || !roman->has_potential) return;
    char candidates[12][80]={{0}};
    const size_t n=government_potential_titles(roman,candidates,SDL_arraysize(candidates));
    for (size_t i=0;i<n && i<SDL_arraysize(candidates);++i) {
        struct realm_title_info title,primary;
        if (!realm_title_info(&menu->realms,candidates[i],&title) || !title.holder ||
            !realm_primary_title(&menu->realms,title.holder,&primary) ||
            strcmp(primary.id,title.id)) continue;
        struct ruler_profile *holder=realm_ruler_mutable(&menu->realms,title.holder);
        if (!holder || (holder->government_key[0] && !holder->government_inferred)) continue;
        /* The title's dated succession law is read separately from the
         * player's active-law state; other rulers do not inherit player laws. */
        char path[160];
        SDL_snprintf(path,sizeof path,"history/titles/%s.txt",title.id);
        char *history=load_data(menu->asset_root,path);
        struct ck2_law_state laws={0};
        const char *succession=NULL;
        if (history) {
            const unsigned cutoff=(unsigned)(menu->start_date.year*10000+
                menu->start_date.month*100+menu->start_date.day);
            if (ck2_law_title_history(&laws,&menu->law_catalog,history,cutoff)) {
                const struct ck2_law *law=ck2_law_selected(&laws,&menu->law_catalog,"succession");
                if (law) succession=law->key;
            }
        }
        struct government_context ctx={.primary_title=primary.id,
            .succession_law=succession,.game_started=false,.save_game=false,.is_patrician=false};
        bool inferred=false;
        const struct government_definition *result=government_resolve_special(
            &menu->settlement_scene.governments,
            holder->government_inferred ? NULL : holder->government_key,&ctx,&inferred);
        if (result && inferred) {
            SDL_strlcpy(holder->government_key,result->id,sizeof holder->government_key);
            holder->government_inferred=true;
            SDL_Log("CK2 imperial government inferred for %u (%s) from original potential",
                    holder->id,primary.id);
        }
        ck2_law_state_free(&laws);
        SDL_free(history);
    }
}

/* Capital/holding government assignment is provisional: the original
 * CK2 potential scripts also examine religion, holy orders, patricians,
 * gender, and trade/coast flags. We cannot assert those have all passed.
 * In particular, a city alone NEVER establishes a merchant republic. */
static int compare_government_holders(const void *a, const void *b)
{
    const struct government_holding_evidence *aa=a, *bb=b;
    return (aa->holder_id > bb->holder_id) - (aa->holder_id < bb->holder_id);
}
static void government_assign_holdings_bookmark(struct menu *menu)
{
    struct map_settlement_scene *scene=&menu->settlement_scene;
    struct government_holding_evidence *e=scene->holding_evidence;
    if (!e || !scene->holding_evidence_count) return;
    qsort(e,scene->holding_evidence_count,sizeof *e,compare_government_holders);
    size_t assigned=0;
    for (size_t i=0;i<scene->holding_evidence_count;) {
        size_t j=i+1;
        while (j<scene->holding_evidence_count && e[j].holder_id==e[i].holder_id) ++j;
        struct ruler_profile *r=realm_ruler_mutable(&menu->realms,e[i].holder_id);
        if (r && !r->government_key[0]) {
            struct realm_title_info primary;
            if (realm_primary_title(&menu->realms,r->id,&primary)) {
                const uint32_t bit=government_verified_holding(e+i,j-i,r->id,
                                                                  primary.id,primary.capital);
                const struct government_definition *type=government_provisional_holding(
                    &scene->governments,bit);
                if (type && bit==GOVERNMENT_TEMPLE) {
                    const struct government_religion *faith=government_religion_find(
                        &scene->religions,r->religion_key);
                    /* Theocracy potential excludes Muslims and forbids a
                     * temple-holder gender unless its religion permits it. */
                    if (!faith || !strcmp(faith->group,"muslim") ||
                        (r->female ? !faith->female_temple : !faith->male_temple))
                        type=NULL;
                }
                if (type) {
                    SDL_strlcpy(r->government_key,type->id,sizeof r->government_key);
                    r->government_inferred=true;
                    ++assigned;
                }
            }
        }
        i=j;
    }
    SDL_Log("Government holding evidence: %zu verified baronies; %zu provisional nomad/republic/theocracy rulers",
            scene->holding_evidence_count,assigned);
}

static uint32_t government_runtime_capital_holding(const struct menu *menu,
                                                   unsigned holder,
                                                   const struct realm_title_info *primary)
{
    /* After succession, dated barony holders can have changed. Re-read
     * owners from the runtime title index rather than trusting bookmark IDs. */
    const struct map_settlement_scene *scene=&menu->settlement_scene;
    if (!scene->holding_evidence_count) return 0;
    struct government_holding_evidence *current=calloc(scene->holding_evidence_count,sizeof *current);
    if (!current) return 0;
    size_t n=0;
    for (size_t i=0;i<scene->holding_evidence_count;++i) {
        const struct government_holding_evidence *f=scene->holding_evidence+i;
        struct realm_title_info t;
        if (realm_title_info(&menu->realms,f->title_id,&t) && t.holder==holder) {
            current[n]=*f;current[n++].holder_id=holder;
        }
    }
    const uint32_t bit=government_verified_holding(current,n,holder,
                                                    primary->id,primary->capital);
    free(current);
    return bit;
}

/* Re-evaluate the playable ruler when a developer changes succession law or
 * after a title transfer. A missing current law is unresolved, not false. */
static bool government_refresh_player(struct menu *menu)
{
    struct ruler_profile *ruler=realm_ruler_mutable(&menu->realms,menu->campaign_ruler_id);
    if (!ruler || (ruler->government_key[0] && !ruler->government_inferred)) return false;
    struct realm_title_info primary;
    if (!realm_primary_title(&menu->realms,ruler->id,&primary)) return false;
    char before[sizeof ruler->government_key];
    SDL_strlcpy(before,ruler->government_key,sizeof before);
    const struct ck2_law *law=ck2_law_selected(&menu->active_laws,&menu->law_catalog,"succession");
    struct government_context ctx={.primary_title=primary.id,
        .succession_law=law ? law->key : NULL,
        .game_started=true,.is_patrician=false};
    const struct government_definition *roman=government_find(
        &menu->settlement_scene.governments,"roman_imperial_government");
    const enum government_truth status=government_evaluate_potential(roman,&ctx);
    if (status==GOVERNMENT_TRUE && roman) {
        SDL_strlcpy(ruler->government_key,roman->id,sizeof ruler->government_key);
        ruler->government_inferred=true;
    } else if (status!=GOVERNMENT_UNRESOLVED) {
        /* A former imperial holder must not retain the imperial government
         * after losing the title/law. Reclassify from *currently* held baronies. */
        ruler->government_key[0]=0;
        ruler->government_inferred=false;
        const uint32_t bit=government_runtime_capital_holding(menu,ruler->id,&primary);
        const struct government_definition *type=government_provisional_holding(
            &menu->settlement_scene.governments,bit);
        if (type && bit==GOVERNMENT_TEMPLE) {
            const struct government_religion *faith=government_religion_find(
                &menu->settlement_scene.religions,ruler->religion_key);
            if (!faith || !strcmp(faith->group,"muslim") ||
                (ruler->female ? !faith->female_temple : !faith->male_temple))
                type=NULL;
        }
        if (type) {
            SDL_strlcpy(ruler->government_key,type->id,sizeof ruler->government_key);
            ruler->government_inferred=true;
        }
    }
    return strcmp(before,ruler->government_key)!=0;
}

static SDL_Rect lobby_row(const struct menu *menu, enum lobby_element list, int index)
{
    const enum lobby_element template = list == LOBBY_BOOKMARKS ? LOBBY_BOOKMARK_ROW : LOBBY_CHARACTER_ROW;
    const SDL_Rect box = menu->toolbar.lobby[list].area, row = menu->toolbar.lobby[template].area;
    return (SDL_Rect){box.x + row.x, box.y + row.y + row.h * index, box.w, row.h};
}

static int lobby_rows(const struct menu *menu, enum lobby_element list)
{
    const SDL_Rect row = lobby_row(menu, list, 0);
    return row.h > 0 ? menu->toolbar.lobby[list].area.h / row.h : 0;
}

static const struct era_ruler *featured_ruler(const struct starting_era *era, unsigned id)
{
    for (int i = 0; i < era->count; ++i)
        if (era->rulers[i].id == id)
            return &era->rulers[i];
    return NULL;
}

static const struct ruler_profile *setup_profile(const struct menu *menu, const char **title,
                                                unsigned *capital)
{
    const struct starting_era *era = &menu->era.eras[menu->era.selected];
    if (menu->setup_title[0]) {
        struct realm_title_info info;
        if (!realm_title_info(&menu->realms, menu->setup_title, &info))
            return NULL;
        const char *name = realm_title_name(&menu->realms, menu->setup_title);
        *title = name ? name : menu->setup_title;
        *capital = info.capital;
        return realm_ruler(&menu->realms, info.holder);
    }
    if (menu->era.ruler < 0)
        return NULL;
    const struct era_ruler *ruler = &era->rulers[menu->era.ruler];
    const char *name = realm_title_name(&menu->realms, ruler->title);
    *title = name && *name ? name : ruler->title_name;
    return realm_ruler(&menu->realms, ruler->id);
}

static bool inspected_title(const struct menu *menu, struct realm_title_info *title)
{
    const struct starting_era *era = &menu->era.eras[menu->era.selected];
    const char *id = menu->setup_title[0] ? menu->setup_title
        : menu->era.ruler >= 0 ? era->rulers[menu->era.ruler].title : "";
    return *id && realm_title_info(&menu->realms, id, title);
}

static void setup_label(SDL_Renderer *renderer, const struct menu_assets *assets, const char *text,
                        SDL_Rect row, int offset)
{
    const SDL_Rect clip = {row.x + offset, row.y, row.w - offset - 3, row.h};
    SDL_RenderSetClipRect(renderer, &clip);
    campaign_text(renderer, assets, text, clip.x + clip.w / 2, row.y + (row.h - 22) / 2);
    SDL_RenderSetClipRect(renderer, NULL);
}

static void setup_description(SDL_Renderer *renderer, const struct menu_assets *assets,
                              const char *text, SDL_Rect box)
{
    SDL_RenderSetClipRect(renderer, &box);
    for (int y = box.y; *text && y + 18 <= box.y + box.h; y += 18) {
        while (*text == ' ')
            ++text;
        const char *end = text, *space = NULL;
        int width = 0, before_space = 0;
        while (*end && *end != '\n') {
            const int advance = assets->glyphs[(unsigned char)*end].advance;
            if (width + advance > box.w && end != text)
                break;
            if (*end == ' ') {
                space = end;
                before_space = width;
            }
            width += advance;
            ++end;
        }
        if (*end && *end != '\n' && space && space > text) {
            end = space;
            width = before_space;
        }
        centered_text(renderer, assets, text, (size_t)(end - text), box.x + width / 2, y);
        text = *end == '\n' ? end + 1 : end;
    }
    SDL_RenderSetClipRect(renderer, NULL);
}

static void setup_left_label(SDL_Renderer *renderer, const struct menu_assets *assets,
                             const char *text, SDL_Rect area)
{
    int width = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p)
        width += assets->glyphs[*p].advance;
    SDL_RenderSetClipRect(renderer, &area);
    centered_text(renderer, assets, text, strlen(text), area.x + width / 2,
                  area.y + SDL_max(0, (area.h - 18) / 2));
    SDL_RenderSetClipRect(renderer, NULL);
}

static int rank_frame(char rank)
{
    return rank == 'e' ? 5 : rank == 'k' ? 4 : rank == 'd' ? 3 : rank == 'c' ? 2 : 1;
}

static void draw_widget_frame(SDL_Renderer *renderer, const struct setup_widget *widget, int frame)
{
    const SDL_Rect source = {SDL_clamp(frame, 0, widget->frames - 1) * widget->area.w,
                             0, widget->area.w, widget->area.h};
    SDL_RenderCopy(renderer, widget->texture, &source, &widget->area);
}

static void setup_scrollbar(const struct menu *menu, enum lobby_element element, int offset,
                            int maximum, struct setup_widget parts[SETUP_SCROLL_PARTS])
{
    memcpy(parts, menu->toolbar.scroll, sizeof menu->toolbar.scroll);
    const SDL_Rect list = menu->toolbar.lobby[element].area;
    struct setup_widget *up = &parts[SETUP_SCROLL_UP], *down = &parts[SETUP_SCROLL_DOWN];
    struct setup_widget *track = &parts[SETUP_SCROLL_TRACK], *thumb = &parts[SETUP_SCROLL_THUMB];
    for (int i = 0; i < SETUP_SCROLL_PARTS; ++i)
        parts[i].area.x = list.x + list.w - (element == LOBBY_VASSALS ? parts[i].area.w : 0);
    up->area.y = list.y;
    down->area.y = list.y + list.h - down->area.h;
    track->area.y = up->area.y + up->area.h;
    track->area.h = SDL_max(0, down->area.y - track->area.y);
    thumb->area.y = track->area.y + SDL_max(0, track->area.h - thumb->area.h) *
        offset / SDL_max(1, maximum);
}

static void draw_setup_scrollbar(const struct menu *menu, SDL_Renderer *renderer,
                                  enum lobby_element list, int offset, int maximum)
{
    if (maximum <= 0)
        return;
    struct setup_widget parts[SETUP_SCROLL_PARTS];
    setup_scrollbar(menu, list, offset, maximum, parts);
    const struct setup_widget *track = &menu->toolbar.scroll[SETUP_SCROLL_TRACK];
    const SDL_Rect source = {0, 0, track->area.w, track->area.h};
    SDL_RenderCopy(renderer, track->texture, &source, &parts[SETUP_SCROLL_TRACK].area);
    setup_widget_draw(&parts[SETUP_SCROLL_UP], renderer, menu->pointer);
    setup_widget_draw(&parts[SETUP_SCROLL_DOWN], renderer, menu->pointer);
    setup_widget_draw(&parts[SETUP_SCROLL_THUMB], renderer, menu->pointer);
}

static void draw_setup(SDL_Renderer *renderer, const struct menu_assets *assets, const struct menu *menu)
{
    struct menu_assets small = {.font = menu->era.small_font};
    memcpy(small.glyphs, menu->era.small_glyphs, sizeof small.glyphs);
    char date[48], year[16];
    const struct starting_era *era = &menu->era.eras[menu->era.selected];
    const struct setup_widget *ui = menu->toolbar.lobby;
    setup_toolbar_draw(&menu->toolbar, renderer, menu->pointer);
    for (int i = LOBBY_YEAR_UP1; i <= LOBBY_DAY_DOWN; ++i)
        setup_widget_draw(&ui[i], renderer, menu->pointer);
    format_campaign_date(date, sizeof date, era->date);
    char *last = strrchr(date, ' ');
    if (last)
        *last = '\0';
    SDL_snprintf(year, sizeof year, "%d", era->date.year);
    SDL_SetTextureColorMod(small.font, 39, 26, 15);
    campaign_text(renderer, &small, date, ui[LOBBY_DAYMONTH].area.x + ui[LOBBY_DAYMONTH].area.w / 2,
                  ui[LOBBY_DAYMONTH].area.y);
    campaign_text(renderer, &small, year, ui[LOBBY_YEAR].area.x + ui[LOBBY_YEAR].area.w / 2,
                  ui[LOBBY_YEAR].area.y);
    SDL_SetTextureColorMod(small.font, 255, 255, 255);
    setup_label(renderer, &small, menu->toolbar.prompt, ui[LOBBY_PROMPT].area, 0);

    setup_widget_draw(&ui[LOBBY_BOTTOM], renderer, menu->pointer);
    setup_description(renderer, &small, era->description, ui[LOBBY_DESCRIPTION].area);
    const SDL_Rect expansions = ui[LOBBY_EXPANSIONS].area;
    const int step = ui[LOBBY_EXPANSION_ROW].area.w + menu->toolbar.expansion_spacing;
    for (int i = 0; i < CK2_DLC_ICON_COUNT && step > 0 && (i + 1) * step <= expansions.w; ++i) {
        if (!assets->dlc_icons[i])
            continue;
        bool enabled = false;
        for (size_t j = 0; j < menu->catalog.dlc.count; ++j) {
            const struct content_entry *entry = &menu->catalog.dlc.entries[j];
            if (entry->icon == i && entry->installed && entry->enabled) {
                enabled = true;
                break;
            }
        }
        const SDL_Rect source = {enabled ? 0 : 64, 0, 32, 32};
        const SDL_Rect target = {expansions.x + step * i, expansions.y, 32, 32};
        SDL_RenderCopy(renderer, assets->dlc_icons[i], &source, &target);
    }

    setup_widget_draw(&ui[LOBBY_LEFT], renderer, menu->pointer);
    setup_widget_draw(&ui[LOBBY_CHARACTERS], renderer, menu->pointer);
    setup_widget_draw(&ui[LOBBY_LOAD], renderer, menu->pointer);
    setup_label(renderer, &small, menu->toolbar.load_label, ui[LOBBY_LOAD].area, 0);
    setup_label(renderer, &small, menu->toolbar.bookmarks_label, ui[LOBBY_BOOKMARK_LABEL].area, 0);
    setup_label(renderer, &small, menu->toolbar.featured_label, ui[LOBBY_FEATURED_LABEL].area, 0);
    SDL_SetTextureColorMod(small.font, 39, 26, 15);
    const int bookmarks = lobby_rows(menu, LOBBY_BOOKMARKS);
    for (int slot = 0; slot < bookmarks && slot + menu->bookmark_scroll < menu->era.count; ++slot) {
        const int i = slot + menu->bookmark_scroll;
        const SDL_Rect row = lobby_row(menu, LOBBY_BOOKMARKS, slot);
        struct setup_widget art = ui[LOBBY_BOOKMARK_ART];
        art.area.x += row.x;
        art.area.y += row.y;
        if (art.frames > 1 && i == menu->era.selected)
            SDL_SetTextureColorMod(art.texture, 225, 225, 225);
        setup_widget_draw(&art, renderer, menu->pointer);
        SDL_SetTextureColorMod(art.texture, 255, 255, 255);
    }
    for (int slot = 0; slot < bookmarks && slot + menu->bookmark_scroll < menu->era.count; ++slot) {
        const int i = slot + menu->bookmark_scroll;
        const SDL_Rect row = lobby_row(menu, LOBBY_BOOKMARKS, slot);
        char label[48];
        format_campaign_date(label, sizeof label, menu->era.eras[i].date);
        SDL_Rect name_box = ui[LOBBY_BOOKMARK_TITLE].area;
        SDL_Rect date_box = ui[LOBBY_BOOKMARK_DATE].area;
        name_box.x += row.x;
        name_box.y += row.y;
        date_box.x += row.x;
        date_box.y += row.y;
        setup_description(renderer, &small, menu->era.eras[i].name, name_box);
        setup_description(renderer, &small, label, date_box);
    }
    draw_setup_scrollbar(menu, renderer, LOBBY_BOOKMARKS, menu->bookmark_scroll,
                         menu->era.count - bookmarks);
    const char *title = "";
    unsigned capital = 0;
    const struct ruler_profile *profile = setup_profile(menu, &title, &capital);
    for (int slot = 0; slot < lobby_rows(menu, LOBBY_FEATURED) && slot + menu->featured_scroll < era->count; ++slot) {
        const struct era_ruler *ruler = &era->rulers[slot + menu->featured_scroll];
        const SDL_Rect row = lobby_row(menu, LOBBY_FEATURED, slot);
        if (profile && profile->id == ruler->id)
            SDL_RenderCopy(renderer, assets->lobby_row, NULL, &row);
        era_draw_portrait(&menu->era, renderer, ruler->portrait, ruler->title[0],
                          (SDL_Rect){row.x + 2, row.y + 2, 50, 50});
        setup_label(renderer, &small, ruler->name, row, 54);
    }
    draw_setup_scrollbar(menu, renderer, LOBBY_FEATURED, menu->featured_scroll,
                         era->count - lobby_rows(menu, LOBBY_FEATURED));
    SDL_SetTextureColorMod(small.font, 255, 255, 255);

    const struct era_ruler *ruler = profile ? featured_ruler(era, profile->id)
        : !menu->setup_title[0] && menu->era.ruler >= 0 ? &era->rulers[menu->era.ruler] : NULL;
    if (!profile && !ruler && !menu->setup_title[0])
        return;
    struct realm_title_info info = {0}, liege = {0};
    inspected_title(menu, &info);
    setup_widget_draw(&ui[LOBBY_RIGHT], renderer, menu->pointer);
    SDL_SetTextureColorMod(small.font, 39, 26, 15);
    setup_label(renderer, &small, ruler ? ruler->name : profile && *profile->name ? profile->name :
                profile ? info.name : "No recorded holder", ui[LOBBY_RULER].area, 0);
    SDL_Texture *portrait = menu->setup_portrait ? menu->setup_portrait : ruler ? ruler->portrait : NULL;
    const int frame = rank_frame(*info.id ? info.id[0] : 'c');
    if (portrait) {
        SDL_RenderCopy(renderer, portrait, NULL, &ui[LOBBY_PORTRAIT].area);
        draw_widget_frame(renderer, &ui[LOBBY_PORTRAIT_FRAME], frame);
    }
    SDL_Texture *shield = menu->setup_shield ? menu->setup_shield : ruler ? ruler->shield : NULL;
    if (shield) {
        SDL_Rect arms = ui[LOBBY_SHIELD].area;
        SDL_QueryTexture(shield, NULL, NULL, &arms.w, &arms.h);
        SDL_RenderCopy(renderer, shield, NULL, &arms);
        draw_widget_frame(renderer, &ui[LOBBY_CROWN], frame);
    }
    if (profile && profile->dynasty[0])
        setup_left_label(renderer, &small, profile->dynasty, ui[LOBBY_DYNASTY].area);
    struct menu_assets details = {.font = menu->toolbar.detail_font};
    memcpy(details.glyphs, menu->toolbar.detail_glyphs, sizeof details.glyphs);
    SDL_SetTextureColorMod(details.font, 0, 0, 0);
    if (!title[0])
        title = *info.name ? info.name : ruler ? ruler->title_name : "";
    setup_left_label(renderer, &details, title, ui[LOBBY_TITLE].area);
    setup_left_label(renderer, &details, profile && *profile->culture ? profile->culture : "-",
                ui[LOBBY_CULTURE].area);
    setup_left_label(renderer, &details, profile && *profile->religion ? profile->religion : "-",
                ui[LOBBY_RELIGION].area);
    char line[192];
    if (realm_title_info(&menu->realms, info.liege, &liege) && liege.holder) {
        SDL_snprintf(line, sizeof line, "Liege: %s", liege.name);
        setup_left_label(renderer, &details, line, ui[LOBBY_STATUS].area);
    } else if (info.holder) {
        setup_left_label(renderer, &details, "Independent", ui[LOBBY_STATUS].area);
    }
    SDL_RenderCopy(renderer, ui[LOBBY_DIFFICULTY_TRACK].texture, NULL, &ui[LOBBY_DIFFICULTY_TRACK].area);
    setup_widget_draw(&ui[LOBBY_DIFFICULTY], renderer, menu->pointer);
    setup_label(renderer, &small, menu->toolbar.difficulty_label, ui[LOBBY_DIFFICULTY_LABEL].area, 0);
    setup_label(renderer, &small, menu->toolbar.diplomacy_label, ui[LOBBY_DIPLOMACY_LABEL].area, 0);
    struct realm_title_info vassals[VASSAL_DISPLAY_LIMIT];
    const int rows = SDL_min(lobby_rows(menu, LOBBY_VASSALS), (int)SDL_arraysize(vassals));
    const size_t total = realm_vassal_titles(&menu->realms, &info, menu->vassal_scroll, vassals, rows);
    for (int i = 0; i < rows && menu->vassal_scroll + (size_t)i < total; ++i) {
        const struct realm_title_info vassal_title = vassals[i];
        const SDL_Rect row = lobby_row(menu, LOBBY_VASSALS, i);
        const struct ruler_profile *vassal = realm_ruler(&menu->realms, vassal_title.holder);
        if (SDL_PointInRect(&menu->pointer, &row))
            SDL_RenderCopy(renderer, assets->lobby_row, NULL, &row);
        const struct vassal_art *art = &menu->vassal_art[i];
        const int rank = rank_frame(vassal_title.id[0]);
        SDL_Rect face = ui[LOBBY_ROW_PORTRAIT].area;
        face.x += row.x;
        face.y += row.y;
        if (art->portrait) {
            SDL_RenderCopy(renderer, art->portrait, NULL, &face);
            struct setup_widget frame = ui[LOBBY_ROW_FRAME];
            frame.area.x += row.x;
            frame.area.y += row.y;
            draw_widget_frame(renderer, &frame, rank);
        }
        if (art->shield) {
            SDL_Rect arms = ui[LOBBY_ROW_SHIELD].area;
            arms.x += row.x;
            arms.y += row.y;
            arms.w = arms.h = 22;
            SDL_RenderCopy(renderer, art->shield, NULL, &arms);
        }
        SDL_Rect name = ui[LOBBY_ROW_NAME].area;
        name.x += row.x;
        name.y += row.y;
        SDL_snprintf(line, sizeof line, "%s\n%s", vassal && *vassal->name ? vassal->name : vassal_title.name,
                     vassal_title.name);
        setup_description(renderer, &small, line, name);
    }
    draw_setup_scrollbar(menu, renderer, LOBBY_VASSALS, (int)menu->vassal_scroll, (int)total - rows);
    SDL_SetTextureColorMod(small.font, 255, 255, 255);
    setup_label(renderer, &small, menu->toolbar.vassals_label, ui[LOBBY_VASSALS_LABEL].area, 0);
    SDL_snprintf(line, sizeof line, "%zu", total);
    SDL_Rect amount = ui[LOBBY_VASSALS_AMOUNT].area;
    amount.x += amount.w * 3 / 4;
    amount.w /= 4;
    setup_label(renderer, &small, line, amount, 0);


}

static void setup_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets, const struct menu *menu)
{
    const SDL_Point point = menu->pointer;
    if (point.y < 110 || point.y > 824 || SDL_PointInRect(&point, &menu->toolbar.lobby[LOBBY_BOTTOM].area) || SDL_PointInRect(&point, &menu->toolbar.lobby[LOBBY_LEFT].area) ||
        SDL_PointInRect(&point, &menu->toolbar.lobby[LOBBY_RIGHT].area))
        return;
    SDL_Point world;
    if (!map_view_pick(&menu->campaign_map, point.x, point.y, &world))
        return;
    const struct province *province = province_at(&menu->provinces, world.x, world.y);
    if (!province)
        return;
    const size_t index = (size_t)(province - menu->provinces.provinces);
    struct realm_title_info county, realm;
    if (!realm_county_info(&menu->realms, index, false, &county))
        return;
    const bool has_realm = realm_county_info(&menu->realms, index, true, &realm);
    const struct ruler_profile *ruler = realm_ruler(&menu->realms, county.holder);
    const SDL_Rect box = {SDL_clamp(point.x + 16, 4, WIDTH - 344),
                          SDL_clamp(point.y + 18, 4, HEIGHT - 94), 340, 90};
    SDL_RenderCopy(renderer, assets->lobby_tooltip, NULL, &box);
    char line[230];
    snprintf(line, sizeof line, "%s%s%s", province->name, ruler && *ruler->name ? " - " : "",
             ruler ? ruler->name : "");
    struct menu_assets small = {.font = menu->era.small_font};
    memcpy(small.glyphs, menu->era.small_glyphs, sizeof small.glyphs);
    setup_label(renderer, &small, line, (SDL_Rect){box.x + 7, box.y + 6, 326, 23}, 0);
    if (has_realm) {
        SDL_SetTextureColorMod(small.font, 231, 203, 100);
        setup_label(renderer, &small, realm.name, (SDL_Rect){box.x + 7, box.y + 29, 326, 23}, 0);
        SDL_SetTextureColorMod(small.font, 255, 255, 255);
    }
    setup_label(renderer, &small, "Click: county   Alt: primary title   Shift: realm",
                (SDL_Rect){box.x + 7, box.y + 57, 326, 23}, 0);
}

static void open_campaign(struct menu *menu, SDL_Window *window, bool new_game)
{
    if (!load_campaign_map(menu, window))
        return;
    if (!menu->holding_layout_ready && load_holding_layout(menu, menu->assets->hud_province) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Province holdings", SDL_GetError(), window);
        return;
    }
    if (!menu->construction.count && construction_load(&menu->construction, menu->asset_root) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Construction", SDL_GetError(), window);
        return;
    }
    if (!menu->building_bg && load_building_layout(menu) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Construction", SDL_GetError(), window);
        return;
    }
    if (new_game) {
        menu->technology.open = false;
        menu->console.open = false;
        menu->event_open = false;
        menu->event_target_id = 0;
        menu->bloodlines.open = false;
        menu->wonder.open = false;
        free(menu->wonder.locations);
        menu->wonder.locations = NULL;
        menu->wonder.location_count = menu->wonder.location_capacity = 0;
        menu->wonder.catalog_ready = false;
        SDL_StopTextInput();
        construction_reset(&menu->construction);
        family_gameplay_free(&menu->family);
        ck2_law_state_free(&menu->active_laws);
        ck2_law_catalog_free(&menu->law_catalog);
        ck2_elections_free(&menu->election_catalog);
        ck2_trade_catalog_free(&menu->trade_catalog);
        menu->government_mechanics=(struct ck2_government_mechanics){0};
        menu->government_rules=(struct ck2_government_rules){0};
        char *gov_defines=load_data(menu->asset_root,"common/defines.lua");
        if (!ck2_government_rules_parse(&menu->government_rules,gov_defines))
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,"Nomad/merchant defines.lua rules missing");
        SDL_free(gov_defines);
        static const char *const trade_files[]={"common/buildings/00_tradepost.txt",
                                                "common/buildings/01_tradepost.txt"};
        for (size_t tf=0;tf<SDL_arraysize(trade_files);++tf) {
            char *data=load_data(menu->asset_root,trade_files[tf]);
            if (!data) continue;
            if (!ck2_trade_catalog_parse(&menu->trade_catalog,data))
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,"Trade post definitions malformed: %s",trade_files[tf]);
            SDL_free(data);
        }
        struct ck2_asset_files election_files=ck2_list_assets(menu->asset_root,"common/succession_voting");
        for (size_t i=0;i<election_files.count;++i) {
            const char *name=election_files.names[i];
            const size_t len=strlen(name);
            if (len<4 || strcmp(name+len-4,".txt")) continue;
            char *source=load_from_directory(menu->asset_root,"common/succession_voting",name);
            if (source) {
                if (!ck2_elections_parse(&menu->election_catalog,source))
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,"Cannot index CK2 elective rules: %s",name);
                SDL_free(source);
            }
        }
        ck2_free_asset_files(&election_files);
        SDL_Log("Indexed %zu original CK2 election rules (scripted voting not yet evaluated)",
                menu->election_catalog.count);
        static const char *const law_files[] = {
            "common/laws/demesne_laws.txt", "common/laws/ze_demesne_laws.txt",
            "common/laws/ze_obligation_laws.txt", "common/laws/succession_laws.txt",
            "common/laws/crown_laws.txt", "common/laws/ze_crown_laws.txt",
            "common/laws/ze_council_power_laws.txt"
        };
        for (size_t law_i=0;law_i<SDL_arraysize(law_files);++law_i) {
            char *source=load_data(menu->asset_root,law_files[law_i]);
            if (source) {
                if (!ck2_law_parse(&menu->law_catalog,source))
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,"Could not parse CK2 laws: %s",law_files[law_i]);
                SDL_free(source);
            }
        }
        SDL_Log("Indexed %zu authored CK2 laws (no scripted law eligibility yet)",menu->law_catalog.count);
        warfare_state_free(&menu->warfare);
        warfare_graph_free(&menu->warfare_graph);
        conversion_state_free(&menu->conversion);
        conversion_catalog_load_files(menu);
        /* Dynamic IDs are reserved high above CK2's ordinary character IDs.
         * Do not write these provisional IDs into history files. */
        menu->family.next_id = 0xf0000000u;
        char *family_defines = load_data(menu->asset_root, "common/defines.lua");
        if (!family_gameplay_rules(&menu->family, family_defines))
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Family rules unavailable: marriage/pregnancy actions disabled");
        SDL_free(family_defines);
        menu->character.open = false;
        menu->character.ruler.id = 0;
        menu->character.show_vassals = false;
        menu->character.vassal_scroll = 0;
        menu->building_open = false;
        menu->selected = NULL;
        menu->holding_province = 0;
        menu->holdings = (struct county_holdings){0};
        campaign_start(&menu->campaign);
        const struct starting_era *era = &menu->era.eras[menu->era.selected];
        menu->start_date = era->date;
        menu->campaign.date = menu->start_date;
        menu->campaign_active = true;
        const char *title = "";
        unsigned capital = 0;
        const struct ruler_profile *profile = setup_profile(menu, &title, &capital);
        const int index = menu->era.ruler >= 0 ? menu->era.ruler
            : era->date.year == 1066 || !era->count ? -1 : 0;
        menu->campaign_ruler = profile ? featured_ruler(era, profile->id)
            : index >= 0 ? &era->rulers[index] : NULL;
        menu->campaign_ruler_id = profile ? profile->id : menu->campaign_ruler ? menu->campaign_ruler->id : 0;
        menu->campaign_start_ruler_id = menu->campaign_ruler_id;
        if (menu->campaign_portrait)
            SDL_DestroyTexture(menu->campaign_portrait);
        menu->campaign_portrait = menu->setup_province ? menu->setup_portrait : NULL;
        if (menu->setup_province)
            menu->setup_portrait = NULL;
        menu->home_province = capital ? capital : menu->setup_province ? menu->setup_province
            : menu->campaign_ruler ? realm_capital(menu->asset_root, &menu->provinces, menu->campaign_ruler->title)
            : YORK_PROVINCE;
        if (!province_by_id(&menu->provinces, menu->home_province))
            menu->home_province = 0;
        select_province(menu, province_by_id(&menu->provinces, menu->home_province));
    }
    if (new_game) map_title_shields_clear(menu);
    if (!load_realms(menu, window, menu->start_date))
        menu->realms_visible = false;
    if (new_game) {
        struct realm_title_info historical_primary;
        if (realm_primary_title(&menu->realms,menu->campaign_ruler_id,&historical_primary)) {
            char path[160];
            SDL_snprintf(path,sizeof path,"history/titles/%s.txt",historical_primary.id);
            char *title_history=load_data(menu->asset_root,path);
            if (title_history) {
                const unsigned cutoff=(unsigned)(menu->start_date.year*10000 +
                    menu->start_date.month*100 + menu->start_date.day);
                if (!ck2_law_title_history(&menu->active_laws,&menu->law_catalog,title_history,cutoff))
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,"Unable to apply primary title's dated laws");
                SDL_free(title_history);
            }
        }
    }
    if (new_game) {
        SDL_DestroyTexture(menu->campaign_title_shield);
        menu->campaign_title_shield = NULL;
        struct realm_title_info primary;
        if (realm_primary_title(&menu->realms, menu->campaign_ruler_id, &primary)) {
            const struct ruler_profile *ruler = realm_ruler(&menu->realms, menu->campaign_ruler_id);
            menu->campaign_title_shield = era_title_shield(menu->renderer, menu->asset_root,
                primary.id, ruler ? ruler->religion_key : "");
        }
    }
    if ((new_game || !menu->economy.direct_holdings) && menu->realms.history) {
        if (economy_snapshot_build(&menu->economy, menu->asset_root, &menu->provinces,
                                    &menu->realms, &menu->construction, menu->campaign_ruler_id,
                                    menu->start_date) == 0) {
            refresh_law_income(menu);
            menu->campaign.monthly_prestige = menu->economy.monthly_prestige;
            menu->campaign.monthly_piety = menu->economy.monthly_piety;
            menu->campaign.demesne_count = menu->economy.direct_count;
            SDL_Log("Economy bookmark snapshot: %zu personal holdings (%u county-holder fallbacks), "
                    "%.2f direct annual tax, %.2f monthly prestige, %.2f monthly piety; "
                    "%u holdings missing base tax definitions",
                    menu->economy.direct_count, menu->economy.holder_fallbacks,
                    menu->economy.annual_direct_tax, menu->economy.monthly_prestige,
                    menu->economy.monthly_piety, menu->economy.missing_tax_definitions);
        } else SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Economy snapshot unavailable");
    }
    if (new_game) {
        const struct ruler_profile *r = realm_ruler(&menu->realms, menu->campaign_ruler_id);
        if (r) {
            if ((r->known_resources & (1u << RULER_WEALTH)) && r->resources[RULER_WEALTH] >= 0) {
                const double money = r->resources[RULER_WEALTH];
                menu->campaign.treasury = money >= UINT_MAX ? UINT_MAX : (unsigned)money;
                menu->campaign.gold_remainder = money >= UINT_MAX ? 0 : money - menu->campaign.treasury;
            }
            if (r->known_resources & (1u << RULER_PRESTIGE))
                menu->campaign.prestige = r->resources[RULER_PRESTIGE];
            if (r->known_resources & (1u << RULER_PIETY))
                menu->campaign.piety = r->resources[RULER_PIETY];
        }
    }
    if ((new_game || !menu->settlement_scene.markers) && menu->realms.history &&
        map_settlements_build(&menu->settlement_scene, menu->asset_root,
                              &menu->provinces, &menu->realms, menu->start_date) < 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Map settlements unavailable: %s", SDL_GetError());
    government_assign_bookmark(menu);
    government_assign_holdings_bookmark(menu);
    /* Initial tax snapshot may have been calculated before governments were
     * assigned. Rerun once with the actual government exemption rules. */
    if (menu->economy.direct_count || menu->economy.vassal_count)
        refresh_law_income(menu);
    for (size_t i = 0; i < menu->conversion.province_count; ++i)
        conversion_replay_province(menu, menu->conversion.provinces[i].id);
    if (menu->selected) {
        refresh_province_title_shield(menu);
        refresh_holding_style(menu);
    }
    if (new_game && menu->settlement_scene.province_facts) {
        size_t cultures = 0, religions = 0, histories = 0;
        for (size_t i = 0; i < menu->settlement_scene.province_fact_count; ++i) {
            const struct map_province_facts *facts = menu->settlement_scene.province_facts + i;
            histories += facts->loaded;
            cultures += facts->culture[0] != 0;
            religions += facts->religion[0] != 0;
        }
        SDL_Log("Province history audit: %zu files loaded, %zu culture keys, %zu religion keys",
                histories, cultures, religions);
    }
    load_campaign_topbar(menu);
    campaign_hud_ui_load(&menu->hud_ui, menu->renderer, menu->asset_root,
                         (SDL_Point){WIDTH, HEIGHT});
    map_theme_free(&menu->map_theme);
    menu->map_modes.selected = menu->realms_visible ? MAP_MODE_REALMS : MAP_MODE_TERRAIN;
    if (new_game)
        menu->campaign_speed = 4;
    menu_sound_stop_country(&menu->music);
    menu->screen = CAMPAIGN;
    menu->focus = TOGGLE_TIME;
    menu->time_running = false;
}

static bool navigable(enum screen screen)
{
    return screen == MAP_SETUP || screen == CAMPAIGN;
}

static void move_map(struct menu *menu, SDL_Window *window, int dx, int dy, int zoom)
{
    if (!map_view_move(&menu->campaign_map, (float)dx, (float)dy, zoom))
        return;
    const struct campaign_date date = menu->screen == CAMPAIGN ? menu->start_date
        : menu->era.eras[menu->era.selected].date;
    if (menu->realms_visible && !load_realms(menu, window, date))
        menu->realms_visible = false;
    if (menu->screen == CAMPAIGN) {
        if (menu->map_modes.selected != MAP_MODE_TERRAIN &&
            menu->map_modes.selected != MAP_MODE_REALMS) {
            if (map_theme_build(&menu->map_theme, menu->renderer, &menu->campaign_map,
                                &menu->provinces, &menu->realms, &menu->settlement_scene,
                                &menu->construction, menu->asset_root,
                                menu->start_date, menu->map_modes.selected) < 0)
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Map mode refresh failed");
        } else {
            menu->map_modes.selected = menu->realms_visible ? MAP_MODE_REALMS : MAP_MODE_TERRAIN;
        }
    }
    if (menu->screen == CAMPAIGN && menu->selected) {
        const struct province *selected = menu->selected;
        menu->selected = NULL;
        select_province(menu, selected);
    }
}

static SDL_Texture *setup_character_portrait(struct menu *menu, const struct ruler_profile *profile)
{
    if (!profile || !profile->name[0])
        return NULL;
    if (!menu->portrait_source_loaded) {
        portraits_open(&menu->portrait_source, menu->asset_root);
        menu->portrait_source_loaded = true;
    }
    struct era_ruler ruler = {.id = profile->id, .age = profile->age,
        .portrait_year = menu->start_date.year,
        .female = profile->female, .age_known = profile->age_known};
    SDL_strlcpy(ruler.dna, profile->dna, sizeof ruler.dna);
    SDL_strlcpy(ruler.properties, profile->properties, sizeof ruler.properties);
    SDL_strlcpy(ruler.culture, profile->culture_key, sizeof ruler.culture);
    SDL_strlcpy(ruler.religion, profile->religion_key, sizeof ruler.religion);
    return portrait_render(&menu->portrait_source, menu->renderer, menu->asset_root, &ruler);
}

static SDL_Texture *character_relative_portrait(void *context, const struct ruler_profile *profile)
{
    return setup_character_portrait(context, profile);
}

static bool wonder_catalog_open(struct menu *menu, int step)
{
    wonder_catalog_build(menu);
    struct wonder_panel *wonder = &menu->wonder;
    if (!wonder->location_count) return false;
    const size_t index = wonder_navigation_index(wonder->locations, wonder->location_count,
                                                  menu->selected ? menu->selected->id : 0,
                                                  step);
    const struct province *province = province_by_id(&menu->provinces, wonder->locations[index]);
    if (!province) return false;
    menu->character.open = false;
    select_province(menu, province);
    wonder->open = load_wonder_view(menu);
    return wonder->open;
}

static bool campaign_wonder_click(struct menu *menu, SDL_Point point)
{
    struct wonder_panel *wonder = &menu->wonder;
    if (wonder->open) {
        if (SDL_PointInRect(&point, &wonder->close.area)) {
            wonder->open = false;
        } else if (wonder->next.texture && SDL_PointInRect(&point, &wonder->next.area)) {
            (void)wonder_catalog_open(menu, 1);
        } else if (wonder->previous.texture && SDL_PointInRect(&point, &wonder->previous.area)) {
            (void)wonder_catalog_open(menu, -1);
        }
        return true; /* Great Works is modal; no click-through. */
    }
    if (menu->character.open || menu->building_open || !wonder->id[0] || !wonder->button.texture ||
        !SDL_PointInRect(&point, &wonder->button.area))
        return false;
    wonder->open = load_wonder_view(menu);
    if (wonder->open)
        menu_sound_play(&menu->music, CUE_GENERIC);
    else
        SDL_Log("Great Works: %s", SDL_GetError());
    return true;
}

static void inspect_campaign_character(struct menu *menu, const struct ruler_profile *source)
{
    struct character_view *view = &menu->character;
    if (!source)
        return;
    if (!view->background.texture && character_view_load(view, menu->renderer, menu->asset_root,
                                                          (SDL_Point){WIDTH, HEIGHT}) < 0) {
        SDL_Log("%s", SDL_GetError());
        return;
    }
    if (view->ruler.id != source->id) {
        struct ruler_profile profile;
        (void)family_gameplay_get(&menu->family, source, &profile);
        /* Government assignment belongs to the current realm snapshot, not
         * the family's earlier imported copy of the character. */
        SDL_strlcpy(profile.government_key, source->government_key,
                    sizeof profile.government_key);
        profile.government_inferred=source->government_inferred;
        /* Runtime faith/culture overrides must also survive a family edit. */
        const char *faith = conversion_person_value(&menu->conversion, profile.id,
                                                     CONVERSION_RELIGION, profile.religion_key);
        const char *culture = conversion_person_value(&menu->conversion, profile.id,
                                                       CONVERSION_CULTURE, profile.culture_key);
        char faith_key[64], culture_key[64];
        SDL_strlcpy(faith_key, faith, sizeof faith_key);
        SDL_strlcpy(culture_key, culture, sizeof culture_key);
        SDL_strlcpy(profile.religion_key, faith_key, sizeof profile.religion_key);
        SDL_strlcpy(profile.culture_key, culture_key, sizeof profile.culture_key);
        SDL_strlcpy(profile.religion, ck2_localize(faith_key, faith_key), sizeof profile.religion);
        SDL_strlcpy(profile.culture, ck2_localize(culture_key, culture_key), sizeof profile.culture);
        /* Marriage history can be recorded only on the other spouse. */
        if (!profile.spouse && view->partner.profile.id == profile.id && !view->ruler.deceased)
            profile.spouse = view->ruler.id;
        character_view_set(view, menu->asset_root, menu->start_date, &profile, menu->renderer,
                           character_relative_portrait, menu);
        /* The original portrait loader only scans static character history.
         * Merge runtime births into the same child-card slots. */
        unsigned ids[CHARACTER_FAMILY_LIMIT];
        const size_t child_count = family_gameplay_children(&menu->family, profile.id,
                                                             ids, CHARACTER_FAMILY_LIMIT);
        for (size_t i = 0; i < child_count && i < CHARACTER_FAMILY_LIMIT; ++i) {
            const struct ruler_profile *child = family_gameplay_find(&menu->family, ids[i]);
            if (!child) continue;
            size_t slot = 0;
            while (slot < CHARACTER_FAMILY_LIMIT &&
                   view->family_members[2][slot].profile.id &&
                   view->family_members[2][slot].profile.id != child->id) ++slot;
            if (slot == CHARACTER_FAMILY_LIMIT) continue;
            struct character_relative *relative = &view->family_members[2][slot];
            if (relative->profile.id != child->id) {
                SDL_DestroyTexture(relative->portrait);
                relative->portrait = character_relative_portrait(menu, child);
            }
            relative->profile = *child;
        }
        const struct ruler_profile *partner = family_gameplay_find(&menu->family,
                                                                   profile.spouse);
        if (partner) view->partner.profile = *partner;
        character_view_titles(view, &menu->realms, menu->renderer, menu->asset_root);
    }
    menu->building_open = false;
    view->open = true;
    menu_sound_play(&menu->music, CUE_GENERIC);
}

static bool campaign_character_click(struct menu *menu, SDL_Point point)
{
    static const SDL_Rect player_portrait = {2, 0, 130, 130};
    struct character_view *view = &menu->character;
    if (SDL_PointInRect(&point, &player_portrait)) {
        if (view->open && view->ruler.id == menu->campaign_ruler_id)
            view->open = false;
        else
            inspect_campaign_character(menu, realm_ruler(&menu->realms, menu->campaign_ruler_id));
        return true;
    }
    if (!view->open)
        return false;
    if (SDL_PointInRect(&point, &view->close.area)) {
        view->open = false;
        return true;
    }
    if (view->extra_icons[3].texture && SDL_PointInRect(&point, &view->extra_icons[3].area)) {
        if (bloodline_view_load(&menu->bloodlines, menu->renderer, menu->asset_root,
                                (SDL_Point){WIDTH, HEIGHT}) < 0)
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Bloodlines window: %s", SDL_GetError());
        else {
            SDL_strlcpy(menu->bloodlines.owner_name, view->ruler.name, sizeof menu->bloodlines.owner_name);
            menu->bloodlines.open = true;
            view->open = false;
        }
        return true;
    }
    if (view->extra_icons[11].texture && SDL_PointInRect(&point, &view->extra_icons[11].area)) {
        if (!wonder_catalog_open(menu, 0))
            SDL_Log("Great Works: no constructed works available in recorded province history");
        return true;
    }
    enum tree_mode requested = TREE_FAMILY;
    bool open_tree = false;
    if (view->extra_icons[0].texture && SDL_PointInRect(&point, &view->extra_icons[0].area)) {
        requested = TREE_FAMILY; open_tree = true;
    } else if (view->extra_icons[1].texture && SDL_PointInRect(&point, &view->extra_icons[1].area)) {
        requested = TREE_REALM; open_tree = true;
    } else if (view->dynasty_arms &&
               SDL_PointInRect(&point, &view->dynasty_shield_area)) {
        requested = TREE_DYNASTY; open_tree = true;
    } else if (SDL_PointInRect(&point, &view->labels[2])) {
        requested = TREE_DYNASTY; open_tree = true;
    }
    if (open_tree) {
        if (tree_view_open(&menu->genealogy, requested, menu->renderer,
                           menu->asset_root, view, &menu->realms, menu->start_date,
                           character_relative_portrait, menu) < 0)
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not open genealogy view: %s", SDL_GetError());
        return true;
    }
    unsigned selected_id;
    const struct ruler_profile *selected = character_view_pick(view, point, &selected_id);
    if (selected || selected_id) {
        inspect_campaign_character(menu, selected ? selected : realm_ruler(&menu->realms, selected_id));
        return true;
    }
    if (SDL_PointInRect(&point, &view->tabs[0].area))
        view->show_vassals = false;
    if (SDL_PointInRect(&point, &view->tabs[2].area) && !view->show_vassals) {
        view->show_vassals = true;
        character_view_vassals(view, &menu->realms, menu->renderer, menu->asset_root,
                               character_relative_portrait, menu);
    }
    return SDL_PointInRect(&point, &view->background.area);
}

static void refresh_vassal_art(struct menu *menu, const struct realm_title_info *title)
{
    struct realm_title_info rows[VASSAL_DISPLAY_LIMIT];
    const int limit = SDL_min(lobby_rows(menu, LOBBY_VASSALS), VASSAL_DISPLAY_LIMIT);
    const size_t total = title ? realm_vassal_titles(&menu->realms, title, menu->vassal_scroll, rows, limit) : 0;
    for (size_t i = 0; i < SDL_arraysize(menu->vassal_art); ++i) {
        struct vassal_art *art = &menu->vassal_art[i];
        SDL_DestroyTexture(art->portrait);
        SDL_DestroyTexture(art->shield);
        *art = (struct vassal_art){0};
        if (i >= (size_t)limit || menu->vassal_scroll + i >= total)
            continue;
        const struct ruler_profile *profile = realm_ruler(&menu->realms, rows[i].holder);
        art->portrait = setup_character_portrait(menu, profile);
        art->shield = era_title_shield(menu->renderer, menu->asset_root, rows[i].id,
                                       profile ? profile->religion_key : "");
    }
}

static void preview_portrait(struct menu *menu, const struct realm_title_info *title)
{
    SDL_DestroyTexture(menu->setup_portrait);
    SDL_DestroyTexture(menu->setup_shield);
    const struct ruler_profile *profile = title ? realm_ruler(&menu->realms, title->holder) : NULL;
    menu->setup_portrait = setup_character_portrait(menu, profile);
    menu->setup_shield = title ? era_title_shield(menu->renderer, menu->asset_root, title->id,
                                                  profile ? profile->religion_key : "") : NULL;
    refresh_vassal_art(menu, title);
}

static bool scroll_vassals(struct menu *menu, const struct realm_title_info *title, SDL_Point point)
{
    const int rows = lobby_rows(menu, LOBBY_VASSALS);
    const int maximum = (int)realm_vassal_titles(&menu->realms, title, 0, NULL, 0) - rows;
    if (maximum <= 0)
        return false;
    struct setup_widget parts[SETUP_SCROLL_PARTS];
    setup_scrollbar(menu, LOBBY_VASSALS, (int)menu->vassal_scroll, maximum, parts);
    int target = (int)menu->vassal_scroll;
    if (SDL_PointInRect(&point, &parts[SETUP_SCROLL_UP].area))
        --target;
    else if (SDL_PointInRect(&point, &parts[SETUP_SCROLL_DOWN].area))
        ++target;
    else if (SDL_PointInRect(&point, &parts[SETUP_SCROLL_TRACK].area))
        target = (point.y - parts[SETUP_SCROLL_TRACK].area.y) * maximum /
                 SDL_max(1, parts[SETUP_SCROLL_TRACK].area.h - 1);
    else
        return false;
    target = SDL_clamp(target, 0, maximum);
    if (target != (int)menu->vassal_scroll) {
        menu->vassal_scroll = (size_t)target;
        refresh_vassal_art(menu, title);
    }
    return true;
}

static void select_setup_title(struct menu *menu, const struct realm_title_info *title,
                               unsigned province)
{
    SDL_strlcpy(menu->setup_title, title->id, sizeof menu->setup_title);
    menu->setup_province = province ? province : title->capital;
    if (!menu->setup_province && title->id[0] == 'b') {
        struct realm_title_info county;
        if (realm_title_info(&menu->realms, title->liege, &county))
            menu->setup_province = county.capital;
    }
    menu->vassal_scroll = 0;
    preview_portrait(menu, title);
    menu_sound_play(&menu->music, CUE_COUNTRY);
}

static void select_setup_mode(struct menu *menu, SDL_Window *window, enum setup_control mode)
{
    if (mode == SETUP_REALMS && !menu->realms_visible)
        menu->realms_visible = load_realms(menu, window, menu->era.eras[menu->era.selected].date);
    else if (mode == SETUP_TERRAIN)
        menu->realms_visible = false;
    menu_sound_play(&menu->music, CUE_GENERIC);
}

static bool setup_select(struct menu *menu, SDL_Window *window, SDL_Point point)
{
    const enum setup_control mode = setup_toolbar_hit(&menu->toolbar, point);
    if (mode != SETUP_NO_CONTROL) {
        if (mode <= SETUP_REALMS)
            select_setup_mode(menu, window, mode);
        return true;
    }
    if (setup_lobby_deferred(&menu->toolbar, point))
        return true;
    for (int slot = 0; slot < lobby_rows(menu, LOBBY_BOOKMARKS) && slot + menu->bookmark_scroll < menu->era.count; ++slot) {
        const int i = slot + menu->bookmark_scroll;
        const SDL_Rect row = lobby_row(menu, LOBBY_BOOKMARKS, slot);
        if (SDL_PointInRect(&point, &row)) {
            if (i != menu->era.selected && load_realms(menu, window, menu->era.eras[i].date)) {
                menu->era.selected = i;
                menu->era.ruler = -1;
                menu->featured_scroll = 0;
                menu->setup_title[0] = 0;
                menu->setup_province = 0;
                preview_portrait(menu, NULL);
            }
            menu_sound_play(&menu->music, CUE_BOOKMARK);
            return true;
        }
    }
    const struct starting_era *era = &menu->era.eras[menu->era.selected];
    for (int slot = 0; slot < lobby_rows(menu, LOBBY_FEATURED) && slot + menu->featured_scroll < era->count; ++slot) {
        const int i = slot + menu->featured_scroll;
        const SDL_Rect row = lobby_row(menu, LOBBY_FEATURED, slot);
        if (SDL_PointInRect(&point, &row)) {
            struct realm_title_info title;
            menu->era.ruler = i;
            if (realm_title_info(&menu->realms, era->rulers[i].title, &title) ||
                realm_primary_title(&menu->realms, era->rulers[i].id, &title))
                select_setup_title(menu, &title, 0);
            return true;
        }
    }
    struct realm_title_info inspected;
    if (inspected_title(menu, &inspected)) {
        if (scroll_vassals(menu, &inspected, point))
            return true;
        if (SDL_PointInRect(&point, &menu->toolbar.lobby[LOBBY_STATUS].area)) {
            struct realm_title_info liege;
            if (realm_title_info(&menu->realms, inspected.liege, &liege) && liege.holder)
                select_setup_title(menu, &liege, 0);
            return true;
        }
        const int rows = lobby_rows(menu, LOBBY_VASSALS);
        for (int i = 0; i < rows; ++i) {
            const SDL_Rect row = lobby_row(menu, LOBBY_VASSALS, i);
            if (SDL_PointInRect(&point, &row)) {
                struct realm_title_info vassal;
                const size_t offset = menu->vassal_scroll + (size_t)i;
                if (realm_vassal_titles(&menu->realms, &inspected, offset, &vassal, 1) > offset)
                    select_setup_title(menu, &vassal, 0);
                return true;
            }
        }
    }
    if (SDL_PointInRect(&point, &menu->toolbar.lobby[LOBBY_BOTTOM].area) ||
        SDL_PointInRect(&point, &menu->toolbar.lobby[LOBBY_LEFT].area) || SDL_PointInRect(&point, &menu->toolbar.lobby[LOBBY_RIGHT].area))
        return true;
    SDL_Point world;
    if (map_view_pick(&menu->campaign_map, point.x, point.y, &world)) {
        const struct province *province = province_at(&menu->provinces, world.x, world.y);
        struct realm_title_info title;
        const SDL_Keymod mods = SDL_GetModState();
        if (province && realm_county_info(&menu->realms, (size_t)(province - menu->provinces.provinces),
                                          (mods & KMOD_SHIFT) != 0, &title)) {
            if ((mods & KMOD_ALT) && !(mods & KMOD_SHIFT))
                realm_primary_title(&menu->realms, title.holder, &title);
            select_setup_title(menu, &title, province->id);
        }
    }
    return true;
}

static bool world_screen(enum screen screen)
{
    return screen == CAMPAIGN || screen == MAP_SETUP || screen == GAME_RULES;
}

static void draw(SDL_Renderer *renderer,
                 const struct menu_assets *assets, struct menu *menu)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    const bool world = world_screen(menu->screen);
    SDL_Texture *map = world ? menu->campaign_map.texture : menu->menu_map.texture;
    SDL_RenderCopy(renderer, map, NULL, NULL);
    if (world && menu->realms_visible)
        realm_map_draw(&menu->realms, renderer);
    if (menu->screen == CAMPAIGN && menu->map_modes.selected != MAP_MODE_TERRAIN &&
        menu->map_modes.selected != MAP_MODE_REALMS)
        map_theme_draw(&menu->map_theme, renderer);
    if (menu->screen == CAMPAIGN)
        draw_map_settlements(renderer, assets, menu);
    if (!world && menu->screen != STARTING_ERA) {
        blit(renderer, assets->title, 15, 5);
        blit(renderer, assets->logo, WIDTH - 155, 8);
    }

    if (menu->screen == OPTIONS) {
        settings_draw(&menu->settings, renderer, assets, centered_text);
        SDL_RenderPresent(renderer);
        return;
    }

    if (menu->screen == CAMPAIGN) {
        draw_campaign(renderer, assets, menu);
    } else if (menu->screen == GAME_RULES) {
        rules_draw(&menu->rules, renderer, assets, centered_text, menu->pointer);
    } else if (menu->screen == MAP_SETUP) {
        draw_setup(renderer, assets, menu);
    } else if (menu->screen == STARTING_ERA) {
        era_menu_draw(&menu->era, renderer, assets, centered_text, menu->pointer);
    } else if (menu->screen == SINGLE_PLAYER) {
        draw_single_player(renderer, assets, menu);
    } else if (menu->screen == CREDITS) {
        blit(renderer, assets->credits_frame, 530, 110);
        SDL_SetTextureColorMod(assets->font, 53, 37, 25);
        draw_credits(renderer, assets, menu->credit_scroll);
        SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    } else {
        blit(renderer, assets->top, 608, 230);
        blit(renderer, assets->bottom_content, 608, 562);
        draw_menu_content(renderer, assets, &menu->catalog);
        blit(renderer, assets->version_plate, 16, 843);
        static const char version[] = OPENCK2_VERSION_LABEL;
        centered_text(renderer, assets, version, sizeof version - 1, 90, 851);
        const SDL_Rect welcome_plate = {1186, 811, 394, 35};
        static const char welcome[] = "Welcome back, Crusader!";
        SDL_RenderCopy(renderer, assets->version_plate, NULL, &welcome_plate);
        centered_text(renderer, assets, welcome, sizeof welcome - 1,
                      welcome_plate.x + welcome_plate.w / 2, welcome_plate.y + 7);
    }

    size_t count;
    const struct button *buttons = buttons_for(menu, &count);
    for (size_t i = 0; i < count; ++i) {
        const struct button *button = &buttons[i];
        const bool enabled = action_enabled(menu, button->action);
        const bool compact = button->area.h < BUTTON_HEIGHT;
        const bool hovered = enabled && button->action == menu->hover;
        SDL_Texture *texture = compact ? (hovered ? assets->compact_hover : assets->compact_button)
                                       : (hovered ? assets->button_hover : assets->button);
        if (menu->screen == STARTING_ERA)
            texture = hovered || !enabled ? menu->era.button_hover : menu->era.button;
        const Uint8 brightness = !enabled ? 145 : button->action == menu->focus ? 255 : 235;
        const char *label = button_label(button, menu);
        if (menu->screen == MAP_SETUP) {
            const enum lobby_element element = button->action == BACK ? LOBBY_BACK : LOBBY_PLAY;
            const struct setup_widget *widget = &menu->toolbar.lobby[element];
            SDL_SetTextureColorMod(widget->texture, brightness, brightness, brightness);
            setup_widget_draw(widget, renderer, enabled ? menu->pointer : (SDL_Point){-1, -1});
            SDL_SetTextureColorMod(widget->texture, 255, 255, 255);
        } else {
            SDL_SetTextureColorMod(texture, brightness, brightness, brightness);
            SDL_RenderCopy(renderer, texture, NULL, &button->area);
        }
        SDL_SetTextureAlphaMod(assets->font, enabled ? 255 : 150);
        centered_text(renderer, assets, label, strlen(label), button->area.x + button->area.w / 2,
                      button->area.y + (button->area.h - 22) / 2);
    }
    SDL_SetTextureAlphaMod(assets->font, 255);
    if (action_unimplemented(menu->hover) ||
        (menu->screen == STARTING_ERA && (era_menu_deferred(&menu->era, menu->pointer) ||
         (menu->hover == START_ERA && menu->era.selected >= 0 && !era_menu_can_start(&menu->era)))))
        settings_unavailable_tooltip(renderer, assets, centered_text, menu->pointer);
    if (menu->screen == MAP_SETUP) {
        if (setup_toolbar_hit(&menu->toolbar, menu->pointer) >= SETUP_COUNTIES ||
            setup_lobby_deferred(&menu->toolbar, menu->pointer))
            settings_unavailable_tooltip(renderer, assets, centered_text, menu->pointer);
        else
            setup_tooltip(renderer, assets, menu);
    }
    if (menu->screen == CAMPAIGN) {
        technology_view_draw(&menu->technology, renderer, assets, menu->pointer, centered_text);
        bloodline_view_draw(&menu->bloodlines, renderer, assets, menu->pointer, centered_text);
        tree_view_draw(&menu->genealogy, renderer, assets, centered_text);
        draw_scripted_event(renderer, assets, menu);
        draw_debug_console(renderer, assets, &menu->console);
    }
    SDL_RenderPresent(renderer);
}

static enum action hit_test(const struct menu *menu, int x, int y)
{
    size_t count;
    const struct button *buttons = buttons_for(menu, &count);
    const SDL_Point point = {x, y};
    if (menu->screen == CAMPAIGN) {
        if (menu->topbar_ready) {
            if (SDL_PointInRect(&point, &menu->topbar[BAR_UP].area))
                return SPEED_UP;
            if (SDL_PointInRect(&point, &menu->topbar[BAR_DOWN].area))
                return SPEED_DOWN;
            if (SDL_PointInRect(&point, &menu->topbar[BAR_PAUSE].area))
                return TOGGLE_TIME;
        } else if (SDL_PointInRect(&point, &(SDL_Rect){1331, 40, 216, 24})) {
            return TOGGLE_TIME;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        if (SDL_PointInRect(&point, &buttons[i].area))
            return buttons[i].action;
    }
    return UNAVAILABLE;
}

static void activate(struct menu *menu, SDL_Window *window, enum action action)
{
    if (!action_enabled(menu, action))
        return;
    enum menu_cue cue = CUE_MENU;
    switch (action) {
    case QUIT: cue = CUE_QUIT; break;
    case LAUNCH_CAMPAIGN: cue = CUE_PLAY; break;
    case CONTENT_SETTINGS: cue = CUE_GENERIC; break;
    case NEXT_MONTH: cue = CUE_TIME_FORWARD; break;
    case TOGGLE_REALMS: case TOGGLE_TIME: cue = CUE_GENERIC; break;
    case SELECT_HOME: cue = CUE_GENERIC; break;
    default: break;
    }
    menu_sound_play(&menu->music, cue);
    menu->hover = UNAVAILABLE;
    switch (action) {
    case OPEN_SINGLE_PLAYER:
        menu->screen = SINGLE_PLAYER;
        menu->focus = menu->campaign_active ? CONTINUE_GAME : NEW_GAME;
        break;
    case OPEN_OPTIONS:
        menu->screen = OPTIONS;
        settings_open(&menu->settings, window);
        break;
    case OPEN_CREDITS:
        menu->screen = CREDITS;
        menu->credit_scroll = 0;
        menu->focus = BACK;
        break;
    case BACK:
        if (menu->screen == GAME_RULES) {
            menu->screen = menu->rules_back;
            menu->focus = START_ERA;
            break;
        }
        if (menu->screen == MAP_SETUP) {
            menu->setup_province = 0;
            menu->setup_title[0] = 0;
            preview_portrait(menu, NULL);
            if (!menu->era.eras[menu->era.selected].card)
                menu->era.selected = menu->era.ruler = -1;
            menu->screen = STARTING_ERA;
            menu->focus = CUSTOM_SETUP;
            break;
        }
        if (menu->screen == STARTING_ERA) {
            if (menu->era.selected >= 0) {
                menu->era.selected = menu->era.ruler = -1;
                menu->focus = BACK;
            } else {
                menu->screen = SINGLE_PLAYER;
                menu->focus = NEW_GAME;
            }
            break;
        }
        if (menu->screen == CAMPAIGN) {
            menu->time_running = false;
            menu->focus = OPEN_SINGLE_PLAYER;
        } else if (menu->screen == SINGLE_PLAYER) {
            menu->focus = OPEN_SINGLE_PLAYER;
        } else {
            menu->focus = menu->screen == OPTIONS ? OPEN_OPTIONS : OPEN_CREDITS;
        }
        menu->screen = MAIN_MENU;
        break;
    case QUIT:
        menu->running = false;
        break;
    case NEW_GAME:
        if (!menu->era.background && era_menu_load(&menu->era, menu->renderer, menu->asset_root) < 0) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Starting Era", SDL_GetError(), window);
            era_menu_free(&menu->era);
            menu->era = (struct era_menu){0};
            break;
        }
        menu->era.selected = menu->era.ruler = -1;
        menu->bookmark_scroll = menu->featured_scroll = 0;
        menu->setup_province = 0;
        menu->setup_title[0] = 0;
        menu->pointer = (SDL_Point){-1, -1};
        menu->screen = STARTING_ERA;
        menu->focus = BACK;
        break;
    case START_ERA:
    case CUSTOM_SETUP: {
        const struct starting_era *era = &menu->era.eras[menu->era.selected];
        if (action == CUSTOM_SETUP && !menu->toolbar.widgets[SETUP_BANNER].texture &&
            setup_lobby_load(&menu->toolbar, menu->renderer, menu->asset_root, (SDL_Point){WIDTH, HEIGHT}) < 0) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Custom Game Setup", SDL_GetError(), window);
            break;
        }
        if (!load_realms(menu, window, era->date))
            break;
        menu->realms_visible = true;
        if (action == CUSTOM_SETUP) {
            menu->bookmark_scroll = SDL_clamp(menu->era.selected - lobby_rows(menu, LOBBY_BOOKMARKS) + 1,
                0, SDL_max(0, menu->era.count - lobby_rows(menu, LOBBY_BOOKMARKS)));
            menu->setup_buttons[0] = (struct button){menu->toolbar.lobby[LOBBY_BACK].area,
                                                     menu->toolbar.back_label, BACK};
            menu->setup_buttons[1] = (struct button){menu->toolbar.lobby[LOBBY_PLAY].area,
                                                     menu->toolbar.play_label, START_ERA};
            if (menu->era.ruler >= 0) {
                struct realm_title_info title;
                if (realm_title_info(&menu->realms, era->rulers[menu->era.ruler].title, &title))
                    select_setup_title(menu, &title, 0);
            }
            menu->screen = MAP_SETUP;
            menu->focus = START_ERA;
            break;
        }
        if (!menu->rules.frame && rules_load(&menu->rules, menu->renderer, menu->asset_root) < 0) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Rules", SDL_GetError(), window);
            rules_free(&menu->rules);
            break;
        }
        menu->rules_back = menu->screen;
        menu->screen = GAME_RULES;
        menu->focus = LAUNCH_CAMPAIGN;
        break;
    }
    case LAUNCH_CAMPAIGN:
    case CONTINUE_GAME:
        open_campaign(menu, window, action == LAUNCH_CAMPAIGN);
        break;
    case TOGGLE_REALMS: {
        const struct campaign_date date = menu->screen == CAMPAIGN ? menu->start_date
            : menu->era.eras[menu->era.selected].date;
        if (menu->realms_visible)
            menu->realms_visible = false;
        else
            menu->realms_visible = load_realms(menu, window, date);
        menu->map_modes.selected = menu->realms_visible ? MAP_MODE_REALMS : MAP_MODE_TERRAIN;
        break;
    }
    case SPEED_UP:
    case SPEED_DOWN:
        menu->campaign_speed = SDL_clamp(menu->campaign_speed + (action == SPEED_UP ? 1 : -1), 1, 5);
        menu->next_day = SDL_GetTicks64() + campaign_day_interval(menu->campaign_speed);
        break;
    case TOGGLE_TIME:
        menu->time_running = !menu->time_running;
        menu->next_day = SDL_GetTicks64() + campaign_day_interval(menu->campaign_speed);
        break;
    case NEXT_MONTH:
        menu->time_running = false;
        do {
            advance_campaign_day(menu);
        } while (menu->campaign.date.day != 1);
        break;
    case SELECT_HOME:
        select_province(menu, province_by_id(&menu->provinces, menu->home_province));
        break;
    case CONTENT_SETTINGS:
        open_content_settings(menu, window);
        break;
    case PARADOX_LINK:
    case PDF_LINK:
    case REDDIT_LINK:
    case GITHUB_LINK:
        if (SDL_OpenURL(links[action]) < 0)
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not open link", SDL_GetError(), window);
        break;
    case MULTIPLAYER:
    case LOAD_GAME:
    case TUTORIAL:
    case UNAVAILABLE:
        break;
    }
}

static void move_focus(struct menu *menu, int direction)
{
    size_t count;
    const struct button *buttons = buttons_for(menu, &count);
    int current = direction > 0 ? (int)count - 1 : 0;
    for (size_t i = 0; i < count; ++i) {
        if (action_enabled(menu, buttons[i].action) && buttons[i].action == menu->focus)
            current = (int)i;
    }
    do {
        current = (current + direction + (int)count) % (int)count;
    } while (!action_enabled(menu, buttons[current].action));
    menu->focus = buttons[current].action;
}

static void scroll_credits(struct menu *menu, int amount)
{
    menu->credit_scroll = SDL_clamp(menu->credit_scroll + amount, 0, menu->credit_scroll_max);
}

static void move_setup_thumb(struct menu *menu, int pointer_y)
{
    const enum lobby_element list = menu->scroll_list;
    const int count = list == LOBBY_BOOKMARKS ? menu->era.count : menu->era.eras[menu->era.selected].count;
    int *scroll = list == LOBBY_BOOKMARKS ? &menu->bookmark_scroll : &menu->featured_scroll;
    const int maximum = count - lobby_rows(menu, list);
    if (maximum <= 0)
        return;
    struct setup_widget parts[SETUP_SCROLL_PARTS];
    setup_scrollbar(menu, list, *scroll, maximum, parts);
    const SDL_Rect track = parts[SETUP_SCROLL_TRACK].area;
    const int travel = SDL_max(1, track.h - parts[SETUP_SCROLL_THUMB].area.h);
    const int offset = SDL_clamp(pointer_y - menu->bookmark_drag_offset - track.y, 0, travel);
    *scroll = (offset * maximum + travel / 2) / travel;
}

static bool press_setup_scrollbar(struct menu *menu, enum lobby_element list, SDL_Point point)
{
    const int count = list == LOBBY_BOOKMARKS ? menu->era.count : menu->era.eras[menu->era.selected].count;
    int *scroll = list == LOBBY_BOOKMARKS ? &menu->bookmark_scroll : &menu->featured_scroll;
    const int maximum = count - lobby_rows(menu, list);
    if (maximum <= 0)
        return false;
    struct setup_widget parts[SETUP_SCROLL_PARTS];
    setup_scrollbar(menu, list, *scroll, maximum, parts);
    if (SDL_PointInRect(&point, &parts[SETUP_SCROLL_UP].area))
        *scroll = SDL_max(0, *scroll - 1);
    else if (SDL_PointInRect(&point, &parts[SETUP_SCROLL_DOWN].area))
        *scroll = SDL_min(maximum, *scroll + 1);
    else if (SDL_PointInRect(&point, &parts[SETUP_SCROLL_THUMB].area)) {
        menu->bookmark_drag_offset = point.y - parts[SETUP_SCROLL_THUMB].area.y;
        menu->scroll_list = list;
        menu->bookmark_drag = true;
    } else if (SDL_PointInRect(&point, &parts[SETUP_SCROLL_TRACK].area)) {
        menu->bookmark_drag_offset = parts[SETUP_SCROLL_THUMB].area.h / 2;
        menu->scroll_list = list;
        move_setup_thumb(menu, point.y);
    } else {
        return false;
    }
    return true;
}

static SDL_Cursor *load_game_cursor(const char *root)
{
    char *path = ck2_asset_path(root, "gfx/cursors/normal.png");
    SDL_Surface *art = path ? IMG_Load(path) : NULL;
    free(path);
    if (!art)
        return NULL;
    SDL_Cursor *cursor = SDL_CreateColorCursor(art, 0, 0);
    SDL_FreeSurface(art);
    return cursor;
}

static void select_campaign_mode(struct menu *menu, SDL_Window *window, enum map_mode_id mode)
{
    /* No false coloring: unsupported buttons stay visible but inactive. */
    if (!map_mode_select(&menu->map_modes, mode))
        return;
    if (mode == MAP_MODE_REALMS && !menu->realms_visible)
        menu->realms_visible = load_realms(menu, window, menu->start_date);
    else if (mode == MAP_MODE_TERRAIN)
        menu->realms_visible = false;
    if (mode == MAP_MODE_REALMS && !menu->realms_visible)
        menu->map_modes.selected = MAP_MODE_TERRAIN;
    else if (mode != MAP_MODE_REALMS && mode != MAP_MODE_TERRAIN) {
        if (map_theme_build(&menu->map_theme, menu->renderer, &menu->campaign_map,
                            &menu->provinces, &menu->realms, &menu->settlement_scene,
                            &menu->construction, menu->asset_root, menu->start_date, mode) < 0) {
            menu->map_modes.selected = MAP_MODE_TERRAIN;
        } else {
            menu->realms_visible = false;
        }
    }
    menu_sound_play(&menu->music, CUE_GENERIC);
}

static void campaign_open_province_owner(struct menu *menu, SDL_Point pointer, bool top_liege)
{
    const struct province *province = campaign_pointer_province(menu, pointer);
    if (!province) return;
    const size_t index = (size_t)(province - menu->provinces.provinces);
    struct realm_province_ownership owners;
    realm_province_ownership(&menu->realms, index, &owners);
    const bool has_target = top_liege ? owners.has_independent : owners.has_county;
    const struct realm_title_info *target = top_liege ? &owners.independent : &owners.county;
    if (!has_target || !target->holder)
        return;
    const struct ruler_profile *holder = realm_ruler(&menu->realms, target->holder);
    if (holder) inspect_campaign_character(menu, holder);
}

static void toggle_technology(struct menu *menu)
{
    if (menu->technology.open) {
        menu->technology.open = false;
        return;
    }
    if (!menu->technology.loaded && technology_view_load(&menu->technology, menu->renderer,
                       menu->asset_root, (SDL_Point){WIDTH, HEIGHT}) < 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Technology UI: %s", SDL_GetError());
        return;
    }
    menu->technology.open = true;
    map_gesture_cancel(&menu->map_gesture);
}

static void conversion_command(struct menu *menu, struct debug_console_result command)
{
    enum conversion_kind kind = CONVERSION_RELIGION;
    switch (command.action) {
    case DEBUG_SET_PROVINCE_CULTURE: case DEBUG_SET_CHARACTER_CULTURE:
    case DEBUG_MISSION_CULTURE: case DEBUG_RESOLVE_CULTURE:
        kind = CONVERSION_CULTURE; break;
    default: break;
    }
    const bool province = command.action == DEBUG_SET_PROVINCE_CULTURE ||
        command.action == DEBUG_SET_PROVINCE_RELIGION;
    const bool character = command.action == DEBUG_SET_CHARACTER_CULTURE ||
        command.action == DEBUG_SET_CHARACTER_RELIGION;
    const bool mission = command.action == DEBUG_MISSION_CULTURE ||
        command.action == DEBUG_MISSION_RELIGION;
    const bool resolve = command.action == DEBUG_RESOLVE_CULTURE ||
        command.action == DEBUG_RESOLVE_RELIGION;
    if (command.action == DEBUG_CONVERSION_STATUS) {
        char output[200];
        snprintf(output, sizeof output, "Conversion records: %zu provinces; %zu characters; %u revision.",
                 menu->conversion.province_count, menu->conversion.person_count,
                 menu->conversion.revision);
        debug_console_output(&menu->console, output);
        for (int k = 0; k <= 1; ++k) {
            const struct conversion_mission *m = &menu->conversion.missions[k];
            if (m->active) {
                snprintf(output, sizeof output, "%s mission in province %u by ruler %u for %s (manual event resolution only).",
                         k ? "Culture" : "Faith", m->province_id, m->actor_id, m->target);
                debug_console_output(&menu->console, output);
            }
        }
        return;
    }
    const struct ruler_profile *player = realm_ruler(&menu->realms, menu->campaign_ruler_id);
    if (character) {
        if (!player) { debug_console_output(&menu->console, "No active landed character to convert."); return; }
        const char *current = kind == CONVERSION_RELIGION ? player->religion_key : player->culture_key;
        enum conversion_status status = conversion_person_set(&menu->conversion,
                            &menu->conversion_catalog, player->id, kind, command.argument, current);
        if (status == CONVERSION_OK) {
            conversion_replay_rulers(menu);
            menu->character.ruler.id = 0;
            if (menu->character.open)
                inspect_campaign_character(menu, realm_ruler(&menu->realms, menu->campaign_ruler_id));
            conversion_map_refresh(menu);
        }
        debug_console_output(&menu->console, conversion_status_name(status));
        return;
    }
    if (resolve && !menu->conversion.missions[kind].active) {
        debug_console_output(&menu->console, conversion_status_name(CONVERSION_NO_MISSION));
        return;
    }
    const unsigned id = resolve ? menu->conversion.missions[kind].province_id :
                             menu->selected ? menu->selected->id : 0;
    const struct province *selection = province_by_id(&menu->provinces, id);
    if (!selection) {
        debug_console_output(&menu->console, "Select a land province first (or assign a mission).");
        return;
    }
    const size_t index = (size_t)(selection - menu->provinces.provinces);
    if (index >= menu->settlement_scene.province_fact_count) {
        debug_console_output(&menu->console, "Province history is not loaded."); return;
    }
    const struct map_province_facts *facts = &menu->settlement_scene.province_facts[index];
    const char *current = kind == CONVERSION_RELIGION ? facts->religion : facts->culture;
    enum conversion_status status = CONVERSION_INVALID_TARGET;
    if (province) {
        status = conversion_province_set(&menu->conversion, &menu->conversion_catalog,
                                          id, kind, command.argument, current);
    } else if (mission) {
        /* The original job-action definitions constrain these assignments:
         * inquisition: own or vassal county; settle_tribe: own county + tribal.
         * More elaborate scripture, game rules and diplomacy remain absent. */
        struct realm_province_ownership owner;
        if (!player || !realm_province_ownership(&menu->realms, index, &owner) ||
            !owner.has_county || (kind == CONVERSION_CULTURE &&
            (owner.county.holder != player->id || strcmp(player->government_key, "tribal_government"))) ||
            (kind == CONVERSION_RELIGION && (!owner.has_independent ||
                                              owner.independent.holder != player->id))) {
            debug_console_output(&menu->console, "CK2 council mission: must be eligible owned/vassal province (Settle Tribe requires tribal ruler)." );
            return;
        }
        const char *target = kind == CONVERSION_RELIGION ? player->religion_key : player->culture_key;
        status = conversion_mission_assign(&menu->conversion, &menu->conversion_catalog,
                                           kind, id, player->id, target, current);
    } else if (resolve) {
        status = conversion_mission_complete(&menu->conversion, &menu->conversion_catalog,
                                             kind, current);
    }
    debug_console_output(&menu->console, conversion_status_name(status));
    if (status == CONVERSION_OK && !mission) {
        conversion_replay_province(menu, id);
        if (menu->selected && menu->selected->id == id) {
            conversion_selected_labels(menu);
            refresh_holding_style(menu);
        }
        conversion_map_refresh(menu);
    } else if (status == CONVERSION_OK && mission)
        debug_console_output(&menu->console, "Mission staged; CK2 event logic unavailable. Use resolvefaith / resolveculture for a developer effect.");
}

/* Searches mounted CK2 base/DLC/mod event scripts; no bundled CK2 event data.
 * Intentional on demand: an absent events/ directory cannot crash startup. */
static bool scripted_event_open(struct menu *menu, const char *arguments)
{
    char event_id[96]={0}, target_text[48]={0}, trailing[2]={0};
    const int fields=arguments ? sscanf(arguments,"%95s %47s %1s",event_id,target_text,trailing) : 0;
    if (fields<1 || fields>2 || !ck2_event_valid_id(event_id)) {
        debug_console_output(&menu->console,"Usage: event <id> [target id] (e.g. event 900 or event WoL.1)");
        return false;
    }
    unsigned target_id=0;
    if (fields==2) {
        errno=0;
        char *end=NULL;
        const unsigned long parsed=strtoul(target_text,&end,10);
        if (errno || !end || *end || !parsed || parsed>UINT_MAX) {
            debug_console_output(&menu->console,"Invalid event target ID.");
            return false;
        }
        target_id=(unsigned)parsed;
    }
    struct ck2_asset_files files=ck2_list_assets(menu->asset_root,"events");
    if (!files.count) {
        debug_console_output(&menu->console,"No events/ files mounted. Select your CK2 installation/DLC in Content Settings.");
        ck2_free_asset_files(&files);
        return false;
    }
    struct ck2_event match={0};
    bool found=false;
    for (size_t i=0;i<files.count && !found;i++) {
        const char *name=files.names[i];
        const size_t n=strlen(name);
        if (n<4 || strcmp(name+n-4,".txt")) continue;
        char *script=load_from_directory(menu->asset_root,"events",name);
        if (!script) continue;
        char origin[192];
        snprintf(origin,sizeof origin,"events/%.177s",name);
        found=ck2_event_find_in_text(script,event_id,origin,&match)==1;
        SDL_free(script);
    }
    ck2_free_asset_files(&files);
    if (!found) {
        char message[180];
        snprintf(message,sizeof message,"Event %.95s not found in mounted events/ scripts.",event_id);
        debug_console_output(&menu->console,message);
        return false;
    }
    if (match.kind==CK2_EVENT_CHARACTER) {
        if (!menu->campaign_ruler_id || (target_id && target_id!=menu->campaign_ruler_id)) {
            debug_console_output(&menu->console,"Only the playable ruler is supported as a character-event target.");
            return false;
        }
        target_id=menu->campaign_ruler_id;
    } else {
        if (!target_id && menu->selected) target_id=menu->selected->id;
        if (!target_id || !province_by_id(&menu->provinces,target_id)) {
            debug_console_output(&menu->console,"Select a province or supply a valid province ID: event <id> <provinceID>.");
            return false;
        }
    }
    menu->event_target_id=target_id;
    menu->pending_event=match;
    menu->event_open=true;
    menu->console.open=false;
    SDL_StopTextInput();
    map_gesture_cancel(&menu->map_gesture);
    tree_view_close(&menu->genealogy);
    menu->technology.open=false;
    char message[220];
    snprintf(message,sizeof message,"Fired %.80s (%s) from %.90s. Choose an option; triggers bypassed.",
             match.id,match.kind==CK2_EVENT_CHARACTER?"character":"province",match.source);
    debug_console_output(&menu->console,message);
    if (match.has_immediate)
        debug_console_output(&menu->console,"Note: immediate script block is not executed.");
    if (match.truncated_options)
        debug_console_output(&menu->console,"Warning: this event has more than 8 options; remaining choices are not available yet.");
    return true;
}

static void scripted_event_choose(struct menu *menu, unsigned selected)
{
    if (!menu->event_open) {
        debug_console_output(&menu->console,"No fired event is pending. Try event <id>.");
        return;
    }
    if (!selected || selected>menu->pending_event.option_count) {
        debug_console_output(&menu->console,"Invalid option number for this event.");
        return;
    }
    const struct ck2_event_option *option=&menu->pending_event.options[selected-1];
    const struct ck2_event_deltas delta=ck2_event_numeric_deltas(option);
    /* A province event is scoped to the province; numerical character effects
     * cannot be assigned safely without interpreting a nested character scope. */
    unsigned unsupported=option->ignored_effects;
    if (menu->pending_event.kind==CK2_EVENT_CHARACTER) {
        double gold=(double)menu->campaign.treasury+menu->campaign.gold_remainder+delta.wealth;
        if (gold<0) gold=0;
        if (gold>UINT_MAX) gold=UINT_MAX;
        menu->campaign.treasury=(unsigned)gold;
        menu->campaign.gold_remainder=gold-(double)menu->campaign.treasury;
        menu->campaign.prestige+=delta.prestige;
        menu->campaign.piety+=delta.piety;
    } else {
        for (size_t i=0;i<option->effect_count;i++) {
            enum ck2_event_effect_kind kind=option->effects[i].kind;
            if (kind==CK2_EVENT_WEALTH || kind==CK2_EVENT_PRESTIGE || kind==CK2_EVENT_PIETY)
                ++unsupported;
        }
    }
    for (size_t i=0;i<option->effect_count;i++) {
        const struct ck2_event_effect *effect=&option->effects[i];
        if (effect->kind!=CK2_EVENT_CULTURE && effect->kind!=CK2_EVENT_RELIGION) continue;
        enum conversion_kind kind=effect->kind==CK2_EVENT_CULTURE?CONVERSION_CULTURE:CONVERSION_RELIGION;
        enum conversion_status status=CONVERSION_INVALID_TARGET;
        const struct province *target=menu->pending_event.kind==CK2_EVENT_PROVINCE ?
            province_by_id(&menu->provinces,menu->event_target_id) : NULL;
        if (target) {
            const size_t index=(size_t)(target-menu->provinces.provinces);
            const struct map_province_facts *facts=index<menu->settlement_scene.province_fact_count?
                &menu->settlement_scene.province_facts[index]:NULL;
            const char *original=facts?(kind==CONVERSION_RELIGION?facts->religion:facts->culture):"";
            status=conversion_province_set(&menu->conversion,&menu->conversion_catalog,
                                            target->id,kind,effect->value,original);
            if (status==CONVERSION_OK) {
                conversion_replay_province(menu,target->id);
                if (menu->selected==target) {
                    conversion_selected_labels(menu);
                    refresh_holding_style(menu);
                }
                conversion_map_refresh(menu);
            }
        } else if (menu->pending_event.kind==CK2_EVENT_CHARACTER) {
            const struct ruler_profile *person=realm_ruler(&menu->realms,menu->campaign_ruler_id);
            if (person) {
                const char *original=kind==CONVERSION_RELIGION?person->religion_key:person->culture_key;
                status=conversion_person_set(&menu->conversion,&menu->conversion_catalog,
                                              person->id,kind,effect->value,original);
                if (status==CONVERSION_OK) {
                    conversion_replay_rulers(menu);
                    conversion_map_refresh(menu);
                    menu->character.ruler.id=0;
                    if (menu->character.open) inspect_campaign_character(menu,
                        realm_ruler(&menu->realms,menu->campaign_ruler_id));
                }
            }
        }
        if (status!=CONVERSION_OK && status!=CONVERSION_ALREADY_SAME) ++unsupported;
    }
    char message[220];
    snprintf(message,sizeof message,"Event %.65s option %u: direct effects handled. %u unsupported/invalid effect(s)%s.",
        menu->pending_event.id,selected,unsupported,option->conditional?"; conditional trigger not checked":"");
    debug_console_output(&menu->console,message);
    if (menu->pending_event.has_immediate)
        debug_console_output(&menu->console,"Caution: immediate event effects were not executed.");
    menu->event_open=false;
}

static bool parse_uint_arg(const char *text, unsigned *id)
{
    if (!text || !*text || !id) return false;
    char *end;
    errno = 0;
    unsigned long n = strtoul(text, &end, 10);
    while (*end == ' ') ++end;
    if (errno || *end || n == 0 || n > UINT_MAX) return false;
    *id = (unsigned)n;
    return true;
}

static void warfare_console_command(struct menu *menu, struct debug_console_result command)
{
    char message[220] = {0};
    unsigned first = 0, second = 0;
    const struct province *province = NULL;
    if (command.action == DEBUG_WAR_LIST) {
        snprintf(message, sizeof message, "%zu wars; %zu armies; %zu occupied provinces",
                 menu->warfare.war_count, menu->warfare.army_count, menu->warfare.occupation_count);
        debug_console_output(&menu->console, message);
        for (size_t i = 0; i < menu->warfare.army_count; ++i) {
            const struct warfare_army *a = menu->warfare.armies + i;
            snprintf(message, sizeof message, "Army %u: ruler %u, province %u, %u troops%s",
                     a->id, a->ruler_id, a->province_id, a->troops,
                     a->route ? " (moving)" : a->siege_days ? " (sieging)" : "");
            debug_console_output(&menu->console, message);
        }
        for (size_t i = 0; i < menu->warfare.war_count; ++i) {
            const struct warfare_war *w = menu->warfare.wars + i;
            if (!w->active) continue;
            snprintf(message, sizeof message, "WAR: ruler %u vs %u", w->attacker, w->defender);
            debug_console_output(&menu->console, message);
        }
        return;
    }
    if (command.action == DEBUG_WAR_MARCH) {
        char extra;
        if (sscanf(command.argument, "%u %u %c", &first, &second, &extra) != 2 || !first || !second) {
            debug_console_output(&menu->console, "Usage: march <army ID> <destination province ID>");
            return;
        }
        if (!province_by_id(&menu->provinces, second)) {
            debug_console_output(&menu->console, "Destination must be a valid land province.");
            return;
        }
        const struct warfare_army *a = warfare_army(&menu->warfare, first);
        if (!a || a->ruler_id != menu->campaign_ruler_id) {
            debug_console_output(&menu->console, "Only your own armies can receive march orders.");
            return;
        }
        if (!menu->warfare_graph.head &&
            !warfare_graph_from_map(&menu->warfare_graph, &menu->provinces)) {
            debug_console_output(&menu->console, "Couldn't build land adjacency from CK2 provinces.bmp.");
            return;
        }
        enum warfare_status status = warfare_march(&menu->warfare, &menu->warfare_graph, first, second);
        snprintf(message, sizeof message, "March army %u -> %u: %s", first, second,
                 warfare_status_text(status));
    } else if (command.action == DEBUG_WAR_DISBAND) {
        if (!parse_uint_arg(command.argument, &first)) {
            debug_console_output(&menu->console, "Usage: disband <army ID>"); return;
        }
        struct warfare_army *a = warfare_army(&menu->warfare, first);
        enum warfare_status status = a && a->ruler_id == menu->campaign_ruler_id ?
            warfare_disband(&menu->warfare, first) : WARFARE_NO_ARMY;
        snprintf(message, sizeof message, "Disband army %u: %s", first, warfare_status_text(status));
    } else if (command.action == DEBUG_WAR_DECLARE || command.action == DEBUG_WAR_PEACE) {
        if (!parse_uint_arg(command.argument, &first) || first == menu->campaign_ruler_id) {
            debug_console_output(&menu->console, "Usage: declarewar/peace <other ruler ID>"); return;
        }
        if (command.action == DEBUG_WAR_DECLARE && !realm_ruler(&menu->realms, first)) {
            debug_console_output(&menu->console, "Target ruler not found in dated title history."); return;
        }
        const enum warfare_status status = command.action == DEBUG_WAR_DECLARE ?
            warfare_declare(&menu->warfare, menu->campaign_ruler_id, first) :
            warfare_peace(&menu->warfare, menu->campaign_ruler_id, first);
        snprintf(message, sizeof message, "%s ruler %u: %s%s",
                 command.action == DEBUG_WAR_DECLARE ? "Declare war on" : "Peace with", first,
                 warfare_status_text(status), status == WARFARE_OK ?
                 " (prototype: no CB, peace demands or title transfer)" : "");
    } else if (command.action == DEBUG_WAR_RAISE || command.action == DEBUG_WAR_RAISE_ENEMY) {
        if (*command.argument) {
            if (!parse_uint_arg(command.argument, &first)) {
                debug_console_output(&menu->console, "Usage: raise [province ID] / raiseenemy <province ID>"); return;
            }
            province = province_by_id(&menu->provinces, first);
        } else province = menu->selected;
        if (!province) {
            debug_console_output(&menu->console, "Select a land province or supply its numeric ID."); return;
        }
        struct realm_province_ownership owners;
        if (!realm_province_ownership(&menu->realms,
                    (size_t)(province - menu->provinces.provinces), &owners) ||
            !owners.has_county || !owners.has_independent) {
            debug_console_output(&menu->console, "Title history has no known county and realm holder."); return;
        }
        const unsigned ruler = command.action == DEBUG_WAR_RAISE ?
            menu->campaign_ruler_id : owners.independent.holder;
        if (command.action == DEBUG_WAR_RAISE && owners.county.holder != ruler) {
            debug_console_output(&menu->console, "You can only raise directly held counties in this version."); return;
        }
        if (command.action == DEBUG_WAR_RAISE_ENEMY &&
            !warfare_hostile(&menu->warfare, menu->campaign_ruler_id, ruler)) {
            debug_console_output(&menu->console, "Enemy independent ruler must be at war with you."); return;
        }
        struct county_holdings county = {0};
        holdings_load(&county, menu->asset_root, province, menu->start_date);
        construction_settlement_apply(&menu->construction, province->id, &county);
        double troops = 0;
        bool known = false;
        for (size_t h = 0; h < county.count; ++h) {
            struct levy_potential v;
            if (construction_holding_levy(&menu->construction, county.slots + h, &v)) {
                troops += v.total;
                known = true;
            }
        }
        if (!known || troops < 1) {
            debug_console_output(&menu->console, "No troop definitions available for this county."); return;
        }
        unsigned army_id = 0;
        unsigned amount = troops > UINT_MAX ? UINT_MAX : (unsigned)troops;
        enum warfare_status status = warfare_raise(&menu->warfare, ruler, province->id, amount, &army_id);
        snprintf(message, sizeof message, "Raise from %u (ruler %u): %s; army %u, %u potential troops",
                 province->id, ruler, warfare_status_text(status), army_id, amount);
    }
    if (message[0]) debug_console_output(&menu->console, message);
}

/* Console-selected hereditary laws use candidate records from CK2 bookmark
 * character history plus explicitly born runtime children. Unknown people,
 * election votes and gender permissions must not be guessed. */
struct succession_preview {
    struct ruler_profile *profiles;
    struct ck2_heir_candidate *candidates;
    size_t count;
};
static void succession_preview_free(struct succession_preview *p)
{
    free(p->profiles);free(p->candidates);*p=(struct succession_preview){0};
}
static bool succession_preview_build(struct menu *menu,const struct ruler_profile *dead,
                                     struct succession_preview *preview)
{
    enum { LIMIT=256 };
    unsigned history_children[LIMIT]={0},history_siblings[LIMIT]={0};
    if (!menu || !dead || !preview) return false;
    rulers_family_ids(menu->asset_root,dead,menu->campaign.date,
                      history_children,LIMIT,history_siblings,LIMIT);
    unsigned runtime_ids[LIMIT]={0};
    const size_t runtime_count=family_gameplay_children(&menu->family,dead->id,runtime_ids,LIMIT);
    unsigned ids[LIMIT]={0};size_t total=0;
    for (size_t group=0;group<3;++group) {
        const unsigned *source=group==0 ? history_children : group==1 ? runtime_ids : history_siblings;
        const size_t limit=group==1 ? (runtime_count<LIMIT ? runtime_count:LIMIT) : LIMIT;
        for (size_t j=0;j<limit;++j) {
            const unsigned id=source[j];if (!id || id==dead->id) continue;
            bool seen=false;
            for (size_t k=0;k<total;++k) seen|=ids[k]==id;
            if (!seen && total<LIMIT) ids[total++]=id;
        }
    }
    if (!total) return true;
    preview->profiles=calloc(total,sizeof(*preview->profiles));
    preview->candidates=calloc(total,sizeof(*preview->candidates));
    if (!preview->profiles || !preview->candidates) {
        succession_preview_free(preview);return false;
    }
    for (size_t i=0;i<total;++i) preview->profiles[i].id=ids[i];
    rulers_load(preview->profiles,total,menu->asset_root,menu->campaign.date);
    for (size_t i=0;i<total;++i) {
        const struct ruler_profile *r=family_gameplay_find(&menu->family,ids[i]);
        if (!r) r=realm_ruler(&menu->realms,ids[i]);
        if (r) preview->profiles[i]=*r;
        r=&preview->profiles[i];
        preview->candidates[i]=(struct ck2_heir_candidate){
            .id=r->id,.father=r->father,.mother=r->mother,.dynasty_id=r->dynasty_id,
            .age=r->age,.age_known=r->age_known,.female=r->female,.alive=!r->deceased};
    }
    preview->count=total;
    return true;
}
static size_t succession_order(struct menu *menu,const struct ruler_profile *dead,
                              struct succession_preview *preview,unsigned *heirs,size_t max)
{
    const struct ck2_law *inheritance=ck2_law_selected(&menu->active_laws,&menu->law_catalog,"succession");
    const struct ck2_law *gender=ck2_law_selected(&menu->active_laws,&menu->law_catalog,"gender_succession");
    if (!inheritance || !gender || !dead) return 0;
    if (!succession_preview_build(menu,dead,preview)) return 0;
    return ck2_succession_divide(ck2_law_succession_mode(inheritance),
        ck2_law_gender_mode(gender),dead->id,dead->dynasty_id,
        preview->candidates,preview->count,heirs,max);
}
static bool succession_transfer(struct menu *menu,struct ruler_profile dead)
{
    const struct government_definition *gov=government_find(
        &menu->settlement_scene.governments,dead.government_key);
    if (!government_uses_hereditary_succession(gov)) {
        debug_console_output(&menu->console,
            "Government succession is non-hereditary: patrician elections / clerical appointment not yet simulated; titles left unchanged.");
        return false;
    }
    struct succession_preview preview={0};
    unsigned heirs[64]={0};
    const size_t nh=succession_order(menu,&dead,&preview,heirs,SDL_arraysize(heirs));
    if (!nh) {
        debug_console_output(&menu->console,"Succession unresolved: set a supported succession and gender law, with an eligible heir.");
        succession_preview_free(&preview);return false;
    }
    const size_t nt=realm_held_title_count(&menu->realms,dead.id);
    if (!nt || nt>10000) {
        debug_console_output(&menu->console,"Succession halted: title count is unavailable or exceeds safety limit.");
        succession_preview_free(&preview);return false;
    }
    /* Check before mutation that every heir can be represented in realm history. */
    bool okay=true;
    for (size_t i=0;i<nh && okay;++i) {
        const struct ruler_profile *heir=realm_ruler(&menu->realms,heirs[i]);
        if (!heir) {
            for (size_t j=0;j<preview.count;++j)
                if (preview.profiles[j].id==heirs[i]) {
                    okay=realm_register_runtime_ruler(&menu->realms,&preview.profiles[j]);break;
                }
        }
        okay=okay && realm_ruler(&menu->realms,heirs[i])!=NULL;
    }
    if (!okay) {succession_preview_free(&preview);return false;}
    const size_t transferred=realm_runtime_inherit_titles(&menu->realms,dead.id,heirs,nh);
    if (transferred==nt) {
        char line[200];
        snprintf(line,sizeof line,"Succession: %zu titles inherited by %zu heir(s); player now character %u.",
                 transferred,nh,heirs[0]);
        debug_console_output(&menu->console,line);
        menu->campaign_ruler_id=heirs[0];
        menu->campaign_ruler=NULL;
        menu->character.ruler.id=0;
        const struct ruler_profile *new_ruler=realm_ruler(&menu->realms,heirs[0]);
        if (new_ruler) {
            SDL_DestroyTexture(menu->campaign_portrait);
            menu->campaign_portrait=setup_character_portrait(menu,new_ruler);
        }
        struct realm_title_info principal;
        if (realm_primary_title(&menu->realms,heirs[0],&principal)) {
            SDL_DestroyTexture(menu->campaign_title_shield);
            menu->campaign_title_shield=era_title_shield(menu->renderer,menu->asset_root,
                                    principal.id,new_ruler ? new_ruler->religion_key : "");
            if (province_by_id(&menu->provinces,principal.capital))
                menu->home_province=principal.capital;
        }
        (void)government_refresh_player(menu);
        if (economy_snapshot_build(&menu->economy,menu->asset_root,&menu->provinces,
            &menu->realms,&menu->construction,heirs[0],menu->start_date)==0) {
            refresh_law_income(menu);
            menu->campaign.demesne_count=menu->economy.direct_count;
            menu->campaign.monthly_prestige=menu->economy.monthly_prestige;
            menu->campaign.monthly_piety=menu->economy.monthly_piety;
        }
        if (menu->realms_visible && realm_map_build(&menu->realms,menu->renderer,
                menu->asset_root,&menu->provinces,&menu->campaign_map,menu->start_date)<0)
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,"Realm graphics refresh after succession failed");
        conversion_map_refresh(menu);
        if (menu->selected) {
            const struct province *selected=menu->selected;
            menu->selected=NULL;
            select_province(menu,selected);
        }
    } else debug_console_output(&menu->console,"Succession incomplete; title ownership requires review.");
    succession_preview_free(&preview);
    return transferred==nt;
}
static void law_console_command(struct menu *menu,struct debug_console_result result)
{
    char line[220];
    if (result.action==DEBUG_LAW_LIST) {
        if (result.argument[0]) {
            size_t found=0;
            for (size_t i=0;i<menu->law_catalog.count && found<9;++i) {
                const struct ck2_law *law=&menu->law_catalog.items[i];
                if (strcmp(law->group,result.argument)) continue;
                snprintf(line,sizeof line,"%s%s",law->key,
                    ck2_law_selected(&menu->active_laws,&menu->law_catalog,law->group)==law ? " [selected]":"");
                debug_console_output(&menu->console,line);++found;
            }
            if (!found) debug_console_output(&menu->console,"Unknown group or no authored law entries.");
        } else {
            debug_console_output(&menu->console,"Laws: use 'laws <group>' then 'setlaw <key>' (DEV override; not a CK2 council vote).");
            static const char *const groups[]={"succession","gender_succession","feudal_tax","city_tax",
                "temple_tax","tax_levy_law_feudal","tax_levy_law_republic","tax_levy_law_theocracy",
                "crown_authority","centralization"};
            for (size_t i=0;i<SDL_arraysize(groups);++i) {
                const struct ck2_law *law=ck2_law_selected(&menu->active_laws,&menu->law_catalog,groups[i]);
                snprintf(line,sizeof line,"%s: %s",groups[i],law ? law->key : "unresolved (historical law not loaded)");
                debug_console_output(&menu->console,line);
            }
        }
    } else if (result.action==DEBUG_LAW_SET) {
        const struct ck2_law *law=ck2_law_find(&menu->law_catalog,result.argument);
        const struct ruler_profile *r=realm_ruler(&menu->realms,menu->campaign_ruler_id);
        const struct government_definition *gov=r ? government_find(
            &menu->settlement_scene.governments,r->government_key) : NULL;
        if (law && !government_allows_law_group(gov,law->group)) {
            debug_console_output(&menu->console,
                "This government forbids that law family; election/clerical succession is not yet implemented.");
            return;
        }
        if (law && ck2_law_select(&menu->active_laws,&menu->law_catalog,result.argument)) {
            if (government_refresh_player(menu)) {
                if (menu->map_modes.selected==MAP_MODE_GOVERNMENTS) conversion_map_refresh(menu);
                if (menu->character.open) {
                    const struct ruler_profile *ruler=realm_ruler(&menu->realms,menu->character.ruler.id);
                    menu->character.ruler.id=0; /* Force cached panel to rebind government. */
                    if (ruler) inspect_campaign_character(menu,ruler);
                }
            }
            refresh_law_income(menu);
            snprintf(line,sizeof line,"Selected %s (%s). Developer override; CK2 potential/allow not evaluated.",law->key,law->group);
        } else snprintf(line,sizeof line,"No CK2 law named '%.85s' in installed law files.",result.argument);
        debug_console_output(&menu->console,line);
    } else if (result.action==DEBUG_LAW_TAXES) {
        snprintf(line,sizeof line,"Direct %.2f + modeled vassal %.2f = %.2f gold/year (%.2f/month).",
            menu->economy.annual_direct_tax,menu->economy.annual_law_vassal_tax,
            menu->economy.annual_direct_tax+menu->economy.annual_law_vassal_tax,
            menu->campaign.monthly_tax);
        debug_console_output(&menu->console,line);
        snprintf(line,sizeof line,"Vassal holdings %zu, unresolved tax %u; excludes exemptions and opinion modifiers.",
            menu->economy.vassal_count,menu->economy.vassal_tax_skipped);
        debug_console_output(&menu->console,line);
    } else if (result.action==DEBUG_LAW_HEIR) {
        const struct ruler_profile *dead=family_gameplay_find(&menu->family,menu->campaign_ruler_id);
        if (!dead) dead=realm_ruler(&menu->realms,menu->campaign_ruler_id);
        struct succession_preview preview={0};unsigned heirs[32]={0};
        const size_t n=succession_order(menu,dead,&preview,heirs,SDL_arraysize(heirs));
        if (!n) debug_console_output(&menu->console,"No verified heir under selected hereditary and gender laws (or laws unset).");
        else {
            snprintf(line,sizeof line,"Projected first heir: %u; %zu eligible direct heirs (preview, not CK2 voting).",heirs[0],n);
            debug_console_output(&menu->console,line);
        }
        succession_preview_free(&preview);
    }
}

/* Script-based election diagnostics: reveal the loaded CK2 voting rule, never
 * substitute ordinary hereditary heirs or fabricate a winning ballot. */
static void election_console_command(struct menu *menu,struct debug_console_result command)
{
    char line[220];
    const struct ck2_law *law=ck2_law_selected(&menu->active_laws,
                                               &menu->law_catalog,"succession");
    if (!law) {
        debug_console_output(&menu->console,"Succession law unresolved: no dated law or developer selection.");
        return;
    }
    if (!law->election_voting_rules[0]) {
        snprintf(line,sizeof line,"%s has no indexed elective voting_rules assignment.",law->key);
        debug_console_output(&menu->console,line);
        return;
    }
    const struct ck2_election_rule *rule=ck2_election_find(&menu->election_catalog,
                                                             law->election_voting_rules);
    if (!rule) {
        snprintf(line,sizeof line,"Missing CK2 succession_voting script: %s",law->election_voting_rules);
        debug_console_output(&menu->console,line);
        return;
    }
    if (command.action==DEBUG_ELECTION_CANDIDATE) {
        unsigned id=0;
        if (!parse_uint_arg(command.argument,&id) || !id) {
            debug_console_output(&menu->console,"Usage: electioncandidate <character ID>");return;
        }
        const struct ruler_profile *r=family_gameplay_find(&menu->family,id);
        if (!r) r=realm_ruler(&menu->realms,id);
        struct ruler_profile history={0};
        if (!r) {
            history.id=id;rulers_load(&history,1,menu->asset_root,menu->campaign.date);
            if (history.name[0]) r=&history;
        }
        if (!r) {
            debug_console_output(&menu->console,"Unknown character. No verified eligibility.");return;
        }
        const unsigned holder=menu->campaign_ruler_id;
        if (r->deceased) {
            snprintf(line,sizeof line,"Character %u is dead; cannot stand for election.",id);
        } else {
            const struct ruler_profile *current=realm_ruler(&menu->realms,holder);
            const char *candidate_traits[RULER_TRAIT_LIMIT]={0};
            const char *holder_traits[RULER_TRAIT_LIMIT]={0};
            for (size_t i=0;i<r->trait_count && i<RULER_TRAIT_LIMIT;++i)
                candidate_traits[i]=r->traits[i];
            if (current) for (size_t i=0;i<current->trait_count && i<RULER_TRAIT_LIMIT;++i)
                holder_traits[i]=current->traits[i];
            struct ck2_election_candidate ctx={
                .ruler_id=holder,.candidate_id=id,
                .ruler_traits=holder_traits,.ruler_trait_count=current ? current->trait_count : 0,
                .candidate_traits=candidate_traits,.candidate_trait_count=r->trait_count
            };
            const enum ck2_election_truth verdict=ck2_election_candidate_trigger(rule,&ctx);
            if (verdict==CK2_ELECTION_FALSE)
                snprintf(line,sizeof line,"Character %u (%s): REJECTED by a proven CK2 candidate-trigger condition.",
                         id,r->name);
            else snprintf(line,sizeof line,"Character %u (%s): CK2 election eligibility UNRESOLVED.",id,r->name);
        }
        debug_console_output(&menu->console,line);
        debug_console_output(&menu->console,"CK2 checks claims/kinship, council/army offices, injuries, realm, religion and other triggers.");
        return;
    }
    snprintf(line,sizeof line,"%s: base_type=%s; voting_rules=%s",
             law->key,law->election_base_type[0] ? law->election_base_type : "unspecified",
             rule->id);
    debug_console_output(&menu->console,line);
    static const char *const names[]={"Elector selection","Elector vote strength",
                                       "Candidate preference","Candidate eligibility"};
    for (int i=0;i<CK2_ELECTION_SECTION_COUNT;++i) {
        if (!rule->script[i]) {
            snprintf(line,sizeof line,"%s: absent in installed CK2 scripts",names[i]);
        } else if (rule->has_base_days[i]) {
            snprintf(line,sizeof line,"%s: authored base %d, %u modifiers (not evaluated)",
                     names[i],rule->base_days[i],rule->modifier_count[i]);
        } else {
            snprintf(line,sizeof line,"%s: script loaded, %u modifiers; dynamic conditions unresolved",
                     names[i],rule->modifier_count[i]);
        }
        debug_console_output(&menu->console,line);
    }
    if (rule->has_max_electors) {
        snprintf(line,sizeof line,"Authored maximum electors: %d",rule->max_electors);
        debug_console_output(&menu->console,line);
    }
    char offices[24][64]={{0}};
    size_t office_count=ck2_election_office_keys(rule,offices,SDL_arraysize(offices));
    if (office_count) {
        snprintf(line,sizeof line,"Original voter offices: %zu job/minor-title types (not assigned yet).",office_count);
        debug_console_output(&menu->console,line);
        for (size_t i=0;i<office_count && i<SDL_arraysize(offices);i+=4) {
            size_t at=0;
            line[0]=0;
            for (size_t j=i;j<office_count && j<i+4 && j<SDL_arraysize(offices);++j) {
                const int n=snprintf(line+at,sizeof line-at,"%s%s",j==i ? "" : ", ",offices[j]);
                if (n<0 || (size_t)n>=sizeof line-at) break;
                at+=(size_t)n;
            }
            debug_console_output(&menu->console,line);
        }
    }
    debug_console_output(&menu->console,"No elected successor calculated: votes, council offices and eligibility require script evaluation.");
}

/* Character lifecycle is separate from historic title grants and succession. */
static void character_lifecycle_command(struct menu *menu, struct debug_console_result command)
{
    const struct ruler_profile *original = realm_ruler(&menu->realms, menu->campaign_ruler_id);
    const struct ruler_profile *player = original ?
        family_gameplay_find(&menu->family, original->id) : NULL;
    if (!player) player = original;
    if (!player) { debug_console_output(&menu->console, "No playable character."); return; }
    if (command.action == DEBUG_CHAR_DEATH) {
        unsigned target_id;
        if (!parse_uint_arg(command.argument, &target_id)) {
            debug_console_output(&menu->console, "Usage: die <character ID>"); return;
        }
        const struct ruler_profile *target = family_gameplay_find(&menu->family, target_id);
        if (!target) target = realm_ruler(&menu->realms, target_id);
        struct ruler_profile other = {0};
        if (!target) {
            other.id = target_id;
            rulers_load(&other, 1, menu->asset_root, menu->start_date);
            if (other.name[0]) target = &other;
        }
        /* A death may realloc family.people; never retain a pointer into it. */
        const struct ruler_profile deceased_before_death = *player;
        enum family_status status = target ? family_gameplay_death(&menu->family, target, menu->campaign.date)
                                           : FAMILY_NO_PERSON;
        debug_console_output(&menu->console, family_status_description(status));
        if (status == FAMILY_OK) {
            if (menu->character.open) {
                menu->character.ruler.id = 0;
                inspect_campaign_character(menu, realm_ruler(&menu->realms, menu->campaign_ruler_id));
            }
            if (target_id == menu->campaign_ruler_id) {
                if (!succession_transfer(menu,deceased_before_death))
                    debug_console_output(&menu->console,"Ruler dead; titles remain unchanged until inheritance is supported.");
            } else debug_console_output(&menu->console,"Non-player death recorded; AI title succession remains unresolved.");
        }
        return;
    }
    unsigned spouse_id = player->spouse;
    const struct ruler_profile *spouse = family_gameplay_find(&menu->family, spouse_id);
    if (!spouse) spouse = realm_ruler(&menu->realms, spouse_id);
    struct ruler_profile unlanded = {0};
    if (!spouse && spouse_id) {
        unlanded.id = spouse_id;
        rulers_load(&unlanded, 1, menu->asset_root, menu->start_date);
        if (unlanded.name[0]) spouse = &unlanded;
    }
    enum family_status status = FAMILY_NOT_PARTNERS;
    unsigned newborn = 0;
    if (spouse) {
        if (!family_gameplay_find(&menu->family, player->id) ||
            !family_gameplay_find(&menu->family, spouse->id))
            status = family_gameplay_import_marriage(&menu->family, player, spouse);
        else status = FAMILY_OK;
        if (status == FAMILY_OK)
            status = family_gameplay_birth(&menu->family, player, spouse,
                                           menu->campaign.date, &newborn);
    }
    char line[200];
    snprintf(line, sizeof line, "Birth: %s (new character ID %u)",
             family_status_description(status), newborn);
    debug_console_output(&menu->console, line);
    if (status == FAMILY_OK) name_runtime_newborn(menu, newborn);
    if (status == FAMILY_OK && menu->character.open) {
        menu->character.ruler.id = 0;
        inspect_campaign_character(menu, original);
    }
}

/* The native checkpoint is deliberately restricted to the currently loaded
 * bookmark. It is not CK2 save-file interoperability and cannot bootstrap
 * a different historical setup from the main menu. */
static bool checkpoint_slot_path(char *path, size_t capacity, const char *raw)
{
    const char *slot = raw && *raw ? raw : "quick";
    const size_t length = strlen(slot);
    if (!length || length > 32) return false;
    for (size_t i = 0; i < length; ++i)
        if (!isalnum((unsigned char)slot[i]) && slot[i] != '-' && slot[i] != '_')
            return false;
    char *prefs = SDL_GetPrefPath("OpenCK2", "OpenCK2");
    if (!prefs) return false;
    const int written = snprintf(path, capacity, "%s%s.ocks", prefs, slot);
    SDL_free(prefs);
    return written > 0 && (size_t)written < capacity;
}

static bool checkpoint_is_compatible(const struct menu *menu,
                                     const struct openck2_checkpoint *state,
                                     char *reason, size_t capacity)
{
    if (state->bookmark.year != menu->start_date.year ||
        state->bookmark.month != menu->start_date.month ||
        state->bookmark.day != menu->start_date.day ||
        state->bookmark_ruler != menu->campaign_start_ruler_id) {
        snprintf(reason, capacity, "Save belongs to another bookmark or starting ruler.");
        return false;
    }
    if (!construction_checkpoint_valid(&menu->construction, &state->construction)) {
        snprintf(reason, capacity, "Save references missing or incompatible building definitions.");
        return false;
    }
    for (size_t i = 0; i < state->laws.count; ++i) {
        const struct ck2_active_law *entry = &state->laws.chosen[i];
        const struct ck2_law *law = ck2_law_find(&menu->law_catalog, entry->key);
        if (!law || strcmp(entry->group, law->group)) {
            snprintf(reason, capacity, "Save references a missing or reassigned law.");
            return false;
        }
    }
    for (size_t i = 0; i < state->title_count; ++i) {
        const struct realm_title_override *entry = state->titles + i;
        struct realm_title_info info;
        if (!realm_title_info(&menu->realms, entry->id, &info) ||
            (entry->owner && !realm_ruler(&menu->realms, entry->owner) &&
             !family_gameplay_find(&state->family, entry->owner))) {
            snprintf(reason, capacity, "Save references an unknown title or heir.");
            return false;
        }
    }
    if (!realm_ruler(&menu->realms, state->player) &&
        !family_gameplay_find(&state->family, state->player)) {
        snprintf(reason, capacity, "Save references an unknown playable character.");
        return false;
    }
    if (state->home_province && !province_by_id(&menu->provinces, state->home_province)) {
        snprintf(reason, capacity, "Saved home province is absent from this map.");
        return false;
    }
    for (size_t i = 0; i < state->conversion.province_count; ++i) {
        const struct conversion_province *p = &state->conversion.provinces[i];
        if (!province_by_id(&menu->provinces, p->id) ||
            (p->has_religion && !conversion_catalog_has(&menu->conversion_catalog, CONVERSION_RELIGION, p->religion)) ||
            (p->has_culture && !conversion_catalog_has(&menu->conversion_catalog, CONVERSION_CULTURE, p->culture))) {
            snprintf(reason, capacity, "Saved province conversion is incompatible with mounted CK2 files.");
            return false;
        }
    }
    for (size_t i = 0; i < state->conversion.person_count; ++i) {
        const struct conversion_person *p = &state->conversion.people[i];
        if ((p->has_religion && !conversion_catalog_has(&menu->conversion_catalog, CONVERSION_RELIGION, p->religion)) ||
            (p->has_culture && !conversion_catalog_has(&menu->conversion_catalog, CONVERSION_CULTURE, p->culture))) {
            snprintf(reason, capacity, "Saved character conversion uses unavailable definitions.");
            return false;
        }
    }
    for (size_t i = 0; i < state->warfare.army_count; ++i) {
        const struct warfare_army *army = state->warfare.armies + i;
        if (!province_by_id(&menu->provinces, army->province_id) ||
            (army->target && !province_by_id(&menu->provinces, army->target))) {
            snprintf(reason, capacity, "Saved army references a missing province.");
            return false;
        }
        for (size_t j = 0; j < army->route_count; ++j) {
            if (!province_by_id(&menu->provinces, army->route[j]) ||
                (j && !warfare_graph_adjacent(&menu->warfare_graph,
                                              army->route[j-1], army->route[j]))) {
                snprintf(reason, capacity, "Saved army route is incompatible with this map.");
                return false;
            }
        }
    }
    if (!ck2_government_mechanics_valid(&state->governments)) {
        snprintf(reason,capacity,"Saved government mechanics are malformed.");
        return false;
    }
    for (size_t i=0;i<state->governments.clan_count;++i) {
        if (!realm_ruler(&menu->realms,state->governments.clans[i].head) &&
            !family_gameplay_find(&state->family,state->governments.clans[i].head)) {
            snprintf(reason,capacity,"Saved nomad clan references a missing character.");return false;
        }
    }
    for (size_t i=0;i<state->governments.family_count;++i) {
        if (!realm_ruler(&menu->realms,state->governments.families[i].head) &&
            !family_gameplay_find(&state->family,state->governments.families[i].head)) {
            snprintf(reason,capacity,"Saved patrician family references a missing character.");return false;
        }
    }
    for (size_t i=0;i<state->governments.post_count;++i) {
        const struct ck2_trade_post *p=state->governments.posts+i;
        if (!province_by_id(&menu->provinces,p->province)) {
            snprintf(reason,capacity,"Saved trade post province is missing.");return false;
        }
        bool owner=false;
        for (size_t j=0;j<state->governments.family_count;++j)
            if (state->governments.families[j].head==p->patrician) owner=true;
        if (!owner || (p->current_build[0] && !ck2_trade_find(&menu->trade_catalog,p->current_build))) {
            snprintf(reason,capacity,"Saved trade post family or building is unavailable.");return false;
        }
        for (size_t j=0;j<p->completed_count;++j)
            if (!ck2_trade_find(&menu->trade_catalog,p->completed[j])) {
                snprintf(reason,capacity,"Saved trade post upgrade is missing.");return false;
            }
    }
    for (size_t i=0;i<state->governments.bishop_count;++i) {
        const struct ck2_bishop_nomination *b=state->governments.bishops+i;
        struct realm_title_info title;
        if (!realm_title_info(&menu->realms,b->temple,&title) ||
            (!realm_ruler(&menu->realms,b->candidate) &&
             !family_gameplay_find(&state->family,b->candidate))) {
            snprintf(reason,capacity,"Saved investiture nomination is incompatible.");return false;
        }
    }
    return true;
}

static void checkpoint_command(struct menu *menu, enum debug_console_action action,
                               const char *slot)
{
    char path[2048], line[220];
    if (!checkpoint_slot_path(path, sizeof path, slot)) {
        debug_console_output(&menu->console, "Invalid slot: use 1-32 letters, numbers, _ or -.");
        return;
    }
    if (action == DEBUG_CHECKPOINT_SAVE) {
        struct openck2_checkpoint state = {
            .bookmark = menu->start_date,
            .bookmark_ruler = menu->campaign_start_ruler_id,
            .player = menu->campaign_ruler_id,
            .home_province = menu->home_province,
            .campaign = menu->campaign,
            .family = menu->family,
            .conversion = menu->conversion,
            .warfare = menu->warfare,
            .laws = menu->active_laws,
            .governments = menu->government_mechanics
        };
        if (!construction_checkpoint_capture(&menu->construction, &state.construction)) {
            debug_console_output(&menu->console, "Save failed: cannot capture construction state.");
            return;
        }
        state.title_count = realm_runtime_title_overrides(&menu->realms, NULL, 0);
        bool ok = true;
        if (state.title_count) {
            state.titles = calloc(state.title_count, sizeof *state.titles);
            ok = state.titles && realm_runtime_title_overrides(&menu->realms,
                                                state.titles, state.title_count) == state.title_count;
        }
        if (ok) ok = checkpoint_write(path, &state);
        if (!ok) snprintf(line, sizeof line, "Save failed: %s", checkpoint_last_error());
        else snprintf(line, sizeof line, "Saved native OpenCK2 checkpoint '%s' (not CK2 format).",
                      slot && *slot ? slot : "quick");
        debug_console_output(&menu->console, line);
        /* Shallow system state is owned by menu, only these captures belong here. */
        construction_checkpoint_free(&state.construction);
        free(state.titles);
        return;
    }
    struct openck2_checkpoint loaded = {0};
    if (!checkpoint_read(path, &loaded)) {
        snprintf(line, sizeof line, "Load failed: %s", checkpoint_last_error());
        debug_console_output(&menu->console, line);
        return;
    }
    char reason[160] = {0};
    if (!checkpoint_is_compatible(menu, &loaded, reason, sizeof reason)) {
        debug_console_output(&menu->console, reason);
        checkpoint_free(&loaded);
        return;
    }
    /* Restore construction first: it preallocates all replacements before
     * changing live projects. Historical CK2 assets remain read-only. */
    if (!construction_checkpoint_restore(&menu->construction, &loaded.construction)) {
        debug_console_output(&menu->console, "Load failed allocating construction state.");
        checkpoint_free(&loaded);
        return;
    }
    bool ok = true;
    for (size_t i = 0; ok && i < loaded.family.count; ++i)
        ok = realm_register_runtime_ruler(&menu->realms, &loaded.family.people[i].profile);
    if (ok) ok = realm_runtime_title_overrides_restore(&menu->realms,
                                                        loaded.titles, loaded.title_count);
    if (!ok) {
        /* The source bookmark is still valid; the runtime game must not
         * claim a successful restore after an allocation/validation failure. */
        debug_console_output(&menu->console, "Checkpoint title restoration failed; reload campaign before retrying.");
        checkpoint_free(&loaded);
        return;
    }
    family_gameplay_free(&menu->family);
    conversion_state_free(&menu->conversion);
    warfare_state_free(&menu->warfare);
    ck2_law_state_free(&menu->active_laws);
    menu->family = loaded.family;
    menu->conversion = loaded.conversion;
    menu->warfare = loaded.warfare;
    menu->active_laws = loaded.laws;
    menu->government_mechanics = loaded.governments;
    loaded.family = (struct family_gameplay){0};
    loaded.conversion = (struct conversion_state){0};
    loaded.warfare = (struct warfare_state){0};
    loaded.laws = (struct ck2_law_state){0};
    menu->campaign = loaded.campaign;
    menu->campaign_ruler_id = loaded.player;
    menu->home_province = loaded.home_province;
    menu->time_running = false;
    menu->event_open = false;
    menu->character.open = false;
    menu->character.ruler.id = 0;
    menu->campaign_ruler = NULL; /* Previous bookmark pointer may be stale after succession. */
    const struct ruler_profile *current = realm_ruler(&menu->realms, menu->campaign_ruler_id);
    SDL_DestroyTexture(menu->campaign_portrait);
    menu->campaign_portrait = current ? setup_character_portrait(menu, current) : NULL;
    (void)government_refresh_player(menu);
    if (economy_snapshot_build(&menu->economy, menu->asset_root, &menu->provinces,
                               &menu->realms, &menu->construction, menu->campaign_ruler_id,
                               menu->start_date) == 0) refresh_law_income(menu);
    map_theme_free(&menu->map_theme);
    map_title_shields_clear(menu);
    SDL_DestroyTexture(menu->campaign_title_shield);
    menu->campaign_title_shield = NULL;
    struct realm_title_info primary;
    if (current && realm_primary_title(&menu->realms, menu->campaign_ruler_id, &primary))
        menu->campaign_title_shield = era_title_shield(menu->renderer, menu->asset_root,
                                                       primary.id, current->religion_key);
    if (menu->realms_visible && realm_map_build(&menu->realms, menu->renderer,
            menu->asset_root, &menu->provinces, &menu->campaign_map, menu->start_date) < 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Realm overlay rebuild after load failed");
    conversion_map_refresh(menu);
    menu->selected = NULL;
    menu->holding_province = 0;
    select_province(menu, province_by_id(&menu->provinces, menu->home_province));
    checkpoint_free(&loaded);
    snprintf(line, sizeof line, "Loaded checkpoint '%s'; simulation paused.",
             slot && *slot ? slot : "quick");
    debug_console_output(&menu->console, line);
}

/* Government gameplay diagnostics and guarded developer actions. CK2 assets
 * remain read-only. These are not substitutes for character interactions,
 * elections, the Republic trade interface or the Investiture screens. */
static void government_mechanics_console(struct menu *menu, struct debug_console_result cmd)
{
    struct ck2_government_mechanics *s=&menu->government_mechanics;
    const struct ck2_government_rules *r=&menu->government_rules;
    char line[220];
    enum ck2_government_action status=CK2_GOV_INVALID;
    unsigned id=0, other=0, province=0;
    double amount=0;
    char tail=0, key[72]={0};
    switch (cmd.action) {
    case DEBUG_GOV_CLANS:
        snprintf(line,sizeof line,"Nomad clans %zu/%u (population progression provisional).",
                 s->clan_count,r->max_clans);
        debug_console_output(&menu->console,line);
        for (size_t i=0;i<s->clan_count;++i) {
            const struct ck2_nomad_clan *c=s->clans+i;
            snprintf(line,sizeof line,"Clan %u: provinces %u, pop %.0f/%.0f, max manpower %.0f",
                     c->head,c->provinces,c->population,c->capacity,c->manpower);
            debug_console_output(&menu->console,line);
        }
        return;
    case DEBUG_GOV_CLAN_REGISTER: {
        unsigned provinces=0;
        if (sscanf(cmd.argument,"%u %u %lf %c",&id,&provinces,&amount,&tail)!=3 ||
            !id || !provinces || !isfinite(amount)) {
            debug_console_output(&menu->console,"Usage: clanregister <nomadic ruler ID> <provinces> <population capacity>");return;
        }
        const struct ruler_profile *p=realm_ruler(&menu->realms,id);
        if (!p || strcmp(p->government_key,"nomadic_government")) {
            debug_console_output(&menu->console,"The character must have a verified nomadic government.");return;
        }
        const size_t title_count=realm_held_titles(&menu->realms,id,NULL,0);
        if (!title_count || title_count>50000) {
            debug_console_output(&menu->console,"No bounded title index for this nomadic ruler.");return;
        }
        struct realm_title_info *titles=calloc(title_count,sizeof *titles);
        if (!titles) { debug_console_output(&menu->console,"Out of memory indexing clan counties.");return; }
        const size_t read=realm_held_titles(&menu->realms,id,titles,title_count);
        unsigned counties=0;
        for (size_t i=0;i<read && i<title_count;++i)
            if (titles[i].id[0]=='c' && titles[i].id[1]=='_') ++counties;
        free(titles);
        if (!counties || provinces>counties) {
            debug_console_output(&menu->console,"Registered county count cannot exceed verified personal county titles.");return;
        }
        status=ck2_nomad_add(s,r,id,provinces,amount);
        if (status==CK2_GOV_OK)
            debug_console_output(&menu->console,"Registered clan using the provided population capacity; land allocation remains unmodeled.");
        break;
    }
    case DEBUG_GOV_CLAN_SPLIT: {
        unsigned counties=0; double population=0;
        if (sscanf(cmd.argument,"%u %u %u %lf %c",&id,&other,&counties,&population,&tail)!=4) {
            debug_console_output(&menu->console,"Usage: clansplit <old head> <new head> <counties> <population>");return;
        }
        const struct ruler_profile *p=realm_ruler(&menu->realms,other);
        if (!p) p=family_gameplay_find(&menu->family,other);
        if (!p || p->deceased) {
            debug_console_output(&menu->console,"The new clan head must be a known living character.");return;
        }
        status=ck2_nomad_split(s,r,id,other,counties,population);
        if (status==CK2_GOV_OK)
            debug_console_output(&menu->console,"Clan population divided; clan-title grants still require the title system.");
        break;
    }
    case DEBUG_GOV_FAMILIES:
    case DEBUG_GOV_DOGE_FORECAST: {
        snprintf(line,sizeof line,"Patrician families %zu/%u; Respect excludes CK2 random/uncalculated election modifiers.",
                 s->family_count,r->max_patricians);
        debug_console_output(&menu->console,line);
        unsigned ages[CK2_PATRICIAN_LIMIT]={0};
        double prestige[CK2_PATRICIAN_LIMIT]={0};
        bool known[CK2_PATRICIAN_LIMIT]={0};
        for (size_t i=0;i<s->family_count;++i) {
            const struct ck2_patrician_family *f=s->families+i;
            const struct ruler_profile *p=family_gameplay_find(&menu->family,f->head);
            if (!p) p=realm_ruler(&menu->realms,f->head);
            const bool known_prestige=p && (p->known_resources & (1u<<RULER_PRESTIGE));
            ages[i]=p ? p->age : 0;
            prestige[i]=known_prestige ? p->resources[RULER_PRESTIGE] : 0;
            if (f->head==menu->campaign_ruler_id) prestige[i]=menu->campaign.prestige;
            known[i]=p && p->age_known && (known_prestige || f->head==menu->campaign_ruler_id) && !p->deceased;
            double respect=0;
            const bool computed=known[i] && ck2_patrician_respect(r,ages[i],prestige[i],f->campaign_fund,&respect);
            snprintf(line,sizeof line,"Family head %u, fund %.0f, Respect subtotal %s",
                     f->head,f->campaign_fund,computed ? "computed (see dogeforecast)" : "unresolved");
            debug_console_output(&menu->console,line);
            if (cmd.action==DEBUG_GOV_DOGE_FORECAST && computed) {
                snprintf(line,sizeof line,"  %u: age %u, prestige %.0f, subtotal %.0f",
                         f->head,ages[i],prestige[i],respect);
                debug_console_output(&menu->console,line);
            }
        }
        if (cmd.action==DEBUG_GOV_DOGE_FORECAST) {
            const unsigned leader=ck2_patrician_respect_leader(s,r,ages,prestige,known);
            snprintf(line,sizeof line,leader ?
                "Highest known Respect subtotal: %u (NOT the elected Doge)" :
                "No complete, unique Respect-subtotal leader (election unresolved).",leader);
            debug_console_output(&menu->console,line);
        }
        return;
    }
    case DEBUG_GOV_FAMILY_REGISTER: {
        if (!parse_uint_arg(cmd.argument,&id)) {
            debug_console_output(&menu->console,"Usage: patricianadd <merchant republic ruler ID>");return;
        }
        const struct ruler_profile *p=realm_ruler(&menu->realms,id);
        if (!p || strcmp(p->government_key,"merchant_republic_government")) {
            debug_console_output(&menu->console,"Requires recorded merchant-republic government; patrician potential not fully evaluated.");return;
        }
        status=ck2_patrician_add(s,r,id);
        break;
    }
    case DEBUG_GOV_FUND: {
        unsigned gold=0;
        if (!parse_uint_arg(cmd.argument,&gold)||gold>menu->campaign.treasury) {
            debug_console_output(&menu->console,"Usage: campaignfund <positive gold amount affordable by player>");return;
        }
        status=ck2_patrician_fund(s,menu->campaign_ruler_id,gold);
        if (status==CK2_GOV_OK) menu->campaign.treasury-=gold;
        break;
    }
    case DEBUG_GOV_TRADEPOSTS:
        snprintf(line,sizeof line,"Trade posts: %zu. Construction requires a port anchor in original CK2 map/positions.txt.",s->post_count);
        debug_console_output(&menu->console,line);
        for (size_t i=0;i<s->post_count;++i) {
            const struct ck2_trade_post *p=s->posts+i;
            snprintf(line,sizeof line,"Trade post %u, family %u, value %.1f, build %s (%u days)",
                p->province,p->patrician,ck2_trade_value(p,&menu->trade_catalog),
                p->current_build[0]?p->current_build:"none",p->days_left);
            debug_console_output(&menu->console,line);
        }
        return;
    case DEBUG_GOV_TRADE_BUILD: {
        if (sscanf(cmd.argument,"%u %71s %c",&province,key,&tail)!=2) {
            debug_console_output(&menu->console,"Usage: tradebuild <province ID> <CK2 building key>");return;
        }
        /* Only tr_port_X is currently proven to have the simple authored
         * 'port=yes; patrician=yes; tech=0' construction condition. */
        if ((strncmp(key,"tr_port_",8) && strncmp(key,"tr_enclave_",11)) ||
            !province_by_id(&menu->provinces,province)) {
            debug_console_output(&menu->console,"Only original tr_port_* or tr_enclave_* buildings in known provinces are supported.");return;
        }
        const struct ck2_trade_building *building=ck2_trade_find(&menu->trade_catalog,key);
        if (!building || !building->has_cost || !building->has_days ||
            building->gold_cost>menu->campaign.treasury) {
            debug_console_output(&menu->console,"Trade post building missing or insufficient treasury.");return;
        }
        char *source=load_data(menu->asset_root,"map/positions.txt");
        struct map_positions pos={0};
        const bool indexed=source&&map_positions_parse(&pos,source);
        const struct map_position *anchor=indexed?map_position_find(&pos,province):NULL;
        const bool port=anchor&&anchor->authored_port;
        map_positions_destroy(&pos);SDL_free(source);
        status=ck2_trade_build(s,&menu->trade_catalog,menu->campaign_ruler_id,
                               province,key,port,&amount);
        if (status==CK2_GOV_OK) {
            menu->campaign.treasury-=(unsigned)ceil(amount);
            refresh_law_income(menu);
            snprintf(line,sizeof line,"Started %s at province %u for %.0f gold (authored cost/days).",key,province,amount);
            debug_console_output(&menu->console,line);
        }
        break;
    }
    case DEBUG_GOV_BISHOP_LIST:
        snprintf(line,sizeof line,"Bishop nominations %zu; appointments require an eligible temple vacancy.",s->bishop_count);
        debug_console_output(&menu->console,line);
        for (size_t i=0;i<s->bishop_count;++i) {
            const struct ck2_bishop_nomination *b=s->bishops+i;
            snprintf(line,sizeof line,"%s: candidate %u, %s, approved %s",b->temple,b->candidate,
                     b->papal_approval_required?"papal":"free investiture",b->approved?"yes":"no");
            debug_console_output(&menu->console,line);
        }
        return;
    case DEBUG_GOV_BISHOP_NOMINATE: {
        if (sscanf(cmd.argument,"%71s %u %c",key,&id,&tail)!=2) {
            debug_console_output(&menu->console,"Usage: bishopnominate <b_temple_title> <candidate ID>");return;
        }
        const struct ck2_law *law=ck2_law_selected(&menu->active_laws,&menu->law_catalog,"investiture_law");
        struct realm_title_info title,county;
        if (!law || !realm_title_info(&menu->realms,key,&title) ||
            !realm_dejure_parent_info(&menu->realms,key,&county) ||
            !realm_title_info(&menu->realms,county.id,&county)) {
            debug_console_output(&menu->console,"Investiture law or temple/de jure county is unresolved.");return;
        }
        bool temple=false;
        for (size_t i=0;i<menu->settlement_scene.holding_evidence_count;++i) {
            const struct government_holding_evidence *e=menu->settlement_scene.holding_evidence+i;
            if (!strcmp(e->title_id,key) && e->holding_bit==GOVERNMENT_TEMPLE) temple=true;
        }
        if (!temple || county.holder!=menu->campaign_ruler_id) {
            debug_console_output(&menu->console,"Temple ownership and the player's county jurisdiction must be verified.");return;
        }
        const struct ruler_profile *p=family_gameplay_find(&menu->family,id);
        if (!p) p=realm_ruler(&menu->realms,id);
        struct ruler_profile external={0};
        if (!p) { external.id=id; rulers_load(&external,1,menu->asset_root,menu->start_date);
            if (external.name[0]) p=&external;
        }
        if (!p || p->deceased || !p->age_known || p->age<16 ||
            (strcmp(p->religion_key,"catholic") && strcmp(p->religion_key,"fraticelli"))) {
            debug_console_output(&menu->console,"Candidate must be a known living adult Catholic/Fraticelli character.");return;
        }
        const struct government_religion *faith=government_religion_find(
            &menu->settlement_scene.religions,p->religion_key);
        if (!faith || (p->female ? !faith->female_temple : !faith->male_temple)) {
            debug_console_output(&menu->console,"Candidate fails the original religion's temple-holder restrictions.");return;
        }
        status=ck2_bishop_nominate(s,key,menu->campaign_ruler_id,id,law->key);
        if (status==CK2_GOV_OK)
            debug_console_output(&menu->console,"Nomination queued; title is NOT transferred until approval and vacancy.");
        break;
    }
    case DEBUG_GOV_BISHOP_APPROVE: {
        if (sscanf(cmd.argument,"%71s %c",key,&tail)!=1) {
            debug_console_output(&menu->console,"Usage: bishopapprove <b_temple_title> (manual papal approval in debug)");return;
        }
        struct ck2_bishop_nomination *nom=NULL;
        for (size_t i=0;i<s->bishop_count;++i)
            if (!strcmp(key,s->bishops[i].temple)) nom=s->bishops+i;
        struct realm_title_info existing;
        if (!nom || !realm_title_info(&menu->realms,key,&existing)) {
            debug_console_output(&menu->console,"No nominated known temple title.");return;
        }
        const struct ruler_profile *holder=family_gameplay_find(&menu->family,existing.holder);
        if (!holder) holder=realm_ruler(&menu->realms,existing.holder);
        if (existing.holder && holder && !holder->deceased &&
            !family_gameplay_death_date(&menu->family,existing.holder)) {
            debug_console_output(&menu->console,"The current bishop is alive. Nomination remains queued until vacancy.");return;
        }
        if (nom->approved || existing.holder==nom->candidate) {
            debug_console_output(&menu->console,"Nomination already approved or candidate already holds temple.");return;
        }
        /* Do not mark a nomination approved before title transfer succeeds.
         * A failed registration or title update must leave the nomination
         * retryable instead of persisting a false appointment. */
        {
            struct ruler_profile candidate={.id=nom->candidate};
            const struct ruler_profile *known=family_gameplay_find(&menu->family,nom->candidate);
            if (!known) known=realm_ruler(&menu->realms,nom->candidate);
            if (!known) { rulers_load(&candidate,1,menu->asset_root,menu->start_date); known=&candidate; }
            if (!known->name[0] || !realm_register_runtime_ruler(&menu->realms,known) ||
                !realm_transfer_runtime_title(&menu->realms,key,existing.holder,nom->candidate)) {
                debug_console_output(&menu->console,"Temple transfer failed; nomination remains pending.");return;
            }
            status=ck2_bishop_approve(s,key,true); /* explicit debug approval, NOT papal AI */
            if (status!=CK2_GOV_OK) {
                debug_console_output(&menu->console,"Temple transfer succeeded but approval bookkeeping failed.");return;
            }
            snprintf(line,sizeof line,"Bishop %u appointed to %s by developer approval (not papal AI).",nom->candidate,key);
            debug_console_output(&menu->console,line);
            menu->character.ruler.id=0;
            map_theme_free(&menu->map_theme);
            if (menu->realms_visible && realm_map_build(&menu->realms,menu->renderer,
                  menu->asset_root,&menu->provinces,&menu->campaign_map,menu->start_date)<0)
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,"Realm overlay rebuild after bishop appointment failed");
            conversion_map_refresh(menu);
        }
        break;
    }
    default: return;
    }
    if (status!=CK2_GOV_OK) debug_console_output(&menu->console,ck2_government_action_text(status));
}

static void submit_console_command(struct menu *menu, SDL_Window *window)
{
    char date[80], ruler[128];
    format_campaign_date(date, sizeof date, menu->campaign.date);
    const struct ruler_profile *person = realm_ruler(&menu->realms, menu->campaign_ruler_id);
    snprintf(ruler, sizeof ruler, "%s", person && person->name[0] ? person->name : "Unknown");
    struct debug_console_context context = {
        .date = date,
        .province = menu->selected ? menu->selected->name : NULL,
        .ruler = ruler,
        .has_province = menu->selected != NULL,
        .gold = menu->campaign.treasury + menu->campaign.gold_remainder,
        .prestige = menu->campaign.prestige,
        .piety = menu->campaign.piety
    };
    double potential = 0;
    context.has_levy = province_potential_levy(menu, &potential, NULL);
    context.levy_potential = potential;
    if (!menu->console.input[0]) return;
    char command[180];
    snprintf(command, sizeof command, "> %.127s", menu->console.input);
    debug_console_output(&menu->console, command);
    const struct debug_console_result result = debug_console_execute(&menu->console,
                                                                      menu->console.input, &context);
    menu->console.input[0] = '\0';
    if (result.action == DEBUG_CHECKPOINT_SAVE || result.action == DEBUG_CHECKPOINT_LOAD) {
        checkpoint_command(menu, result.action, result.argument);
    } else if (result.action >= DEBUG_GOV_CLANS && result.action <= DEBUG_GOV_BISHOP_LIST) {
        government_mechanics_console(menu,result);
    } else if (result.action == DEBUG_GOVERNMENT_STATUS) {
        unsigned id=menu->campaign_ruler_id;
        if (result.argument[0] && (!parse_uint_arg(result.argument,&id) || !id)) {
            debug_console_output(&menu->console,"Usage: government [landed ruler ID]");
        } else {
            const struct ruler_profile *ruler=realm_ruler(&menu->realms,id);
            struct realm_title_info primary;
            if (!ruler || !realm_primary_title(&menu->realms,id,&primary))
                debug_console_output(&menu->console,"Ruler not present in dated title history.");
            else {
                char line[220];
                const struct government_definition *gov=government_find(
                    &menu->settlement_scene.governments,ruler->government_key);
                snprintf(line,sizeof line,"Ruler %u (%s), primary title %s: %s",
                    id,ruler->name,primary.id,gov ? gov->id :
                    ruler->government_key[0] ? ruler->government_key : "unresolved");
                debug_console_output(&menu->console,line);
                snprintf(line,sizeof line,"Source: %s. %s",
                    ruler->government_inferred ?
                        (!strcmp(ruler->government_key,"roman_imperial_government") ?
                         "original Roman imperial potential" :
                         "owned holding; eligibility script incomplete") :
                    ruler->government_key[0] ? "character history" : "unresolved",
                    gov && gov->color_defined ? "CK2 government definition loaded." :
                    "No authenticated government color/definition.");
                debug_console_output(&menu->console,line);
                if (gov) {
                    snprintf(line,sizeof line,
                        "Gov rules: preferred=0x%x; holdings=%s; hereditary=%s; gender laws=%s; tax exemptions=%zu",
                        gov->preferred_holdings,
                        government_can_found_holding(gov) ? "permitted/unknown" : "forbidden",
                        government_uses_hereditary_succession(gov) ? "allowed/unknown" : "not modeled",
                        gov->have_gender_laws_defined ?
                            (gov->have_gender_laws ? "yes" : "no") : "unknown",
                        gov->no_vassal_tax.count);
                    debug_console_output(&menu->console,line);
                }
            }
        }
    } else if (result.action == DEBUG_ELECTION_STATUS || result.action == DEBUG_ELECTION_CANDIDATE) {
        election_console_command(menu,result);
    } else if (result.action >= DEBUG_LAW_LIST && result.action <= DEBUG_LAW_TAXES) {
        law_console_command(menu,result);
    } else if (result.action == DEBUG_CHAR_BIRTH || result.action == DEBUG_CHAR_DEATH) {
        character_lifecycle_command(menu, result);
    } else if (result.action >= DEBUG_WAR_DECLARE && result.action <= DEBUG_WAR_LIST) {
        warfare_console_command(menu, result);
    } else if (result.action == DEBUG_FIRE_EVENT) {
        (void)scripted_event_open(menu,result.argument);
    } else if (result.action == DEBUG_CHOOSE_EVENT) {
        char *end=NULL;
        errno=0;
        unsigned long option=strtoul(result.argument,&end,10);
        while (end && *end==' ') ++end;
        if (errno || !end || *end || !option || option>CK2_EVENT_OPTIONS_MAX)
            debug_console_output(&menu->console,"Usage: eventchoose <1-8>");
        else scripted_event_choose(menu,(unsigned)option);
    } else if (result.action == DEBUG_CLOSE_EVENT) {
        menu->event_open=false;
        debug_console_output(&menu->console,"Event dismissed.");
    } else if (result.action == DEBUG_SET_MAP_MODE) {
        int mode = -1;
        for (int i = 0; i < MAP_MODE_COUNT; ++i) {
            const struct map_mode_definition *definition = map_mode_definition(i);
            const char *widget = definition->widget;
            const char *name = !strncmp(widget, "mapmode_", 8) ? widget + 8 : widget;
            if (!SDL_strcasecmp(result.argument, name) ||
                !SDL_strcasecmp(result.argument, definition->label)) {
                mode = i;
                break;
            }
        }
        if (mode < 0 || !map_mode_definition(mode)->implemented)
            debug_console_output(&menu->console, "Unknown or unsupported map mode.");
        else {
            select_campaign_mode(menu, window, mode);
            debug_console_output(&menu->console,
                (int)menu->map_modes.selected == mode ? "Map mode selected." : "Map mode unavailable.");
        }
    } else if (result.action >= DEBUG_SET_PROVINCE_RELIGION &&
               result.action <= DEBUG_CONVERSION_STATUS) {
        conversion_command(menu, result);
    } else if (result.action == DEBUG_FAMILY || result.action == DEBUG_MARRY ||
               result.action == DEBUG_MARRY_MATRI || result.action == DEBUG_CONCEIVE ||
               result.action == DEBUG_DIVORCE) {
        const struct ruler_profile *original = realm_ruler(&menu->realms, menu->campaign_ruler_id);
        const struct ruler_profile *player = family_gameplay_find(&menu->family, menu->campaign_ruler_id);
        if (!player) player = original;
        if (!player) {
            debug_console_output(&menu->console, "No playable ruler profile is available.");
        } else if (result.action == DEBUG_FAMILY) {
            char message[180];
            const size_t count = family_gameplay_children(&menu->family, player->id, NULL, 0);
            snprintf(message, sizeof message, "Ruler %u: spouse %u; %zu runtime children; %u total births.",
                     player->id, player->spouse, count, menu->family.births);
            debug_console_output(&menu->console, message);
        } else if (result.action == DEBUG_MARRY || result.action == DEBUG_MARRY_MATRI) {
            char *endptr;
            errno = 0;
            unsigned long raw_id = strtoul(result.argument, &endptr, 10);
            while (*endptr == ' ') ++endptr;
            const bool valid_id = !errno && !*endptr && raw_id > 0 && raw_id <= UINT_MAX;
            const struct ruler_profile *target = valid_id ?
                realm_ruler(&menu->realms, (unsigned)raw_id) : NULL;
            if (!target && valid_id) target = family_gameplay_find(&menu->family, (unsigned)raw_id);
            /* A marriage candidate need not hold a title. Load that specific
             * ID through the same CK2 character-history parser as portraits. */
            struct ruler_profile unlanded = {0};
            if (!target && valid_id) {
                unlanded.id = (unsigned)raw_id;
                rulers_load(&unlanded, 1, menu->asset_root, menu->start_date);
                if (unlanded.name[0] && unlanded.age_known) target = &unlanded;
            }
            enum family_status status = family_gameplay_marriage_type(&menu->family, player, target,
                                              result.action == DEBUG_MARRY_MATRI);
            debug_console_output(&menu->console, family_status_description(status));
            if (status == FAMILY_OK && menu->character.open) {
                menu->character.ruler.id = 0;
                inspect_campaign_character(menu, original);
            }
        } else {
            const unsigned spouse_id = player->spouse;
            const struct ruler_profile *spouse = family_gameplay_find(&menu->family, spouse_id);
            if (!spouse) spouse = realm_ruler(&menu->realms, spouse_id);
            struct ruler_profile unlanded = {0};
            if (!spouse && spouse_id) {
                unlanded.id = spouse_id;
                rulers_load(&unlanded, 1, menu->asset_root, menu->start_date);
                if (unlanded.name[0] && unlanded.age_known) spouse = &unlanded;
            }
            enum family_status status = FAMILY_NOT_PARTNERS;
            if (spouse && result.action == DEBUG_CONCEIVE) {
                if (!family_gameplay_find(&menu->family, player->id) ||
                    !family_gameplay_find(&menu->family, spouse->id))
                    status = family_gameplay_import_marriage(&menu->family, player, spouse);
                else status = FAMILY_OK;
                if (status == FAMILY_OK)
                    status = family_gameplay_conceive(&menu->family, player, spouse,
                                                     menu->campaign.date);
            } else if (spouse && result.action == DEBUG_DIVORCE) {
                status = family_gameplay_divorce(&menu->family, player->id, spouse_id);
                if (status == FAMILY_OK && menu->character.open) {
                menu->character.ruler.id = 0;
                inspect_campaign_character(menu, original);
            }
            }
            debug_console_output(&menu->console, family_status_description(status));
        }
    } else if (result.action == DEBUG_LIST_TECH) {
        if (!menu->technology.loaded && technology_view_load(&menu->technology, menu->renderer,
                                menu->asset_root, (SDL_Point){WIDTH, HEIGHT}) < 0) {
            debug_console_output(&menu->console, "Technology definitions unavailable; check content mounts.");
        } else if (!result.argument[0]) {
            char message[120];
            snprintf(message, sizeof message, "%zu technology definitions; no researched levels loaded.",
                     menu->technology.catalog.count);
            debug_console_output(&menu->console, message);
        } else {
            char key[96];
            snprintf(key, sizeof key, "%s", result.argument);
            for (char *p = key; *p; ++p) *p = (char)toupper((unsigned char)*p);
            const struct ck2_technology *entry = ck2_technology_find(&menu->technology.catalog, key);
            debug_console_output(&menu->console, entry ? "Technology definition exists; level data unavailable."
                                                       : "Technology key not found.");
        }
    }
}

static void handle_event(struct menu *menu, SDL_Window *window, const SDL_Event *event)
{
    if (event->type == SDL_WINDOWEVENT && menu->game_cursor) {
        if (event->window.event == SDL_WINDOWEVENT_ENTER ||
            (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED && SDL_GetMouseFocus() == window))
            SDL_SetCursor(menu->game_cursor);
        else if (event->window.event == SDL_WINDOWEVENT_LEAVE ||
                 event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            SDL_SetCursor(SDL_GetDefaultCursor());
    }
    if (menu->screen == OPTIONS && event->type != SDL_QUIT) {
        const int previous_language = menu->settings.applied.language;
        if (settings_event(&menu->settings, window, &menu->music, event))
            activate(menu, window, BACK);
        if (previous_language != menu->settings.applied.language) {
            /* Menu labels and character view text are cached at screen load. */
            era_menu_free(&menu->era);
            character_view_free(&menu->character);
            tree_view_close(&menu->genealogy);
            menu->character = (struct character_view){0};
        }
        return;
    }

    /* Console has priority over map shortcuts, modals and background input. */
    if (menu->screen == CAMPAIGN && event->type == SDL_KEYDOWN && !event->key.repeat &&
        event->key.keysym.sym == SDLK_BACKQUOTE) {
        menu->console.open = !menu->console.open;
        map_gesture_cancel(&menu->map_gesture);
        if (menu->console.open) {
            tree_view_close(&menu->genealogy);
            menu->technology.open = false;
            menu->bloodlines.open = false;
            menu->wonder.open = false;
            SDL_StartTextInput();
        } else SDL_StopTextInput();
        return;
    }
    if (menu->screen == CAMPAIGN && menu->console.open && event->type != SDL_QUIT) {
        if (event->type == SDL_TEXTINPUT) {
            const size_t n = strlen(menu->console.input);
            size_t j = n;
            for (const unsigned char *p = (const unsigned char *)event->text.text;
                 *p && j < OPENCK2_CONSOLE_INPUT; ++p)
                if (*p >= 32 && *p < 127 && *p != '`') menu->console.input[j++] = (char)*p;
            menu->console.input[j] = 0;
        } else if (event->type == SDL_KEYDOWN) {
            switch (event->key.keysym.sym) {
            case SDLK_ESCAPE:
                menu->console.open = false;
                SDL_StopTextInput();
                break;
            case SDLK_BACKSPACE: {
                size_t n = strlen(menu->console.input);
                if (n) menu->console.input[n - 1] = 0;
                break;
            }
            case SDLK_RETURN: case SDLK_KP_ENTER:
                if (!event->key.repeat) submit_console_command(menu, window);
                break;
            default: break;
            }
        }
        return;
    }
    /* The bloodline browser is modal, including mouse-wheel scrolling. */
    if (menu->screen == CAMPAIGN && menu->bloodlines.open && event->type != SDL_QUIT) {
        if (event->type == SDL_KEYDOWN && !event->key.repeat &&
            event->key.keysym.sym == SDLK_ESCAPE) menu->bloodlines.open = false;
        else if (event->type == SDL_MOUSEWHEEL) {
            const int direction = event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
            bloodline_view_scroll(&menu->bloodlines, event->wheel.y * direction,
                                  menu->pointer, menu->renderer, menu->asset_root);
        } else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
            bloodline_view_click(&menu->bloodlines,
                                 (SDL_Point){event->button.x, event->button.y});
        else if (event->type == SDL_MOUSEMOTION)
            menu->pointer = (SDL_Point){event->motion.x, event->motion.y};
        return;
    }
    /* Great Works overlays the map and all HUD buttons. Handle its controls
     * before map modes or province input, preventing click-through. */
    if (menu->screen == CAMPAIGN && menu->wonder.open && event->type != SDL_QUIT) {
        if (event->type == SDL_KEYDOWN && !event->key.repeat &&
            event->key.keysym.sym == SDLK_ESCAPE) menu->wonder.open = false;
        else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
            (void)campaign_wonder_click(menu, (SDL_Point){event->button.x, event->button.y});
        else if (event->type == SDL_MOUSEMOTION)
            menu->pointer = (SDL_Point){event->motion.x, event->motion.y};
        return;
    }
    /* Event popup is modal while open.  The developer console may still be
     * reopened using backtick to execute eventchoose/eventclose. */
    if (menu->screen == CAMPAIGN && menu->event_open && event->type != SDL_QUIT) {
        if (event->type == SDL_KEYDOWN && !event->key.repeat) {
            if (event->key.keysym.sym == SDLK_ESCAPE) menu->event_open=false;
            else if (event->key.keysym.sym>=SDLK_1 && event->key.keysym.sym<=SDLK_8)
                scripted_event_choose(menu,(unsigned)(event->key.keysym.sym-SDLK_1+1));
        } else if (event->type == SDL_MOUSEBUTTONUP && event->button.button==SDL_BUTTON_LEFT) {
            SDL_Point point={event->button.x,event->button.y};
            for (size_t i=0;i<menu->pending_event.option_count;i++) {
                SDL_Rect hit=scripted_option_rect(i);
                if (SDL_PointInRect(&point,&hit)) {
                    scripted_event_choose(menu,(unsigned)(i+1));
                    break;
                }
            }
        }
        return;
    }
    if (menu->screen == CAMPAIGN && menu->genealogy.open && event->type != SDL_QUIT) {
        if (event->type == SDL_KEYDOWN && !event->key.repeat) {
            switch (event->key.keysym.sym) {
            case SDLK_ESCAPE: tree_view_close(&menu->genealogy); break;
            case SDLK_UP: tree_view_scroll(&menu->genealogy, 0, -90); break;
            case SDLK_DOWN: tree_view_scroll(&menu->genealogy, 0, 90); break;
            case SDLK_LEFT: tree_view_scroll(&menu->genealogy, -90, 0); break;
            case SDLK_RIGHT: tree_view_scroll(&menu->genealogy, 90, 0); break;
            case SDLK_HOME: tree_view_scroll(&menu->genealogy,
                                -menu->genealogy.scroll_x, -menu->genealogy.scroll_y); break;
            default: break;
            }
        } else if (event->type == SDL_MOUSEWHEEL) {
            const int sign = event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
            if (SDL_GetModState() & KMOD_SHIFT)
                tree_view_scroll(&menu->genealogy, -event->wheel.y * sign * 66, 0);
            else
                tree_view_scroll(&menu->genealogy, -event->wheel.x * sign * 66,
                                 -event->wheel.y * sign * 66);
        }
        else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT) {
            const unsigned chosen = tree_view_click(&menu->genealogy,
                                                     (SDL_Point){event->button.x,event->button.y});
            if (chosen == UINT_MAX) tree_view_close(&menu->genealogy);
            else if (chosen) {
                struct ruler_profile person = {.id = chosen};
                const struct ruler_profile *known = realm_ruler(&menu->realms, chosen);
                if (!known) rulers_load(&person, 1, menu->asset_root, menu->start_date);
                tree_view_close(&menu->genealogy);
                inspect_campaign_character(menu, known ? known : &person);
            }
        } else if (event->type == SDL_MOUSEMOTION)
            menu->pointer = (SDL_Point){event->motion.x,event->motion.y};
        return;
    }
    if (menu->screen == CAMPAIGN && menu->technology.open && event->type != SDL_QUIT) {
        if (event->type == SDL_KEYDOWN && !event->key.repeat &&
            event->key.keysym.sym == SDLK_ESCAPE) menu->technology.open = false;
        if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
            technology_view_click(&menu->technology,
                                  (SDL_Point){event->button.x, event->button.y});
        if (event->type == SDL_MOUSEMOTION)
            menu->pointer = (SDL_Point){event->motion.x, event->motion.y};
        return;
    }

    switch (event->type) {
    case SDL_QUIT:
        menu->running = false;
        break;
    case SDL_MOUSEMOTION:
        if (menu->screen == MAP_SETUP && menu->bookmark_drag)
            move_setup_thumb(menu, event->motion.y);
        menu->pointer = (SDL_Point){event->motion.x, event->motion.y};
        menu->hover = hit_test(menu, event->motion.x, event->motion.y);
        break;
    case SDL_WINDOWEVENT:
        if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            map_gesture_cancel(&menu->map_gesture);
            menu->bookmark_drag = menu->bookmark_scroll_pressed = false;
        }
        if (event->window.event == SDL_WINDOWEVENT_LEAVE) {
            menu->hover = UNAVAILABLE;
            menu->pointer = (SDL_Point){-1, -1};
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (menu->screen == MAP_SETUP && event->button.button == SDL_BUTTON_LEFT &&
            (press_setup_scrollbar(menu, LOBBY_BOOKMARKS, (SDL_Point){event->button.x, event->button.y}) ||
             press_setup_scrollbar(menu, LOBBY_FEATURED, (SDL_Point){event->button.x, event->button.y}))) {
            menu->bookmark_scroll_pressed = true;
            break;
        }
        if (navigable(menu->screen) && (event->button.button == SDL_BUTTON_MIDDLE ||
                                        event->button.button == SDL_BUTTON_RIGHT)) {
            const SDL_Point point = {event->button.x, event->button.y};
            if (menu->screen != CAMPAIGN || campaign_map_interactive(menu, point))
                map_gesture_begin(&menu->map_gesture, event->button.button, point.x, point.y);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event->button.button == SDL_BUTTON_LEFT && menu->bookmark_scroll_pressed) {
            menu->bookmark_drag = menu->bookmark_scroll_pressed = false;
            break;
        }
        if (menu->map_gesture.active && menu->map_gesture.button == event->button.button) {
            int dx = 0, dy = 0;
            const enum map_gesture_result result = map_gesture_end(&menu->map_gesture,
                event->button.button, event->button.x, event->button.y, 4, &dx, &dy);
            if (result == MAP_GESTURE_DRAG && navigable(menu->screen))
                move_map(menu, window, dx, dy, 0);
            else if (result == MAP_GESTURE_CLICK && menu->screen == CAMPAIGN &&
                     event->button.button == SDL_BUTTON_RIGHT)
                campaign_open_province_owner(menu, (SDL_Point){event->button.x, event->button.y},
                                             (SDL_GetModState() & KMOD_SHIFT) != 0);
            break;
        }
        if (event->button.button == SDL_BUTTON_LEFT) {
            const int x = event->button.x, y = event->button.y;
            if (menu->screen == STARTING_ERA) {
                const SDL_Point point = {x, y};
                const int previous_era = menu->era.selected, previous_ruler = menu->era.ruler;
                if (era_menu_click(&menu->era, point)) {
                    if (menu->era.selected != previous_era) {
                        menu->setup_province = 0;
                        menu_sound_play(&menu->music, CUE_BOOKMARK);
                    }
                    else if (menu->era.ruler != previous_ruler)
                        menu_sound_play(&menu->music, CUE_COUNTRY);
                    menu->focus = era_menu_can_start(&menu->era) ? START_ERA : BACK;
                    break;
                }
            }
            if (menu->screen == GAME_RULES && rules_click(&menu->rules, (SDL_Point){x, y})) {
                menu_sound_play(&menu->music, CUE_GENERIC);
                break;
            }
            const enum action action = hit_test(menu, x, y);
            if (menu->screen == MAP_SETUP && action == UNAVAILABLE &&
                setup_select(menu, window, (SDL_Point){x, y}))
                break;
            if (menu->screen == CAMPAIGN) {
                const SDL_Point point = {x, y};
                if (campaign_hud_ui_next_song_hit(&menu->hud_ui, point)) {
                    if (!menu_music_next(&menu->music, menu->asset_root))
                        SDL_Log("Next song: no playable CK2 music file available");
                    break;
                }
                const int mode = campaign_hud_ui_mode_hit(&menu->hud_ui, point);
                if (mode >= 0) {
                    select_campaign_mode(menu, window, mode);
                    break;
                }
                const int nav = campaign_hud_ui_nav_hit(&menu->hud_ui, point);
                if (nav == 2) {
                    toggle_technology(menu);
                    break;
                }
                /* Other original tabs remain visible, but cannot click through. */
                if (nav >= 0) break;
            }
            if (menu->screen == CAMPAIGN && menu->selected && !menu->character.open &&
                !menu->building_open && !menu->wonder.open &&
                menu->province_close.area.w > 0 &&
                SDL_PointInRect(&(SDL_Point){x, y}, &menu->province_close.area)) {
                select_province(menu, NULL);
                break;
            }
            /* The original province Raise Levies button now calls the same
             * guarded gameplay path as the console. No background map click. */
            if (menu->screen == CAMPAIGN && menu->selected && !menu->character.open &&
                !menu->building_open && !menu->wonder.open &&
                menu->province_skin.raise_levy.area.w > 0 &&
                SDL_PointInRect(&(SDL_Point){x, y}, &menu->province_skin.raise_levy.area)) {
                warfare_console_command(menu, (struct debug_console_result){.action = DEBUG_WAR_RAISE});
                break;
            }
            if (menu->screen == CAMPAIGN &&
                (campaign_wonder_click(menu, (SDL_Point){x, y}) ||
                 campaign_character_click(menu, (SDL_Point){x, y}) ||
                 (!menu->character.open && building_click(menu, (SDL_Point){x, y}))))
                break;
            if (menu->screen == CAMPAIGN && action == UNAVAILABLE &&
                campaign_map_interactive(menu, (SDL_Point){x, y})) {
                SDL_Point world;
                if (map_view_pick(&menu->campaign_map, x, y, &world)) {
                    const struct province *province = province_at(&menu->provinces, world.x, world.y);
                    if (province != menu->selected) {
                        select_province(menu, province);
                        if (province) menu_sound_play(&menu->music, CUE_GENERIC);
                    }
                }
            } else {
                activate(menu, window, action);
            }
        }
        break;
    case SDL_MOUSEWHEEL:
        if (navigable(menu->screen)) {
            const int delta = SDL_clamp(event->wheel.y, -6, 6);
            const int scroll = event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -delta : delta;
            struct character_view *view = &menu->character;
            if (menu->screen == CAMPAIGN && view->open &&
                SDL_PointInRect(&menu->pointer, &view->background.area)) {
                if (view->show_vassals && SDL_PointInRect(&menu->pointer, &view->vassal_list)) {
                    const int maximum = SDL_max(0, (int)view->vassal_count - CHARACTER_VASSAL_ROWS);
                    const size_t first = (size_t)SDL_clamp((int)view->vassal_scroll - scroll, 0, maximum);
                    if (first != view->vassal_scroll) {
                        view->vassal_scroll = first;
                        character_view_vassals(view, &menu->realms, menu->renderer, menu->asset_root,
                                               character_relative_portrait, menu);
                    }
                }
                break;
            }
            if (menu->screen == CAMPAIGN && menu->building_open &&
                SDL_PointInRect(&menu->pointer, &menu->building_panel) && !settlement_view(menu)) {
                scroll_buildings(menu, scroll);
                break;
            }
            if (menu->screen == MAP_SETUP &&
                SDL_PointInRect(&menu->pointer, &menu->toolbar.lobby[LOBBY_BOOKMARKS].area)) {
                const int max = SDL_max(0, menu->era.count - lobby_rows(menu, LOBBY_BOOKMARKS));
                menu->bookmark_scroll = SDL_clamp(menu->bookmark_scroll - scroll, 0, max);
                break;
            }
            if (menu->screen == MAP_SETUP &&
                SDL_PointInRect(&menu->pointer, &menu->toolbar.lobby[LOBBY_FEATURED].area)) {
                const int count = menu->era.eras[menu->era.selected].count;
                const int max = SDL_max(0, count - lobby_rows(menu, LOBBY_FEATURED));
                menu->featured_scroll = SDL_clamp(menu->featured_scroll - scroll, 0, max);
                break;
            }
            if (menu->screen == MAP_SETUP && SDL_PointInRect(&menu->pointer, &menu->toolbar.lobby[LOBBY_RIGHT].area)) {
                struct realm_title_info title;
                if (inspected_title(menu, &title)) {
                    const size_t count = realm_vassal_titles(&menu->realms, &title, 0, NULL, 0);
                    const int max = SDL_max(0, (int)count - lobby_rows(menu, LOBBY_VASSALS));
                    const size_t target = (size_t)SDL_clamp((int)menu->vassal_scroll - scroll, 0, max);
                    if (target != menu->vassal_scroll) {
                        menu->vassal_scroll = target;
                        refresh_vassal_art(menu, &title);
                    }
                }
                break;
            }
            move_map(menu, window, 0, 0, event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -delta : delta);
            break;
        }
        if (menu->screen == GAME_RULES) {
            const int direction = event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
            rules_scroll(&menu->rules, -direction * SDL_clamp(event->wheel.y, -20, 20));
            break;
        }
        if (menu->screen == CREDITS) {
            int steps = SDL_clamp(event->wheel.y, -20, 20);
            if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                steps = -steps;
            scroll_credits(menu, -3 * CREDIT_LINE * steps);
        }
        break;
    case SDL_KEYDOWN:
        if (menu->screen == MAP_SETUP && !event->key.repeat) {
            const enum setup_control mode = setup_toolbar_shortcut(&menu->toolbar, event->key.keysym.sym);
            if (mode != SETUP_NO_CONTROL) {
                select_setup_mode(menu, window, mode);
                break;
            }
        }
        if (menu->screen == CAMPAIGN && !event->key.repeat &&
            !(event->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI))) {
            const int mode = map_mode_shortcut((char)event->key.keysym.sym);
            if (mode >= 0) {
                select_campaign_mode(menu, window, mode);
                break;
            }
        }
        switch (event->key.keysym.sym) {
        case SDLK_F5:
            if (menu->screen == CAMPAIGN && !event->key.repeat)
                toggle_technology(menu);
            break;
        case SDLK_ESCAPE:
            if (!event->key.repeat) {
                if (menu->screen == CAMPAIGN && menu->wonder.open)
                    menu->wonder.open = false;
                else if (menu->screen == CAMPAIGN && menu->building_open)
                    menu->building_open = false;
                else if (menu->screen == CAMPAIGN && menu->character.open)
                    menu->character.open = false;
                else if (menu->screen == CAMPAIGN && menu->selected)
                    select_province(menu, NULL);
                else
                    activate(menu, window, menu->screen == MAIN_MENU ? QUIT : BACK);
            }
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!event->key.repeat)
                activate(menu, window, menu->focus);
            break;
        case SDLK_SPACE:
            if (!event->key.repeat)
                activate(menu, window, menu->screen == CAMPAIGN ? TOGGLE_TIME : menu->focus);
            break;
        case SDLK_UP:
        case SDLK_DOWN:
            if (navigable(menu->screen)) {
                move_map(menu, window, 0, event->key.keysym.sym == SDLK_UP ? -155 : 155, 0);
                break;
            }
            if (menu->screen == CREDITS)
                scroll_credits(menu, event->key.keysym.sym == SDLK_UP ? -CREDIT_LINE : CREDIT_LINE);
            else
                move_focus(menu, event->key.keysym.sym == SDLK_UP ? -1 : 1);
            break;
        case SDLK_LEFT:
        case SDLK_RIGHT:
            if (navigable(menu->screen)) {
                move_map(menu, window, event->key.keysym.sym == SDLK_LEFT ? -210 : 210, 0, 0);
                break;
            }
            if (menu->screen == STARTING_ERA) {
                const int delta = event->key.keysym.sym == SDLK_LEFT ? -1 : 1;
                era_menu_step_card(&menu->era, delta);
                menu_sound_play(&menu->music, CUE_BOOKMARK);
                menu->focus = era_menu_can_start(&menu->era) ? START_ERA : BACK;
            }
            break;
        case SDLK_TAB:
            if (navigable(menu->screen))
                move_focus(menu, event->key.keysym.mod & KMOD_SHIFT ? -1 : 1);
            break;
        case SDLK_HOME:
            if (navigable(menu->screen)) {
                const struct map_view *view = &menu->campaign_map;
                move_map(menu, window, (int)((view->world_width * .5f - view->center_x) * view->zoom),
                         (int)((view->world_height * .5f - view->center_y) * view->zoom), -16);
            }
            break;
        case SDLK_PAGEUP:
        case SDLK_PAGEDOWN:
            if (menu->screen == GAME_RULES) {
                rules_scroll(&menu->rules, event->key.keysym.sym == SDLK_PAGEUP ? -3 : 3);
                break;
            }
            if (menu->screen == CREDITS)
                scroll_credits(menu, event->key.keysym.sym == SDLK_PAGEUP ? -480 : 480);
            break;
        }
        break;
    }
}

static int run_game(const char *root)
{
    struct menu_assets assets = {0};
    struct menu menu = {.screen = MAIN_MENU, .running = true, .asset_root = root, .assets = &assets};
    int status = EXIT_FAILURE;
    SDL_Window *window = SDL_CreateWindow("OpenCK2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
    if (!window) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return status;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    char *prefs = SDL_GetPrefPath("OpenCK2", "Launcher");
    menu.selection_path = prefs ? ck2_join_path(prefs, "content-selection.txt") : NULL;
    SDL_free(prefs);
    refresh_content(&menu);
    ck2_mount_content(root, &menu.catalog);
    if (!renderer || SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT) < 0 ||
        settings_load(&menu.settings, renderer, root) < 0 ||
        menu_assets_load(&assets, renderer, root) < 0 ||
        load_main_button_layout(&menu, &assets) < 0 ||
        map_view_build(&menu.menu_map, renderer, root, WIDTH, HEIGHT, false) < 0) {
        fprintf(stderr, "Could not load the CK2 main menu: %s\n", SDL_GetError());
        goto cleanup;
    }

    menu.renderer = renderer;
    menu_music_start(&menu.music, root);
    int lines = 1;
    for (const char *p = assets.credits; *p; ++p)
        lines += *p == '\n';
    menu.credit_scroll_max = SDL_max(0, lines * CREDIT_LINE - 535);
    menu.game_cursor = load_game_cursor(root);
    SDL_ShowWindow(window);
    if (menu.game_cursor && SDL_GetMouseFocus() == window)
        SDL_SetCursor(menu.game_cursor);
    draw(renderer, &assets, &menu);

    SDL_Event event;
    while (menu.running) {
        const int received = SDL_WaitEventTimeout(&event, 32);
        if (received) {
            do {
                handle_event(&menu, window, &event);
            } while (menu.running && SDL_PollEvent(&event));
        }
        const Uint64 now = SDL_GetTicks64();
        const bool day_elapsed = menu.time_running && now >= menu.next_day;
        if (day_elapsed) {
            advance_campaign_day(&menu);
            if (menu.screen == CAMPAIGN && menu.map_modes.selected == MAP_MODE_ECONOMY &&
                !menu.map_theme.overlay) {
                (void)map_theme_build(&menu.map_theme, renderer, &menu.campaign_map,
                                      &menu.provinces, &menu.realms, &menu.settlement_scene,
                                      &menu.construction, menu.asset_root, menu.start_date,
                                      MAP_MODE_ECONOMY);
            }
            menu.next_day = now + campaign_day_interval(menu.campaign_speed);
        }
        const bool content_changed = poll_content_settings(&menu, window);
        struct map_view *view = world_screen(menu.screen) ? &menu.campaign_map : &menu.menu_map;
        const bool water_changed = menu.running && map_view_animate(view, now);
        if (menu.running && (received || content_changed || day_elapsed || water_changed))
            draw(renderer, &assets, &menu);
    }
    if (menu.running)
        fprintf(stderr, "Menu event loop stopped: %s\n", SDL_GetError());
    else
        status = EXIT_SUCCESS;

cleanup:
    if (menu.game_cursor) {
        SDL_SetCursor(SDL_GetDefaultCursor());
        SDL_FreeCursor(menu.game_cursor);
    }
    if (menu.province_overlay)
        SDL_DestroyTexture(menu.province_overlay);
    map_view_free(&menu.menu_map);
    map_view_free(&menu.campaign_map);
    province_map_free(&menu.provinces);
    ck2_catalog_free(&menu.catalog);
    free(menu.selection_path);
    menu_sound_finish(&menu.music);
    menu_music_stop(&menu.music);
    preview_portrait(&menu, NULL);
    clear_holding_style(&menu);
    province_clear_history_art(&menu);
    struct setup_widget *province_widgets[] = {
        &menu.province_close, &menu.province_skin.background,
        &menu.province_skin.portrait_frame,
        &menu.province_skin.county_crown, &menu.province_skin.region_info,
        &menu.province_skin.raise_levy, &menu.province_skin.raise_ships,
        &menu.province_skin.temple_button, &menu.province_skin.capital_icon,
        &menu.province_skin.levy_progress_bg, &menu.province_skin.garrison_icon,
        &menu.province_skin.levy_icon, &menu.province_skin.tax_icon,
        &menu.province_skin.strait_icon, &menu.province_skin.route_icon,
        &menu.province_skin.crown_focus,
        &menu.province_skin.tech_icons[0], &menu.province_skin.tech_icons[1],
        &menu.province_skin.tech_icons[2],
    };
    for (size_t i=0; i<SDL_arraysize(province_widgets); ++i)
        SDL_DestroyTexture(province_widgets[i]->texture);
    map_title_shields_clear(&menu);
    for (size_t i = 0; i < SDL_arraysize(menu.holding_owner_frames); ++i)
        SDL_DestroyTexture(menu.holding_owner_frames[i].texture);
    SDL_DestroyTexture(menu.province_title_shield);
    SDL_DestroyTexture(menu.campaign_title_shield);
    SDL_DestroyTexture(menu.campaign_portrait);
    portraits_close(&menu.portrait_source);
    rules_free(&menu.rules);
    map_settlements_free(&menu.settlement_scene);
    economy_snapshot_free(&menu.economy);
    ck2_law_state_free(&menu.active_laws);
    ck2_law_catalog_free(&menu.law_catalog);
    ck2_elections_free(&menu.election_catalog);
    map_theme_free(&menu.map_theme);
    family_gameplay_free(&menu.family);
    warfare_state_free(&menu.warfare);
    warfare_graph_free(&menu.warfare_graph);
    conversion_state_free(&menu.conversion);
    conversion_catalog_free(&menu.conversion_catalog);
    ck2_trade_catalog_free(&menu.trade_catalog);
    realm_map_free(&menu.realms);
    era_menu_free(&menu.era);
    setup_toolbar_free(&menu.toolbar);
    campaign_hud_ui_free(&menu.hud_ui);
    technology_view_free(&menu.technology);
    bloodline_view_free(&menu.bloodlines);
    free(menu.wonder.locations);
    tree_view_close(&menu.genealogy);
    SDL_StopTextInput();
    settings_free(&menu.settings);
    construction_free(&menu.construction);
    character_view_free(&menu.character);
    clear_wonder(&menu.wonder);
    SDL_DestroyTexture(menu.wonder.button.texture);
    SDL_DestroyTexture(menu.wonder.background.texture);
    SDL_DestroyTexture(menu.wonder.close.texture);
    SDL_DestroyTexture(menu.wonder.terrain.texture);
    SDL_DestroyTexture(menu.wonder.frame.texture);
    SDL_DestroyTexture(menu.wonder.completed.texture);
    SDL_DestroyTexture(menu.wonder.next.texture);
    SDL_DestroyTexture(menu.wonder.previous.texture);
    for (size_t i = 0; i < SDL_arraysize(menu.topbar); ++i)
        SDL_DestroyTexture(menu.topbar[i].texture);
    SDL_DestroyTexture(menu.building_close);
    SDL_DestroyTexture(menu.building_row);
    SDL_DestroyTexture(menu.building_bg);
    SDL_DestroyTexture(menu.settlement_bg);
    SDL_DestroyTexture(menu.settlement_build);
    SDL_DestroyTexture(menu.settlement_hover);
    menu_assets_free(&assets);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    ck2_locale_free();
    ck2_unmount_content();
    return status;
}

int main(int argc, char **argv)
{
    const char *root = SDL_getenv("CK2_DIR");
    char *default_root = NULL;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: ck2-game [--assets CK2_INSTALL_DIRECTORY]\n"
             "Loads the main menu from an existing Crusader Kings II installation.\n"
             "The asset directory can also be set with CK2_DIR.");
        return EXIT_SUCCESS;
    }
    if (argc == 3 && strcmp(argv[1], "--assets") == 0) {
        root = argv[2];
    } else if (argc != 1) {
        fputs("Usage: OpenCK2-game [--assets CK2_INSTALL_DIRECTORY]\n", stderr);
        return EXIT_FAILURE;
    }

    if (!root || !*root) {
        const char *home = SDL_getenv("HOME");
        const char suffix[] = "/.steam/steam/steamapps/common/Crusader Kings II";
        if (home && *home) {
            const size_t size = strlen(home) + sizeof suffix;
            default_root = SDL_malloc(size);
            if (default_root)
                SDL_snprintf(default_root, size, "%s%s", home, suffix);
        }
        root = default_root;
    }
    if (!root) {
        fputs("Specify the local CK2 installation with --assets or CK2_DIR.\n", stderr);
        return EXIT_FAILURE;
    }

    int status = EXIT_FAILURE;
    if (SDL_Init(SDL_INIT_VIDEO) == 0)
        status = run_game(root);
    else
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
    SDL_Quit();
    SDL_free(default_root);
    return status;
}
