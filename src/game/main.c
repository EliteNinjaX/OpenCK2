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
#include "map_view.h"

#include <SDL_image.h>
#include <errno.h>
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
              START_ERA, CUSTOM_SETUP, LAUNCH_CAMPAIGN, TOGGLE_REALMS, TOGGLE_TIME, NEXT_MONTH, IMPROVE_HOLDING, SELECT_HOME };

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
    {{1331, 40, 216, 24}, NULL, TOGGLE_TIME},
    {{1350, 247, 224, 30}, "Capital", SELECT_HOME},
    {{356, 768, 232, 40}, "Improve Holding", IMPROVE_HOLDING},
    {{356, 816, 232, 40}, "Next Month", NEXT_MONTH},
    {{1352, 816, 180, 30}, "Realms / Terrain", TOGGLE_REALMS},
    {{1360, 856, 164, 30}, "Main Menu", BACK},
};

static const SDL_Rect campaign_hud[] = {
    {0, 0, 492, 136}, {1260, 0, 340, 72}, {1324, 213, 276, 67},
    {0, 276, 348, 624}, {1320, 696, 280, 204},
};
static const SDL_Rect hud_navigation = {146, 15, 320, 36};

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
    SDL_Texture *setup_portrait, *setup_shield, *campaign_portrait;
    struct vassal_art vassal_art[VASSAL_DISPLAY_LIMIT];
    bool portrait_source_loaded;
    enum screen rules_back;
    struct campaign_date start_date;
    const struct era_ruler *campaign_ruler;
    unsigned home_province, setup_province, campaign_ruler_id;
    char setup_title[72];
    struct county_holdings holdings;
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
    if (action == IMPROVE_HOLDING)
        return menu->selected && menu->selected->id == menu->home_province &&
            menu->campaign.treasury >= IMPROVEMENT_COST &&
            menu->campaign.improvements < IMPROVEMENT_LIMIT;
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
    char *path = base ? ck2_join_path(base, "OpenCK2Launcher") : NULL;
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

static int load_holding_layout(struct menu *menu)
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
    for (int i = 0; valid && i < 2; ++i) {
        const int box = frontend_gui_find(&gui, panel, boxes[i]);
        const int row = frontend_gui_find(&gui, -1, templates[i]);
        const int art = frontend_gui_find(&gui, row, "settlement_placeholder");
        const int frame = frontend_gui_find(&gui, row, "province_settlement_frame");
        valid = frontend_gui_rect(&gui, box, viewport, &menu->holding_rows[i]) &&
                frontend_gui_rect(&gui, row, viewport, &menu->holding_templates[i]) &&
                frontend_gui_rect(&gui, art, viewport, &menu->holding_art_area[i]) &&
                frontend_gui_rect(&gui, frame, viewport, &menu->holding_frame_area[i]) &&
                menu->holding_templates[i].w > 0;
        menu->holding_art_scale[i] = 1;
        if (valid) {
            char scale[24], *end;
            copy_token(scale, sizeof scale, field(gui.blocks[art].body, "scale"));
            const float value = strtof(scale, &end);
            if (*scale && !*end && value > 0 && value <= 4)
                menu->holding_art_scale[i] = value;
        }
    }
    frontend_gui_free(&gui);
    if (!valid)
        return SDL_SetError("interface/province.gui: missing campaign holding layout");
    menu->holding_layout_ready = true;
    return 0;
}

static SDL_Rect place_holding_texture(SDL_Texture *texture, SDL_Rect origin, SDL_Rect offset, float scale)
{
    int w = 0, h = 0;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    return (SDL_Rect){origin.x + offset.x, origin.y + offset.y, (int)(w * scale), (int)(h * scale)};
}

static void draw_campaign_holdings(SDL_Renderer *renderer, const struct menu_assets *assets,
                                   const struct menu *menu)
{
    const struct county_holdings *county = &menu->holdings;
    const int columns = SDL_max(1, menu->holding_rows[0].w / menu->holding_templates[1].w);
    int panel_width = 0;
    SDL_QueryTexture(assets->hud_province, NULL, NULL, &panel_width, NULL);
    const int visible_columns = SDL_max(1, SDL_min(columns,
        (panel_width - (menu->holding_rows[0].x - menu->holding_panel.x)) / menu->holding_templates[1].w));
    for (size_t i = 0; i < county->capacity; ++i) {
        const int row = i ? (int)(i - 1) / visible_columns : 0;
        if (i && row >= 2)
            break;
        SDL_Rect origin = menu->holding_panel;
        if (!i) {
            origin.x += menu->holding_capital.x;
            origin.y += menu->holding_capital.y;
        } else {
            origin = menu->holding_rows[row];
            origin.x += (int)((i - 1) % (size_t)visible_columns) *
                        menu->holding_templates[1].w;
        }
        const int template = i ? 1 : 0;
        SDL_Texture *frame = i ? assets->holding_frame : assets->holding_capital;
        const struct holding *slot = i < county->count ? &county->slots[i] : NULL;
        SDL_Texture *art = assets->holding_art[slot ? slot->type : HOLDING_EMPTY];
        const SDL_Rect frame_rect = place_holding_texture(frame, origin, menu->holding_frame_area[template], 1);
        const SDL_Rect art_rect = place_holding_texture(art, origin, menu->holding_art_area[template], menu->holding_art_scale[template]);
        SDL_RenderCopy(renderer, frame, NULL, &frame_rect);
        SDL_RenderCopy(renderer, art, NULL, &art_rect);
        if (SDL_PointInRect(&menu->pointer, &frame_rect))
            campaign_text(renderer, assets, slot ? (*slot->name ? slot->name : slot->id) : "Empty holding slot",
                          menu->holding_panel.x + 174, menu->holding_rows[0].y - 26);
    }
}

static void draw_campaign(SDL_Renderer *renderer, const struct menu_assets *assets,
                          const struct menu *menu)
{
    if (menu->province_overlay)
        SDL_RenderCopy(renderer, menu->province_overlay, NULL, NULL);

    blit(renderer, assets->hud_ruler, 2, 0);
    if (menu->campaign_portrait)
        SDL_RenderCopy(renderer, menu->campaign_portrait, NULL, &(SDL_Rect){16, 18, 106, 106});
    else if (menu->campaign_ruler && menu->campaign_ruler->portrait)
        SDL_RenderCopy(renderer, menu->campaign_ruler->portrait, NULL, &(SDL_Rect){16, 18, 106, 106});
    else
        SDL_RenderCopy(renderer, assets->hud_arms, NULL, &(SDL_Rect){51, 54, 38, 38});
    for (size_t i = 0; i < SDL_arraysize(assets->hud_tabs); ++i) {
        SDL_Texture *icon = assets->hud_tabs[i];
        SDL_SetTextureColorMod(icon, 175, 175, 175);
        SDL_RenderCopy(renderer, icon, &(SDL_Rect){0, 0, 32, 36},
                       &(SDL_Rect){146 + (int)i * 41, 15, 32, 36});
    }
    blit(renderer, assets->hud_resources_left, 1260, 0);
    blit(renderer, assets->hud_resources_right, 1422, 0);
    blit(renderer, assets->hud_outliner, 1324, 213);
    blit(renderer, assets->hud_province, menu->holding_panel.x, menu->holding_panel.y);
    blit(renderer, assets->hud_minimap, 1320, 696);
    blit(renderer, assets->hud_map, 1329, 705);

    char line[160];
    snprintf(line, sizeof line, "%u gold", menu->campaign.treasury);
    campaign_text(renderer, assets, line, 1408, 19);
    campaign_text(renderer, assets, "Outliner", 1420, 220);

    const int center = 174;
    if (menu->selected)
        campaign_text(renderer, assets, menu->selected->name, center, 291);
    const bool owns_selection = menu->selected && menu->selected->id == menu->home_province;
    campaign_text(renderer, assets, owns_selection ? "Your county" : "Outside your county", 99, 349);
    snprintf(line, sizeof line, "+%u gold per month", campaign_monthly_income(&menu->campaign));
    campaign_text(renderer, assets, line, 99, 379);
    snprintf(line, sizeof line, "Holding upgrades: %u / %d", menu->campaign.improvements, IMPROVEMENT_LIMIT);
    SDL_SetTextureColorMod(assets->font, 36, 26, 16);
    campaign_text(renderer, assets, line, center, 600);
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);

    if (menu->selected)
        draw_campaign_holdings(renderer, assets, menu);
    campaign_text(renderer, assets, campaign_goal_met(&menu->campaign) ? "Goal complete"
                  : "Upgrade your capital and reach 100 gold.", 472, 740);
}

static void select_province(struct menu *menu, const struct province *province)
{
    if (menu->selected == province)
        return;
    if (menu->province_overlay)
        SDL_DestroyTexture(menu->province_overlay);
    menu->selected = province;
    const unsigned id = province ? province->id : 0;
    if (menu->holding_province != id) {
        holdings_load(&menu->holdings, menu->asset_root, province, menu->start_date);
        menu->holding_province = id;
    }
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
    return true;
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
    if (!menu->holding_layout_ready && load_holding_layout(menu) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Province holdings", SDL_GetError(), window);
        return;
    }
    if (new_game) {
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
    if (!load_realms(menu, window, menu->start_date))
        menu->realms_visible = false;
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
        .portrait_year = menu->era.eras[menu->era.selected].date.year,
        .female = profile->female, .age_known = profile->age_known};
    SDL_strlcpy(ruler.dna, profile->dna, sizeof ruler.dna);
    SDL_strlcpy(ruler.properties, profile->properties, sizeof ruler.properties);
    SDL_strlcpy(ruler.culture, profile->culture_key, sizeof ruler.culture);
    SDL_strlcpy(ruler.religion, profile->religion_key, sizeof ruler.religion);
    return portrait_render(&menu->portrait_source, menu->renderer, menu->asset_root, &ruler);
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
                 const struct menu_assets *assets, const struct menu *menu)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    const bool world = world_screen(menu->screen);
    SDL_Texture *map = world ? menu->campaign_map.texture : menu->menu_map.texture;
    SDL_RenderCopy(renderer, map, NULL, NULL);
    if (world && menu->realms_visible)
        realm_map_draw(&menu->realms, renderer);
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
        char date_label[72];
        if (button->action == TOGGLE_TIME) {
            texture = assets->hud_date;
            char date[48];
            format_campaign_date(date, sizeof date, menu->campaign.date);
            snprintf(date_label, sizeof date_label, "%s%s", date, menu->time_running ? "" : "  (Paused)");
            label = date_label;
        }

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
        (menu->screen == CAMPAIGN && SDL_PointInRect(&menu->pointer, &hud_navigation)) ||
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
    SDL_RenderPresent(renderer);
}

static enum action hit_test(const struct menu *menu, int x, int y)
{
    size_t count;
    const struct button *buttons = buttons_for(menu, &count);
    const SDL_Point point = {x, y};
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
    case SELECT_HOME: cue = CUE_COUNTRY; break;
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
        break;
    }
    case TOGGLE_TIME:
        menu->time_running = !menu->time_running;
        menu->next_day = SDL_GetTicks64() + 125;
        break;
    case NEXT_MONTH:
        menu->time_running = false;
        campaign_advance_month(&menu->campaign);
        break;
    case IMPROVE_HOLDING:
        campaign_improve(&menu->campaign);
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
        if (settings_event(&menu->settings, window, &menu->music, event))
            activate(menu, window, BACK);
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
            menu->drag_button = 0;
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
            menu->drag_start = (SDL_Point){event->button.x, event->button.y};
            menu->drag_button = event->button.button;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event->button.button == SDL_BUTTON_LEFT && menu->bookmark_scroll_pressed) {
            menu->bookmark_drag = menu->bookmark_scroll_pressed = false;
            break;
        }
        if (menu->drag_button && menu->drag_button == event->button.button) {
            menu->drag_button = 0;
            if (navigable(menu->screen))
                move_map(menu, window, menu->drag_start.x - event->button.x,
                         menu->drag_start.y - event->button.y, 0);
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
            if (menu->screen == CAMPAIGN && action == UNAVAILABLE &&
                campaign_map_area((SDL_Point){x, y})) {
                SDL_Point world;
                if (map_view_pick(&menu->campaign_map, x, y, &world)) {
                    const struct province *province = province_at(&menu->provinces, world.x, world.y);
                    if (province && province != menu->selected) {
                        select_province(menu, province);
                        menu_sound_play(&menu->music, CUE_COUNTRY);
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
        switch (event->key.keysym.sym) {
        case SDLK_ESCAPE:
            if (!event->key.repeat)
                activate(menu, window, menu->screen == MAIN_MENU ? QUIT : BACK);
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
        case SDLK_w:
            if (!event->key.repeat && menu->screen == CAMPAIGN)
                activate(menu, window, TOGGLE_REALMS);
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
    struct menu menu = {.screen = MAIN_MENU, .running = true, .asset_root = root};
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
        menu_assets_load(&assets, renderer, root) < 0 ||
        load_main_button_layout(&menu, &assets) < 0 ||
        settings_load(&menu.settings, renderer, root) < 0 ||
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
            campaign_advance_day(&menu.campaign);
            menu.next_day = now + 125;
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
    if (menu.campaign_portrait)
        SDL_DestroyTexture(menu.campaign_portrait);
    portraits_close(&menu.portrait_source);
    rules_free(&menu.rules);
    realm_map_free(&menu.realms);
    era_menu_free(&menu.era);
    setup_toolbar_free(&menu.toolbar);
    settings_free(&menu.settings);
    menu_assets_free(&assets);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
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
        fputs("Usage: ck2-game [--assets CK2_INSTALL_DIRECTORY]\n", stderr);
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
