#include "technology_view.h"
#include "ck2_script.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Render controls are placed from the original domestic_technology.gui and
 * the textures from domestic_technology.gfx. Unknown levels are not invented. */
static const char *const lists[] = {"list_techs1", "list_techs2", "list_techs3"};
static const char *const headings[] = {"military_label", "economy_label", "culture_label"};
static const char *const point_icons[] = {"martial_icon", "stewardship_icon", "diplomacy_icon"};
static const char *const group_names[] = {"Military", "Economy", "Culture"};
static const char *const point_sprites[] = {"GFX_tech_points_icon_military",
    "GFX_tech_points_icon_economy", "GFX_tech_points_icon_culture"};

static SDL_Rect position(const struct frontend_gui *gui, int parent, const char *name,
                         SDL_Point viewport)
{
    SDL_Rect r = {0};
    const int node = frontend_gui_find(gui, parent, name);
    if (node >= 0) frontend_gui_rect(gui, node, viewport, &r);
    return r;
}

static void prettify(char *dest, size_t size, const char *key)
{
    if (!size) return;
    if (!strncmp(key, "TECH_", 5)) key += 5;
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)key; *p && n + 1 < size; ++p) {
        if (*p == '_') dest[n++] = ' ';
        else {
            const bool inside_word = n && dest[n - 1] != ' ';
            dest[n++] = (char)(inside_word ? tolower(*p) : toupper(*p));
        }
    }
    dest[n] = 0;
}

int technology_view_load(struct technology_view *view, SDL_Renderer *renderer,
                         const char *root, SDL_Point viewport)
{
    if (!view || !renderer) return SDL_SetError("Technology: invalid arguments");
    if (view->loaded) return 0;
    char *definitions = load_data(root, "common/technology.txt");
    if (!definitions) return SDL_SetError("Technology: cannot read common/technology.txt");
    const int parsed = ck2_technology_parse(&view->catalog, definitions);
    SDL_free(definitions);
    if (parsed < 0) return SDL_SetError("Technology: invalid common/technology.txt");
    struct frontend_gui gui = {0};
    if (frontend_gui_load_file(&gui, root, "interface/domestic_technology.gui") < 0)
        return -1;
    const int parent = frontend_gui_find(&gui, -1, "domestic_technology");
    if (parent < 0 || !frontend_gui_rect(&gui, parent, viewport, &view->panel)) {
        frontend_gui_free(&gui);
        return SDL_SetError("Technology: missing domestic_technology window");
    }
    view->title = position(&gui, parent, "headline", viewport);
    view->close_area = position(&gui, parent, "button_close", viewport);
    view->close_area.w = view->close_area.w ? view->close_area.w : 31;
    view->close_area.h = view->close_area.h ? view->close_area.h : 29;
    const int bg = frontend_gui_find(&gui, parent, "bg_domestic_technology");
    SDL_Rect background = {0};
    if (bg >= 0) frontend_gui_rect(&gui, bg, viewport, &background);
    const int entry = frontend_gui_find(&gui, -1, "tech_entry");
    if (entry >= 0) frontend_gui_rect(&gui, entry, viewport, &view->entry_template);
    for (int i = 0; i < CK2_TECH_GROUP_COUNT; ++i) {
        view->headings[i] = position(&gui, parent, headings[i], viewport);
        view->lists[i] = position(&gui, parent, lists[i], viewport);
        const SDL_Rect area = position(&gui, parent, point_icons[i], viewport);
        if (setup_widget_load_sprite(&view->group_icons[i], renderer, root,
                                     "interface/domestic_technology.gfx", point_sprites[i], area) < 0)
            SDL_Log("Technology: point sprite absent: %s", point_sprites[i]);
    }
    /* The original GUI uses a spriteType outside main.gfx. */
    if (setup_widget_load_sprite(&view->background, renderer, root, "interface/domestic_technology.gfx",
                                 "GFX_tech_bg", background) < 0) {
        frontend_gui_free(&gui);
        technology_view_free(view);
        return SDL_SetError("Technology: GFX_tech_bg unavailable");
    }
    if (setup_widget_load_sprite(&view->close, renderer, root, "interface/generalstuff.gfx",
                                 "GFX_main_close_button", view->close_area) < 0)
        SDL_Log("Technology: optional close sprite absent");
    else view->close_area = view->close.area;
    frontend_gui_free(&gui);
    struct ck2_translation terms[CK2_TECH_MAX];
    for (size_t i = 0; i < view->catalog.count; ++i) {
        prettify(view->labels[i], sizeof view->labels[i], view->catalog.items[i].key);
        terms[i] = (struct ck2_translation){view->catalog.items[i].key,
                                             view->labels[i], sizeof view->labels[i]};
    }
    ck2_translate(root, terms, view->catalog.count);
    view->loaded = true;
    return 0;
}

static void ui_text(SDL_Renderer *renderer, const struct menu_assets *assets,
                    const char *line, int x, int y,
                    void (*draw_text)(SDL_Renderer *, const struct menu_assets *, const char *, size_t, int, int))
{
    draw_text(renderer, assets, line, strlen(line), x, y);
}

void technology_view_draw(const struct technology_view *view, SDL_Renderer *renderer,
                          const struct menu_assets *assets, SDL_Point pointer,
                          void (*draw_text)(SDL_Renderer *, const struct menu_assets *,
                                            const char *, size_t, int, int))
{
    if (!view || !view->open || !view->loaded || !draw_text) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 90);
    SDL_RenderFillRect(renderer, &(SDL_Rect){0, 0, 1600, 900});
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    setup_widget_draw(&view->background, renderer, pointer);
    if (view->close.texture) setup_widget_draw(&view->close, renderer, pointer);
    else {
        SDL_SetRenderDrawColor(renderer, 180, 45, 34, 255);
        SDL_RenderFillRect(renderer, &view->close_area);
        ui_text(renderer, assets, "X", view->close_area.x + view->close_area.w/2, view->close_area.y + 4, draw_text);
    }
    ui_text(renderer, assets, "Technology", view->title.x + view->title.w/2, view->title.y, draw_text);
    for (int group = 0; group < CK2_TECH_GROUP_COUNT; ++group) {
        const SDL_Rect title = view->headings[group], list = view->lists[group];
        ui_text(renderer, assets, group_names[group], title.x + title.w/2, title.y, draw_text);
        if (view->group_icons[group].texture)
            setup_widget_draw(&view->group_icons[group], renderer, pointer);
        SDL_SetTextureColorMod(assets->font, 62, 45, 29);
        const int row_height = view->entry_template.h > 0 ? view->entry_template.h : 78;
        int row = 0;
        for (size_t i = 0; i < view->catalog.count; ++i) {
            if ((int)view->catalog.items[i].group != group) continue;
            const int y = list.y + row * row_height;
            if (y + row_height > list.y + list.h) break;
            const SDL_Rect bar = {list.x + 1, y + 1, list.w - 2, row_height - 5};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 224, 202, 159, row & 1 ? 95 : 120);
            SDL_RenderFillRect(renderer, &bar);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            char shortened[64];
            snprintf(shortened, sizeof shortened, "%.35s", view->labels[i]);
            ui_text(renderer, assets, shortened, list.x + list.w / 2, y + 4, draw_text);
            ++row;
        }
        SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    }
    ui_text(renderer, assets, "Historical technology levels / points not yet loaded",
            view->panel.x + 286, view->panel.y + view->panel.h + 12, draw_text);
}

bool technology_view_click(struct technology_view *view, SDL_Point point)
{
    if (!view || !view->open) return false;
    if (SDL_PointInRect(&point, &view->close_area)) view->open = false;
    return true; /* Modal: do not click through any part of the map/HUD. */
}

void technology_view_free(struct technology_view *view)
{
    if (!view) return;
    SDL_DestroyTexture(view->background.texture);
    SDL_DestroyTexture(view->close.texture);
    for (int i = 0; i < CK2_TECH_GROUP_COUNT; ++i)
        SDL_DestroyTexture(view->group_icons[i].texture);
    *view = (struct technology_view){0};
}
