#include "bloodline_view.h"
#include "bloodline_browser_core.h"
#include "ck2_script.h"
#include <stdio.h>
#include <string.h>

static SDL_Rect rect_for(const struct frontend_gui *gui, int parent,
                         const char *name, SDL_Point viewport)
{
    SDL_Rect r = {0};
    frontend_gui_rect(gui, frontend_gui_find(gui, parent, name), viewport, &r);
    return r;
}

static void redraw_symbols(struct bloodline_view *view, SDL_Renderer *renderer, const char *root)
{
    for (size_t i = 0; i < BLOODLINE_VIEW_ROWS; ++i) {
        SDL_DestroyTexture(view->symbols[i].texture);
        view->symbols[i] = (struct setup_widget){0};
        const size_t index = view->first + i;
        if (index >= view->definitions.count) continue;
        const ck2_bloodline_type *definition = &view->definitions.types[index];
        if (!definition->picture || strncmp(definition->picture, "GFX_bloodlines_", 15))
            continue;
        SDL_Rect r = view->rows[i];
        r.x += 20; r.y += 16; r.w = 70; r.h = 70;
        if (setup_widget_load_sprite(&view->symbols[i], renderer, root,
                                     "interface/bloodlines.gfx", definition->picture, r) < 0)
            SDL_ClearError();
    }
}

int bloodline_view_load(struct bloodline_view *view, SDL_Renderer *renderer,
                        const char *root, SDL_Point viewport)
{
    if (!view || !renderer || !root) return SDL_SetError("Bloodlines: invalid arguments");
    if (view->loaded) return 0;
    if (!ck2_bloodlines_load(&view->definitions, root))
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Bloodlines: some definition files could not be parsed");
    struct frontend_gui gui = {0};
    if (frontend_gui_load_file(&gui, root, "interface/bloodlines.gui") < 0) {
        ck2_bloodlines_free(&view->definitions);
        return -1;
    }
    const int panel = frontend_gui_find(&gui, -1, "bloodlines_window");
    view->panel = rect_for(&gui, -1, "bloodlines_window", viewport);
    view->list = rect_for(&gui, panel, "bloodlines_list", viewport);
    view->title = rect_for(&gui, panel, "bloodlines_header_text", viewport);
    view->owner = rect_for(&gui, panel, "bloodlines_owner_text", viewport);
    view->close_area = rect_for(&gui, panel, "close_button", viewport);
    const int bg = frontend_gui_find(&gui, panel, "bloodlines_window_bg");
    SDL_Rect area = rect_for(&gui, panel, "bloodlines_window_bg", viewport);
    if (bg >= 0 && setup_widget_load_sprite(&view->background, renderer, root,
                      "interface/bloodlines.gfx", "GFX_bloodlines_background", area) < 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Bloodlines: background image unavailable");
    if (setup_widget_load_sprite(&view->close, renderer, root, "interface/bloodlines.gfx",
                                 "GFX_bloodlines_close_button", view->close_area) < 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Bloodlines: close image unavailable");
    else view->close_area = view->close.area;
    frontend_gui_free(&gui);
    if (panel < 0 || view->list.w <= 0 || view->list.h <= 0) {
        bloodline_view_free(view);
        return SDL_SetError("Bloodlines: missing list in bloodlines.gui");
    }
    /* Each original bloodline_entry is approximately 340x200. The CK2 list
       has 661px of width: two cards per row, three rows in view. */
    const int cols = 2;
    const int w = view->list.w / cols;
    const int h = view->list.h / (BLOODLINE_VIEW_ROWS / cols);
    for (size_t i = 0; i < BLOODLINE_VIEW_ROWS; ++i)
        view->rows[i] = (SDL_Rect){view->list.x + (int)(i % cols) * w,
                                   view->list.y + (int)(i / cols) * h, w - 2, h - 3};
    redraw_symbols(view, renderer, root);
    view->loaded = true;
    return 0;
}

static void write_text(SDL_Renderer *renderer, const struct menu_assets *assets,
                       const char *str, int x, int y,
                       void (*draw_text)(SDL_Renderer *, const struct menu_assets *, const char *, size_t, int, int))
{
    draw_text(renderer, assets, str, strlen(str), x, y);
}

void bloodline_view_draw(const struct bloodline_view *view, SDL_Renderer *renderer,
                         const struct menu_assets *assets, SDL_Point pointer,
                         void (*draw_text)(SDL_Renderer *, const struct menu_assets *, const char *, size_t, int, int))
{
    if (!view || !view->open || !view->loaded || !draw_text) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
    SDL_RenderFillRect(renderer, &(SDL_Rect){0, 0, 1600, 900});
    if (view->background.texture) setup_widget_draw(&view->background, renderer, pointer);
    else {
        const SDL_Rect rect = {view->list.x - 22, view->list.y - 123,
                               view->list.w + 44, view->list.h + 151};
        SDL_SetRenderDrawColor(renderer, 173, 151, 111, 245);
        SDL_RenderFillRect(renderer, &rect);
    }
    if (view->close.texture) setup_widget_draw(&view->close, renderer, pointer);
    else {
        SDL_SetRenderDrawColor(renderer, 140, 45, 40, 255);
        SDL_RenderFillRect(renderer, &view->close_area);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetTextureColorMod(assets->font, 50, 38, 27);
    write_text(renderer, assets, ck2_localize("BLOODLINES_HEADER", "Bloodlines"),
               view->title.x + view->title.w / 2, view->title.y, draw_text);
    char owner[200];
    snprintf(owner, sizeof owner, "%s: %s", view->owner_name,
             "definitions catalogue (membership not reconstructed)");
    write_text(renderer, assets, owner, view->owner.x + view->owner.w / 2, view->owner.y,
               draw_text);
    if (view->definitions.count == 0)
        write_text(renderer, assets, "No bloodline definition files are mounted",
                   view->list.x + view->list.w / 2, view->list.y + 70, draw_text);
    for (size_t i = 0; i < BLOODLINE_VIEW_ROWS; ++i) {
        const size_t index = view->first + i;
        if (index >= view->definitions.count) break;
        const SDL_Rect box = view->rows[i];
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, index == view->selected ? 228 : 112,
                               index == view->selected ? 197 : 83, 48, 75);
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (view->symbols[i].texture) {
            const struct setup_widget *symbol = &view->symbols[i];
            const SDL_Rect src = {0, 0, symbol->area.w, symbol->area.h};
            const SDL_Rect dst = {box.x + 23, box.y + 22, 68, 68};
            SDL_RenderCopy(renderer, symbol->texture, &src, &dst);
        }
        const ck2_bloodline_type *line = &view->definitions.types[index];
        char name[90];
        snprintf(name, sizeof name, "%.34s", ck2_localize(line->key, line->key));
        write_text(renderer, assets, name, box.x + box.w/2, box.y + 109, draw_text);
        const char *inheritance = line->inheritance == CK2_BLOOD_MATRILINEAL ? "matrilineal" :
                                  line->inheritance == CK2_BLOOD_PATRILINEAL ? "patrilineal" :
                                  line->inheritance == CK2_BLOOD_NO_INHERITANCE ? "no inheritance" : "all descendants";
        write_text(renderer, assets, inheritance, box.x + box.w/2, box.y + 135, draw_text);
    }
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
}

bool bloodline_view_click(struct bloodline_view *view, SDL_Point point)
{
    if (!view || !view->open) return false;
    if (SDL_PointInRect(&point, &view->close_area)) {
        view->open = false;
        return true;
    }
    for (size_t i = 0; i < BLOODLINE_VIEW_ROWS; ++i)
        if (view->first + i < view->definitions.count && SDL_PointInRect(&point, &view->rows[i])) {
            view->selected = view->first + i;
            return true;
        }
    return true; /* modal: never pass click through to the map */
}

bool bloodline_view_scroll(struct bloodline_view *view, int delta, SDL_Point point,
                           SDL_Renderer *renderer, const char *root)
{
    if (!view || !view->open || !SDL_PointInRect(&point, &view->list)) return false;
    if (delta && view->definitions.count > BLOODLINE_VIEW_ROWS) {
        const size_t first = bloodline_browser_page(view->first, view->definitions.count, delta);
        if (first != view->first) {
            view->first = first;
            redraw_symbols(view, renderer, root);
        }
    }
    return true;
}

void bloodline_view_free(struct bloodline_view *view)
{
    if (!view) return;
    SDL_DestroyTexture(view->background.texture);
    SDL_DestroyTexture(view->close.texture);
    for (size_t i = 0; i < BLOODLINE_VIEW_ROWS; ++i)
        SDL_DestroyTexture(view->symbols[i].texture);
    ck2_bloodlines_free(&view->definitions);
    *view = (struct bloodline_view){0};
}
