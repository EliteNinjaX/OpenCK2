#include "campaign_hud.h"
#include <string.h>

static const char *const nav_names[8] = {
    "icon_topbar_court", "icon_topbar_laws", "icon_topbar_technology",
    "icon_topbar_military", "icon_topbar_plots", "icon_topbar_factions",
    "icon_topbar_religion", "icon_topbar_societies"
};
static const char *const nav_labels[8] = {
    "Court", "Laws", "Technology", "Military", "Intrigue", "Factions",
    "Religion", "Societies"
};

const char *campaign_hud_nav_label(int index)
{
    return index >= 0 && index < 8 ? nav_labels[index] : NULL;
}

static void load_named(struct setup_widget *widget, const struct frontend_gui *gui,
                       int parent, const char *name, SDL_Renderer *renderer,
                       const char *root, SDL_Point viewport)
{
    const int node = frontend_gui_find(gui, parent, name);
    SDL_Rect rect;
    if (node < 0 || !frontend_gui_rect(gui, node, viewport, &rect))
        return;
    if (setup_widget_load_node(widget, renderer, root, gui, node, rect) < 0) {
        /* Partially initialized widgets may own an SDL texture. */
        if (widget->texture) SDL_DestroyTexture(widget->texture);
        *widget = (struct setup_widget){0};
        SDL_Log("Campaign HUD: optional sprite for %s unavailable: %s", name, SDL_GetError());
    }
}

void campaign_hud_ui_load(struct campaign_hud_ui *hud, SDL_Renderer *renderer,
                          const char *root, SDL_Point viewport)
{
    if (!hud || hud->initialized) return;
    struct frontend_gui gui = {0};
    if (frontend_gui_load_file(&gui, root, "interface/main.gui") < 0) {
        SDL_Log("Campaign HUD: no main.gui: %s", SDL_GetError());
        return;
    }
    const int root_node = frontend_gui_find(&gui, -1, "main");
    const int minimap = frontend_gui_find(&gui, root_node, "minimap");
    const int mapmodes = frontend_gui_find(&gui, minimap, "mapmodes");
    const int menu_area = frontend_gui_find(&gui, root_node, "menu_area");
    if (menu_area >= 0)
        load_named(&hud->next_song, &gui, menu_area, "minimap_nextsong", renderer, root, viewport);
    if (root_node >= 0) {
        /* These backgrounds are defined separately from the Society icon in
         * main.gui. The pre-existing raw topbar image does not extend to it. */
        static const char *const backgrounds[] = {
            "topbar_extension_0", "topbar_extension_1", "topbar_extension_2"
        };
        for (size_t i = 0; i < SDL_arraysize(hud->extension); ++i)
            load_named(&hud->extension[i], &gui, root_node, backgrounds[i],
                       renderer, root, viewport);
        const int shield = frontend_gui_find(&gui, root_node, "topbar_ruler_shield");
        const int crown = frontend_gui_find(&gui, root_node, "topbar_ruler_crown");
        SDL_Rect rectangle;
        if (frontend_gui_rect(&gui, shield, viewport, &rectangle))
            hud->topbar_shield.area = rectangle;
        if (frontend_gui_rect(&gui, crown, viewport, &rectangle))
            load_named(&hud->topbar_crown, &gui, root_node,
                       "topbar_ruler_crown", renderer, root, viewport);
        for (size_t i = 0; i < SDL_arraysize(hud->navigation); ++i)
            load_named(&hud->navigation[i], &gui, root_node, nav_names[i],
                       renderer, root, viewport);
    }
    if (mapmodes >= 0) {
        for (int i = 0; i < MAP_MODE_COUNT; ++i) {
            const struct map_mode_definition *definition = map_mode_definition(i);
            load_named(&hud->modes[i], &gui, mapmodes, definition->widget,
                       renderer, root, viewport);
        }
    }
    /* Alert icons form a separate strip above the map, not the minimap modes. */
    struct frontend_gui alerts = {0};
    if (frontend_gui_load_file(&alerts, root, "interface/alerts.gui") == 0) {
        const int window = frontend_gui_find(&alerts, -1, "alerticon_window");
        const int start = frontend_gui_find(&alerts, -1, "alerticon_startposition");
        const int step = frontend_gui_find(&alerts, -1, "alerticon_offset");
        SDL_Rect start_rect, step_rect;
        if (frontend_gui_rect(&alerts, start, viewport, &start_rect) &&
            frontend_gui_rect(&alerts, step, viewport, &step_rect)) {
            hud->alert_start = (SDL_Point){start_rect.x, start_rect.y};
            hud->alert_step = step_rect.x;
            load_named(&hud->alert_background, &alerts, window,
                       "alerticon_background", renderer, root, viewport);
            load_named(&hud->alert_strip, &alerts, window,
                       "alerticon", renderer, root, viewport);
            hud->alert_assets_ready = hud->alert_background.texture &&
                                      hud->alert_strip.texture && hud->alert_strip.frames > 1;
        }
        frontend_gui_free(&alerts);
    }
    hud->initialized = true;
    frontend_gui_free(&gui);
}

bool campaign_hud_ui_next_song_hit(const struct campaign_hud_ui *hud, SDL_Point point)
{
    return hud && hud->next_song.texture && SDL_PointInRect(&point, &hud->next_song.area);
}

int campaign_hud_ui_mode_hit(const struct campaign_hud_ui *hud, SDL_Point point)
{
    if (!hud) return -1;
    for (int i = MAP_MODE_COUNT - 1; i >= 0; --i)
        if (hud->modes[i].texture && SDL_PointInRect(&point, &hud->modes[i].area))
            return i;
    return -1;
}

/* Placeholder alert frames are intentionally not coupled to game state yet.
 * Returning an index lets the caller prevent map clicks behind the strip. */
int campaign_hud_ui_alert_hit(const struct campaign_hud_ui *hud, SDL_Point point)
{
    if (!hud || !hud->alert_assets_ready) return -1;
    const int step = hud->alert_step;
    if (step < 1) return -1;
    for (int i = 0; i < 8; ++i) {
        const SDL_Rect rect = {hud->alert_start.x + i * step,
                               hud->alert_start.y, 51, 51};
        if (SDL_PointInRect(&point, &rect)) return i;
    }
    return -1;
}

int campaign_hud_ui_nav_hit(const struct campaign_hud_ui *hud, SDL_Point point)
{
    if (!hud) return -1;
    for (int i = 7; i >= 0; --i)
        if (hud->navigation[i].texture && SDL_PointInRect(&point, &hud->navigation[i].area))
            return i;
    return -1;
}

void campaign_hud_ui_draw_background(const struct campaign_hud_ui *hud,
                                      SDL_Renderer *renderer)
{
    if (!hud) return;
    for (size_t i = 0; i < SDL_arraysize(hud->extension); ++i)
        if (hud->extension[i].texture)
            setup_widget_draw(&hud->extension[i], renderer, (SDL_Point){-1,-1});
}

void campaign_hud_ui_draw(const struct campaign_hud_ui *hud, SDL_Renderer *renderer,
                          SDL_Point pointer, enum map_mode_id selected)
{
    if (!hud) return;
    for (size_t i = 0; i < SDL_arraysize(hud->navigation); ++i)
        if (hud->navigation[i].texture)
            setup_widget_draw(&hud->navigation[i], renderer, pointer);
    if (hud->alert_assets_ready) {
        /* Eight visual slots use the original alerticon sprite sheet and
         * alerticon_startposition/offset. Frame order is NOT the original
         * CK2 alert-priority logic; gray tint marks these as placeholders. */
        for (int i = 0; i < 8; ++i) {
            const int x = hud->alert_start.x + i * hud->alert_step;
            const int y = hud->alert_start.y;
            const struct setup_widget *bg = &hud->alert_background;
            const struct setup_widget *strip = &hud->alert_strip;
            const SDL_Rect bg_dest = {x + bg->area.x, y + bg->area.y, bg->area.w, bg->area.h};
            const SDL_Rect src = {strip->area.w * (i % strip->frames), 0,
                                  strip->area.w, strip->area.h};
            const SDL_Rect dst = {x + strip->area.x, y + strip->area.y,
                                  strip->area.w, strip->area.h};
            SDL_RenderCopy(renderer, bg->texture, NULL, &bg_dest);
            SDL_SetTextureColorMod(strip->texture, 145, 145, 145);
            SDL_RenderCopy(renderer, strip->texture, &src, &dst);
        }
        SDL_SetTextureColorMod(hud->alert_strip.texture, 255, 255, 255);
    }
    if (hud->next_song.texture) setup_widget_draw(&hud->next_song, renderer, pointer);
    for (int i = 0; i < MAP_MODE_COUNT; ++i) {
        const struct setup_widget *widget = &hud->modes[i];
        if (!widget->texture) continue;
        const struct map_mode_definition *definition = map_mode_definition(i);
        if (!definition->implemented)
            SDL_SetTextureColorMod(widget->texture, 145, 145, 145);
        setup_widget_draw(widget, renderer, pointer);
        SDL_SetTextureColorMod(widget->texture, 255, 255, 255);
        if (i == (int)selected) {
            SDL_SetRenderDrawColor(renderer, 232, 203, 111, 255);
            SDL_RenderDrawRect(renderer, &widget->area);
        }
    }
}

void campaign_hud_ui_free(struct campaign_hud_ui *hud)
{
    if (!hud) return;
    for (size_t i = 0; i < SDL_arraysize(hud->navigation); ++i)
        if (hud->navigation[i].texture) SDL_DestroyTexture(hud->navigation[i].texture);
    for (size_t i = 0; i < SDL_arraysize(hud->modes); ++i)
        if (hud->modes[i].texture) SDL_DestroyTexture(hud->modes[i].texture);
    for (size_t i = 0; i < SDL_arraysize(hud->extension); ++i)
        if (hud->extension[i].texture) SDL_DestroyTexture(hud->extension[i].texture);
    SDL_DestroyTexture(hud->topbar_crown.texture);
    SDL_DestroyTexture(hud->next_song.texture);
    SDL_DestroyTexture(hud->alert_background.texture);
    SDL_DestroyTexture(hud->alert_strip.texture);
    memset(hud, 0, sizeof *hud);
}
