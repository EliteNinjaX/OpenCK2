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
    if (root_node >= 0) {
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
    hud->initialized = true;
    frontend_gui_free(&gui);
}

int campaign_hud_ui_mode_hit(const struct campaign_hud_ui *hud, SDL_Point point)
{
    if (!hud) return -1;
    for (int i = MAP_MODE_COUNT - 1; i >= 0; --i)
        if (hud->modes[i].texture && SDL_PointInRect(&point, &hud->modes[i].area))
            return i;
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

void campaign_hud_ui_draw(const struct campaign_hud_ui *hud, SDL_Renderer *renderer,
                          SDL_Point pointer, enum map_mode_id selected)
{
    if (!hud) return;
    for (size_t i = 0; i < SDL_arraysize(hud->navigation); ++i)
        if (hud->navigation[i].texture)
            setup_widget_draw(&hud->navigation[i], renderer, pointer);
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
    memset(hud, 0, sizeof *hud);
}
