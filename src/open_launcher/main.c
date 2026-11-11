#define _XOPEN_SOURCE 700
#include "../version.h"
#include "install.h"
#include "catalog.h"
#include "../game/assets.h"

#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

enum { WIDTH = 1000, HEIGHT = 600, ROW_HEIGHT = 33, VISIBLE_ROWS = 11, ROW_TOP = 128, SCROLL_SIZE = 16 };
enum page { DLC_PAGE, MODS_PAGE, SETTINGS_PAGE };
static const char version[] = OPENCK2_VERSION_LABEL;
enum action { NONE, QUIT, DLC_TAB, MODS_TAB, MASTER, SETTINGS, BROWSE, PLAY };
enum choice { CANCELLED, REJECTED, SELECTED, CHOOSER_ERROR };

static const SDL_Rect button_areas[] = {
    [QUIT] = {49, 531, 300, 52},
    [DLC_TAB] = {617, 85, 160, 30},
    [MODS_TAB] = {784, 85, 160, 30},
    [MASTER] = {948, 87, 26, 26},
    [SETTINGS] = {644, 493, 300, 30},
    [BROWSE] = {644, 299, 300, 52},
    [PLAY] = {651, 531, 300, 52},
};
static const char *const button_labels[] = {
    [QUIT] = "Quit", [DLC_TAB] = "DLC", [MODS_TAB] = "Mods",
    [SETTINGS] = "Settings", [BROWSE] = "Choose Folder", [PLAY] = "Play",
};

struct theme {
    SDL_Texture *background, *panel, *button, *hover, *tabs, *tab_hover, *row, *font, *checkbox;
    SDL_Texture *icons[CK2_DLC_ICON_COUNT];
    SDL_Texture *category_row, *category_arrow, *sub_row, *unavailable_row, *store;
    SDL_Texture *scroll_up, *scroll_down, *scroll_track, *scroll_thumb;
    struct glyph glyphs[256];
};

static void free_theme(struct theme *theme)
{
    SDL_Texture *textures[] = {theme->background, theme->panel, theme->button, theme->hover, theme->tabs, theme->tab_hover, theme->row, theme->font, theme->checkbox, theme->scroll_up, theme->scroll_down,
                              theme->scroll_track, theme->scroll_thumb, theme->category_row, theme->category_arrow,
                              theme->sub_row, theme->unavailable_row, theme->store};
    for (size_t i = 0; i < SDL_arraysize(textures); ++i) {
        if (textures[i])
            SDL_DestroyTexture(textures[i]);
    }
    for (int i = 0; i < CK2_DLC_ICON_COUNT; ++i) {
        if (theme->icons[i])
            SDL_DestroyTexture(theme->icons[i]);
    }
}

static bool load_theme(struct theme *theme, SDL_Renderer *renderer, const char *directory)
{
    char *base = SDL_GetBasePath();
    if (base) {
        char *background = ck2_join_path(base, "openck2-background.png");
        if (background && access(background, R_OK) == 0)
            theme->background = ck2_load_texture(renderer, base, "openck2-background.png");
        free(background);
        SDL_free(base);
    }
    if (!theme->background)
        theme->background = ck2_load_texture(renderer, directory, "launcher/gfx/interface/background.dds");
    theme->panel = ck2_load_texture(renderer, directory, "launcher/gfx/interface/launcher_dlc_mod_bg.dds");
    theme->button = ck2_load_texture(renderer, directory, "gfx/interface/big_button_300.dds");
    theme->hover = ck2_load_hover(renderer, directory, "gfx/interface/big_button_300.dds");
    theme->tabs = ck2_load_texture(renderer, directory, "launcher/gfx/interface/tab_button.tga");
    theme->tab_hover = ck2_load_hover(renderer, directory, "launcher/gfx/interface/tab_button.tga");
    theme->row = ck2_load_texture(renderer, directory, "launcher/gfx/interface/launcher_dlc_entry_bg.dds");
    theme->category_row = ck2_load_texture(renderer, directory, "launcher/gfx/interface/launcher_dlc_entry_category_bg.dds");
    theme->category_arrow = ck2_load_texture(renderer, directory, "launcher/gfx/interface/dlc_category_button.dds");
    theme->sub_row = ck2_load_texture(renderer, directory, "launcher/gfx/interface/launcher_dlc_entry_sub_bg.dds");
    theme->unavailable_row = ck2_load_texture(renderer, directory, "launcher/gfx/interface/launcher_dlc_entry_sub_unowned_bg.dds");
    theme->store = ck2_load_texture(renderer, directory, "launcher/gfx/interface/store_button.dds");
    theme->scroll_up = ck2_load_texture(renderer, directory, "gfx/interface/scroll_up.dds");
    theme->scroll_down = ck2_load_texture(renderer, directory, "gfx/interface/scroll_down.dds");
    theme->scroll_track = ck2_load_texture(renderer, directory, "gfx/interface/scroll_bg.dds");
    theme->scroll_thumb = ck2_load_texture(renderer, directory, "gfx/interface/scroll.dds");
    theme->checkbox = ck2_load_texture(renderer, directory, "gfx/interface/checkbox_default.dds");
    for (int i = 0; i < CK2_DLC_ICON_COUNT; ++i)
        theme->icons[i] = ck2_load_texture(renderer, directory, ck2_catalog_icon_path(i));
    theme->font = ck2_load_texture(renderer, directory, "gfx/fonts/vic_22.tga");
    return theme->background && theme->panel && theme->button && theme->hover &&
        theme->tabs && theme->tab_hover && theme->row && theme->font && theme->checkbox && theme->scroll_up && theme->scroll_down &&
        theme->scroll_track && theme->scroll_thumb && theme->category_row && theme->category_arrow &&
        theme->sub_row && theme->unavailable_row && theme->store &&
        ck2_load_glyphs(theme->glyphs, directory) == 0;
}

static const struct glyph *lookup(const struct theme *theme, unsigned char ch)
{
    return &theme->glyphs[theme->glyphs[ch].advance > 0 ? ch : '?'];
}

static void text(SDL_Renderer *renderer, const struct theme *theme, const char *label,
                 size_t length, int x, int y, bool centered)
{
    if (centered) {
        int width = 0;
        for (size_t i = 0; i < length; ++i)
            width += lookup(theme, (unsigned char)label[i])->advance;
        x -= width / 2;
    }

    for (size_t i = 0; i < length; ++i) {
        const struct glyph *glyph = lookup(theme, (unsigned char)label[i]);
        const SDL_Rect target = {x + glyph->x_offset, y + glyph->y_offset,
                                 glyph->area.w, glyph->area.h};
        if (target.w > 0 && target.h > 0)
            SDL_RenderCopy(renderer, theme->font, &glyph->area, &target);
        x += glyph->advance;
    }
}

static void draw_path(SDL_Renderer *renderer, const struct theme *theme, const char *path)
{
    const SDL_Rect field = {620, 184, 348, 100};
    SDL_SetRenderDrawColor(renderer, 19, 16, 15, 255);
    SDL_RenderFillRect(renderer, &field);
    SDL_SetRenderDrawColor(renderer, 128, 112, 87, 255);
    SDL_RenderDrawRect(renderer, &field);
    const SDL_Rect inset = {630, 191, 328, 86};
    SDL_RenderSetClipRect(renderer, &inset);

    for (int row = 0; row < 4 && *path; ++row) {
        int width = 0;
        size_t length = 0;
        const int limit = row == 3 ? 305 : 325;
        size_t separator = 0;
        int separator_width = 0;
        while (path[length]) {
            const unsigned char ch = (unsigned char)path[length];
            const int advance = lookup(theme, ch)->advance;
            if (width + advance > limit)
                break;
            width += advance;
            ++length;
            if (ch == '/' || ch == ' ') {
                separator = length;
                separator_width = width;
            }
        }
        if (row < 3 && path[length] && separator > 1) {
            length = separator;
            width = separator_width;
        }
        text(renderer, theme, path, length, inset.x, inset.y + 21 * row, false);
        path += length;
        if (row == 3 && *path)
            text(renderer, theme, "...", 3, inset.x + width, inset.y + 21 * row, false);
    }
    SDL_RenderSetClipRect(renderer, NULL);
}

static bool visible(enum page page, enum action action, bool content_only)
{
    if (content_only && (action == QUIT || action == SETTINGS))
        return false;
    if (action == BROWSE)
        return page == SETTINGS_PAGE;
    if (action == DLC_TAB || action == MODS_TAB || action == MASTER)
        return page != SETTINGS_PAGE;
    return action != NONE;
}

struct dlc_row {
    struct content_entry *entry;
    enum dlc_category category;
};

struct dlc_view {
    struct dlc_row *rows;
    size_t count;
    bool collapsed[DLC_CATEGORY_COUNT];
};

static void rebuild_dlc_view(struct dlc_view *view, const struct content_list *list)
{
    free(view->rows);
    view->rows = calloc(list->count + DLC_CATEGORY_COUNT, sizeof *view->rows);
    view->count = 0;
    if (!view->rows)
        return;

    enum dlc_category previous = EXPANSION;
    for (size_t i = 0; i < list->count; ++i) {
        struct content_entry *entry = &list->entries[i];
        const enum dlc_category category = entry->category;
        if (category != previous) {
            view->rows[view->count++] = (struct dlc_row){NULL, category};
            previous = category;
        }
        if (!view->collapsed[category])
            view->rows[view->count++] = (struct dlc_row){entry, category};
    }
}

static size_t visible_count(const struct content_catalog *catalog,
                            const struct dlc_view *view, enum page page)
{
    return page == MODS_PAGE ? catalog->mods.count : view->count;
}

static struct dlc_row visible_row(const struct content_catalog *catalog,
                                  const struct dlc_view *view, enum page page, size_t index)
{
    if (page == MODS_PAGE)
        return (struct dlc_row){&catalog->mods.entries[index], EXPANSION};
    return view->rows[index];
}

static SDL_Rect store_area(int row_y)
{
    return (SDL_Rect){818, row_y + 5, 116, 24};
}

static void open_dlc_store(SDL_Window *window, unsigned steam_id)
{
    char url[96];
    snprintf(url, sizeof url, "https://store.steampowered.com/app/%u/", steam_id);
    if (SDL_OpenURL(url) < 0)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Steam Store", SDL_GetError(), window);
}

static void draw_checkbox(SDL_Renderer *renderer, const struct theme *theme,
                           const SDL_Rect *target, bool checked, bool mixed, bool highlight)
{
    const SDL_Rect frame = {checked ? 24 : 0, 0, 24, 24};
    const Uint8 shade = highlight ? 255 : 232;
    SDL_SetTextureColorMod(theme->checkbox, shade, shade, shade);
    SDL_RenderCopy(renderer, theme->checkbox, &frame, target);
    if (mixed) {
        const SDL_Rect mark = {target->x + 8, target->y + target->h / 2 - 1, target->w - 16, 2};
        SDL_SetRenderDrawColor(renderer, 227, 202, 135, 255);
        SDL_RenderFillRect(renderer, &mark);
    }
}

struct scrollbar {
    SDL_Rect up, down, track, thumb;
};

static struct scrollbar scrollbar_layout(size_t first, size_t last)
{
    const int x = 956, bottom = ROW_TOP + VISIBLE_ROWS * ROW_HEIGHT;
    struct scrollbar bar = {
        .up = {x, ROW_TOP, SCROLL_SIZE, SCROLL_SIZE},
        .down = {x, bottom - SCROLL_SIZE, SCROLL_SIZE, SCROLL_SIZE},
        .track = {x, ROW_TOP + SCROLL_SIZE, SCROLL_SIZE,
                  bottom - ROW_TOP - 2 * SCROLL_SIZE},
    };
    const int travel = bar.track.h - SCROLL_SIZE;
    const int offset = (int)(first * (size_t)travel / last);
    bar.thumb = (SDL_Rect){x, bar.track.y + offset, SCROLL_SIZE, SCROLL_SIZE};
    return bar;
}

static void draw_catalog(SDL_Renderer *renderer, const struct theme *theme,
                         const struct content_catalog *catalog, const struct dlc_view *view,
                         enum page page, size_t first, SDL_Point pointer)
{
    const size_t count = visible_count(catalog, view, page);
    if (!count) {
        const char *empty = "No local content found";
        text(renderer, theme, empty, strlen(empty), 794, 170, true);
        return;
    }
    for (size_t i = 0; i < VISIBLE_ROWS && first + i < count; ++i) {
        const int y = ROW_TOP + (int)i * ROW_HEIGHT;
        const struct dlc_row item = visible_row(catalog, view, page, first + i);
        const struct content_entry *entry = item.entry;
        const bool child = page == DLC_PAGE && entry && item.category != EXPANSION;
        const SDL_Rect row = {child ? 640 : 622, y, child ? 314 : 332, ROW_HEIGHT};
        const bool store_visible = entry && !entry->installed && entry->steam_id &&
                                   SDL_PointInRect(&pointer, &row);
        if (!entry) {
            const SDL_Rect frame = {0, 0, 318, 44};
            SDL_RenderCopy(renderer, theme->category_row, &frame, &row);
            const SDL_Rect arrow = {view->collapsed[item.category] ? 26 : 0, 0, 26, 26};
            const SDL_Rect arrow_target = {632, y + 3, 26, 26};
            SDL_RenderCopy(renderer, theme->category_arrow, &arrow, &arrow_target);
        } else {
            SDL_Texture *background = !entry->installed ? theme->unavailable_row :
                                      child ? theme->sub_row : theme->row;
            const SDL_Rect sprite = {0, 3, background == theme->row ? 356 : 318, 38};
            SDL_RenderCopy(renderer, background, &sprite, &row);
            if (entry->icon >= 0 && theme->icons[entry->icon]) {
                const SDL_Rect frame = {entry->enabled ? 0 : 64, 0, 32, 32};
                const SDL_Rect icon = {633, y, 32, 32};
                SDL_RenderCopy(renderer, theme->icons[entry->icon], &frame, &icon);
            }
            if (entry->installed) {
                const SDL_Rect checkbox = {895, y + 4, 24, 24};
                draw_checkbox(renderer, theme, &checkbox, entry->enabled, false, true);
            }
        }

        const int label_x = !entry || entry->icon >= 0 ? 669 : child ? 668 : 640;
        const int end_x = store_visible ? 814 : entry && entry->installed ? 891 : 935;
        const SDL_Rect clip = {label_x, y, end_x - label_x, ROW_HEIGHT};
        SDL_RenderSetClipRect(renderer, &clip);
        SDL_SetTextureColorMod(theme->font, 0, 0, 0);
        const char *name = entry ? entry->name : ck2_category_label(item.category);
        int width = 0;
        size_t length = 0;
        while (name[length]) {
            const int advance = lookup(theme, (unsigned char)name[length])->advance;
            if (width + advance > clip.w - 13)
                break;
            width += advance;
            ++length;
        }
        text(renderer, theme, name, length, label_x, y + 6, false);
        if (name[length])
            text(renderer, theme, "...", 3, label_x + width, y + 6, false);
        SDL_SetTextureColorMod(theme->font, 255, 255, 255);
        SDL_RenderSetClipRect(renderer, NULL);
        if (store_visible) {
            const SDL_Rect store = store_area(y);
            const Uint8 shade = SDL_PointInRect(&pointer, &store) ? 255 : 214;
            SDL_SetTextureColorMod(theme->store, shade, shade, shade);
            SDL_RenderCopy(renderer, theme->store, NULL, &store);
            text(renderer, theme, "View Store", 10, store.x + store.w / 2, y + 5, true);
        }
    }

    if (count > VISIBLE_ROWS) {
        const struct scrollbar bar = scrollbar_layout(first, count - VISIBLE_ROWS);
        SDL_RenderCopy(renderer, theme->scroll_track, NULL, &bar.track);
        SDL_RenderCopy(renderer, theme->scroll_up, NULL, &bar.up);
        SDL_RenderCopy(renderer, theme->scroll_down, NULL, &bar.down);
        SDL_RenderCopy(renderer, theme->scroll_thumb, NULL, &bar.thumb);
    }
}

static void draw(SDL_Renderer *renderer, const struct theme *theme, const char *directory,
                 const struct content_catalog *catalog, const struct dlc_view *view,
                 enum page page, size_t first, enum action focus, enum action hover,
                 SDL_Point pointer, bool content_only)
{
    const SDL_Rect panel = {600, 70, 388, 460};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, theme->background, NULL, NULL);
    SDL_RenderCopy(renderer, theme->panel, NULL, &panel);
    const SDL_Rect badge = {829, 39, 151, 25};
    SDL_SetRenderDrawColor(renderer, 27, 23, 19, 255);
    SDL_RenderFillRect(renderer, &badge);
    SDL_SetRenderDrawColor(renderer, 150, 127, 86, 255);
    SDL_RenderDrawRect(renderer, &badge);
    text(renderer, theme, version, sizeof version - 1, 904, 41, true);
    if (page == SETTINGS_PAGE) {
        text(renderer, theme, "Settings", 8, 794, 87, true);
        const char *title = "Crusader Kings II installation";
        text(renderer, theme, title, strlen(title), 794, 152, true);
        draw_path(renderer, theme, directory);
    } else {
        draw_catalog(renderer, theme, catalog, view, page, first, pointer);
    }

    for (int i = QUIT; i <= PLAY; ++i) {
        const enum action action = (enum action)i;
        if (!visible(page, action, content_only))
            continue;
        const SDL_Rect *area = &button_areas[i];
        if (action == MASTER) {
            const struct content_list *list = page == MODS_PAGE ? &catalog->mods : &catalog->dlc;
            const bool checked = ck2_content_all_enabled(list);
            draw_checkbox(renderer, theme, area, checked,
                          !checked && ck2_content_any_enabled(list),
                          action == hover || action == focus);
            continue;
        }
        if (action == DLC_TAB || action == MODS_TAB) {
            const bool active = (action == DLC_TAB) == (page == DLC_PAGE);
            const SDL_Rect frame = {active ? 160 : 0, 0, 160, 30};
            SDL_Texture *texture = action == hover ? theme->tab_hover : theme->tabs;
            const Uint8 shade = active || action == hover || action == focus ? 255 : 219;
            SDL_SetTextureColorMod(texture, shade, shade, shade);
            SDL_RenderCopy(renderer, texture, &frame, area);
        } else {
            SDL_Texture *texture = action == hover ? theme->hover : theme->button;
            const Uint8 brightness = action == focus ? 255 : 235;
            SDL_SetTextureColorMod(texture, brightness, brightness, brightness);
            SDL_RenderCopy(renderer, texture, NULL, area);
        }
        const char *label = action == PLAY && content_only ? "Done" :
                            action == SETTINGS && page == SETTINGS_PAGE ? "Back" : button_labels[i];
        text(renderer, theme, label, strlen(label),
             area->x + area->w / 2, area->y + (area->h - 22) / 2, true);
    }
    SDL_RenderPresent(renderer);
}

static char *pick_folder(SDL_Window *window, const char *directory, bool *failed)
{
    int output[2];
    *failed = false;
    if (pipe(output) != 0) {
        *failed = true;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Folder chooser", strerror(errno), window);
        return NULL;
    }

    char *start = directory ? ck2_join_path(directory, "") : NULL;
    char *args[] = {"zenity", "--file-selection", "--directory",
                    "--title=Choose the Crusader Kings II installation",
                    "--width=760", "--height=540", NULL, NULL, NULL};
    if (start) {
        args[6] = "--filename";
        args[7] = start;
    }

    posix_spawn_file_actions_t actions;
    int error = posix_spawn_file_actions_init(&actions);
    pid_t child = -1;
    if (!error) {
        error = posix_spawn_file_actions_addclose(&actions, output[0]);
        if (!error)
            error = posix_spawn_file_actions_adddup2(&actions, output[1], STDOUT_FILENO);
        if (!error)
            error = posix_spawn_file_actions_addclose(&actions, output[1]);
        if (!error)
            error = posix_spawnp(&child, "zenity", &actions, NULL, args, environ);
        posix_spawn_file_actions_destroy(&actions);
    }
    close(output[1]);
    free(start);

    if (error) {
        *failed = true;
        close(output[0]);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Folder chooser",
            "Could not open Zenity. Install Zenity or use --assets PATH.", window);
        return NULL;
    }

    FILE *file = fdopen(output[0], "r");
    char *chosen = NULL;
    size_t capacity = 0;
    ssize_t length = file ? getline(&chosen, &capacity, file) : -1;
    if (file)
        fclose(file);
    else
        close(output[0]);

    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0 || length <= 0) {
        free(chosen);
        return NULL;
    }
    if (chosen[length - 1] == '\n')
        chosen[length - 1] = '\0';
    return chosen;
}

static enum choice choose_install(SDL_Window *window, SDL_Renderer *renderer, struct theme *theme,
                           char **directory, const char *settings)
{
    bool failed;
    char *chosen = pick_folder(window, *directory, &failed);
    if (!chosen)
        return failed ? CHOOSER_ERROR : CANCELLED;

    struct theme replacement = {0};
    if (!ck2_install_valid(chosen) || !load_theme(&replacement, renderer, chosen)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CK2 files not found",
            "Choose the Crusader Kings II folder that contains the map, gfx, and launcher folders.", window);
        free_theme(&replacement);
        free(chosen);
        return REJECTED;
    }

    free_theme(theme);
    *theme = replacement;
    free(*directory);
    *directory = chosen;
    if (!ck2_write_install(settings, chosen))
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Game folder",
            "The folder is available for this session. The launcher could not save it for next time.", window);
    return SELECTED;
}

static enum action hit(enum page page, int x, int y, bool content_only)
{
    SDL_Point point = {x, y};
    for (int i = QUIT; i <= PLAY; ++i) {
        if (visible(page, (enum action)i, content_only) && SDL_PointInRect(&point, &button_areas[i]))
            return (enum action)i;
    }
    return NONE;
}

static void scroll_list(size_t count, size_t *first, int amount)
{
    const size_t last = count > VISIBLE_ROWS ? count - VISIBLE_ROWS : 0;
    if (amount < 0)
        *first -= SDL_min(*first, (size_t)-amount);
    else
        *first = SDL_min(last, *first + (size_t)amount);
}

static bool scrollbar_press(size_t count, SDL_Point point,
                            size_t *first, int *drag_offset)
{
    if (count <= VISIBLE_ROWS)
        return false;
    const struct scrollbar bar = scrollbar_layout(*first, count - VISIBLE_ROWS);
    if (SDL_PointInRect(&point, &bar.up))
        scroll_list(count, first, -1);
    else if (SDL_PointInRect(&point, &bar.down))
        scroll_list(count, first, 1);
    else if (SDL_PointInRect(&point, &bar.thumb)) {
        *drag_offset = point.y - bar.thumb.y;
        SDL_CaptureMouse(SDL_TRUE);
    } else if (SDL_PointInRect(&point, &bar.track)) {
        scroll_list(count, first, point.y < bar.thumb.y ? -VISIBLE_ROWS : VISIBLE_ROWS);
    } else {
        return false;
    }
    return true;
}

static void scrollbar_drag(size_t count, int y, int drag_offset,
                           size_t *first)
{
    const size_t last = count - VISIBLE_ROWS;
    const struct scrollbar bar = scrollbar_layout(*first, last);
    const int travel = bar.track.h - SCROLL_SIZE;
    const int offset = SDL_clamp(y - drag_offset - bar.track.y, 0, travel);
    *first = ((size_t)offset * last + (size_t)travel / 2) / (size_t)travel;
}

static bool run_launcher(SDL_Window *window, SDL_Renderer *renderer, struct theme *theme,
                         char **directory, const char *settings, const char *selection, const char *game, bool content_only)
{
    enum action focus = PLAY, hover = NONE;
    enum page page = DLC_PAGE, previous_page = DLC_PAGE;
    size_t first = 0;
    int drag_offset = -1;
    bool scrollbar_pressed = false;
    struct content_catalog catalog = {0};
    struct dlc_view view = {0};
    SDL_Point pointer = {-1, -1};
    ck2_catalog_load(&catalog, *directory);
    ck2_catalog_restore(&catalog, selection);
    rebuild_dlc_view(&view, &catalog.dlc);

    SDL_ShowWindow(window);
    draw(renderer, theme, *directory, &catalog, &view, page, first, focus, hover, pointer, content_only);

    bool play = false;
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        enum action action = NONE;
        bool changed = false;
        const size_t count = visible_count(&catalog, &view, page);
        if (event.type == SDL_QUIT)
            break;
        if (event.type == SDL_MOUSEMOTION) {
            pointer = (SDL_Point){event.motion.x, event.motion.y};
            if (drag_offset >= 0 && page != SETTINGS_PAGE)
                scrollbar_drag(count, event.motion.y, drag_offset, &first);
            else
                hover = hit(page, event.motion.x, event.motion.y, content_only);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT &&
            page != SETTINGS_PAGE) {
            const SDL_Point pointer = {event.button.x, event.button.y};
            scrollbar_pressed = scrollbar_press(count, pointer,
                                                &first, &drag_offset);
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            drag_offset = -1;
            scrollbar_pressed = false;
            SDL_CaptureMouse(SDL_FALSE);
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_LEAVE) {
            hover = NONE;
            pointer = (SDL_Point){-1, -1};
        }
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT &&
            scrollbar_pressed) {
            scrollbar_pressed = false;
            drag_offset = -1;
            SDL_CaptureMouse(SDL_FALSE);
        } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            action = hit(page, event.button.x, event.button.y, content_only);
            const SDL_Rect rows = {622, ROW_TOP, 332, VISIBLE_ROWS * ROW_HEIGHT};
            const SDL_Point pointer = {event.button.x, event.button.y};
            if (action == NONE && page != SETTINGS_PAGE && SDL_PointInRect(&pointer, &rows)) {
                const size_t slot = (size_t)(pointer.y - ROW_TOP) / ROW_HEIGHT;
                const size_t index = first + slot;
                if (index < count) {
                    const struct dlc_row item = visible_row(&catalog, &view, page, index);
                    if (!item.entry) {
                        view.collapsed[item.category] = !view.collapsed[item.category];
                        rebuild_dlc_view(&view, &catalog.dlc);
                        scroll_list(view.count, &first, 0);
                    } else if (item.entry->installed) {
                        item.entry->enabled = !item.entry->enabled;
                        changed = true;
                    } else if (item.entry->steam_id) {
                        const SDL_Rect store = store_area(ROW_TOP + (int)slot * ROW_HEIGHT);
                        if (SDL_PointInRect(&pointer, &store))
                            open_dlc_store(window, item.entry->steam_id);
                    }
                }
            }
        }
        if (event.type == SDL_MOUSEWHEEL && page != SETTINGS_PAGE) {
            int steps = SDL_clamp(event.wheel.y, -10, 10);
            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                steps = -steps;
            scroll_list(count, &first, -steps * 3);
        }
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                if (!event.key.repeat)
                    action = page == SETTINGS_PAGE ? SETTINGS : QUIT;
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (!event.key.repeat)
                    action = focus;
                break;
            case SDLK_TAB: {
                const int direction = event.key.keysym.mod & KMOD_SHIFT ? -1 : 1;
                do {
                    focus = (enum action)(QUIT + (focus - QUIT + direction + PLAY) % PLAY);
                } while (!visible(page, focus, content_only));
                hover = NONE;
                break;
            }
            case SDLK_UP:
            case SDLK_DOWN:
            case SDLK_PAGEUP:
            case SDLK_PAGEDOWN:
                if (page != SETTINGS_PAGE) {
                    int amount = event.key.keysym.sym == SDLK_UP ? -1 :
                                 event.key.keysym.sym == SDLK_DOWN ? 1 :
                                 event.key.keysym.sym == SDLK_PAGEUP ? -VISIBLE_ROWS : VISIBLE_ROWS;
                    scroll_list(count, &first, amount);
                }
                break;
            }
        }

        if (action == MASTER) {
            struct content_list *list = page == MODS_PAGE ? &catalog.mods : &catalog.dlc;
            ck2_content_set_all(list, !ck2_content_all_enabled(list));
            changed = true;
        }
        if (changed && !ck2_catalog_save(&catalog, selection))
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Content selection",
                "The selections could not be saved for next time.", window);
        if (action == QUIT)
            break;
        if (action == DLC_TAB || action == MODS_TAB) {
            drag_offset = -1;
            SDL_CaptureMouse(SDL_FALSE);
            page = action == DLC_TAB ? DLC_PAGE : MODS_PAGE;
            first = 0;
            focus = action;
        }
        if (action == SETTINGS) {
            drag_offset = -1;
            SDL_CaptureMouse(SDL_FALSE);
            if (page == SETTINGS_PAGE) {
                page = previous_page;
                focus = SETTINGS;
            } else {
                previous_page = page;
                page = SETTINGS_PAGE;
                focus = BROWSE;
            }
            hover = NONE;
        }
        if (action == BROWSE &&
            choose_install(window, renderer, theme, directory, settings) == SELECTED) {
            ck2_catalog_free(&catalog);
            ck2_catalog_load(&catalog, *directory);
            ck2_catalog_restore(&catalog, selection);
            rebuild_dlc_view(&view, &catalog.dlc);
            page = DLC_PAGE;
            first = 0;
            focus = PLAY;
            hover = NONE;
        }
        if (action == PLAY) {
            if (content_only)
                break;
            if (!ck2_install_valid(*directory)) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CK2 files not found",
                    "Choose a valid Crusader Kings II installation in Settings.", window);
            } else if (!game || access(game, X_OK) != 0) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenCK2 executable not found",
                    "Build ck2-game and place it beside the launcher.", window);
            } else {
                play = true;
                break;
            }
        }
        draw(renderer, theme, *directory, &catalog, &view, page, first, focus, hover, pointer, content_only);
    }
    free(view.rows);
    ck2_catalog_free(&catalog);
    return play;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: OpenCK2Launcher [--assets CK2_INSTALL_DIRECTORY]");
        return EXIT_SUCCESS;
    }
    const bool content_only = argc == 4 && strcmp(argv[1], "--content-settings") == 0 &&
                              strcmp(argv[2], "--assets") == 0;
    if (!content_only && argc != 1 && (argc != 3 || strcmp(argv[1], "--assets") != 0)) {
        fputs("Usage: OpenCK2Launcher [--assets CK2_INSTALL_DIRECTORY]\n", stderr);
        return EXIT_FAILURE;
    }
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    char *prefs = SDL_GetPrefPath("OpenCK2", "Launcher");
    char *settings = prefs ? ck2_join_path(prefs, "install-path.txt") : NULL;
    char *selection = prefs ? ck2_join_path(prefs, "content-selection.txt") : NULL;
    char *base = SDL_GetBasePath();
    char *game = base ? ck2_join_path(base, "ck2-game") : NULL;
    SDL_free(base);
    SDL_free(prefs);

    char *directory = NULL;
    const char *override = content_only ? argv[3] : argc == 3 ? argv[2] : getenv("CK2_DIR");
    if (override && *override) {
        directory = realpath(override, NULL);
        if (!ck2_install_valid(directory)) {
            fprintf(stderr, "Invalid CK2 installation: %s\n", override);
            free(directory);
            free(selection);
            free(settings);
            free(game);
            SDL_Quit();
            return EXIT_FAILURE;
        }
    } else {
        directory = ck2_read_install(settings);
        if (!directory)
            directory = ck2_find_steam_install();
    }

    int status = EXIT_FAILURE;
    struct theme theme = {0};
    bool play = false;
    SDL_Window *window = SDL_CreateWindow(content_only ? "OpenCK2 - Content Settings" : "OpenCK2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window) {
        SDL_SetWindowMinimumSize(window, WIDTH, HEIGHT);
        SDL_SetWindowMaximumSize(window, WIDTH, HEIGHT);
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : NULL;
    if (window && !renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    if (!renderer || SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT) < 0) {
        fprintf(stderr, "Could not open OpenCK2Launcher: %s\n", SDL_GetError());
    } else {
        enum choice result = directory && load_theme(&theme, renderer, directory)
            ? SELECTED : REJECTED;
        while (result == REJECTED && !content_only)
            result = choose_install(window, renderer, &theme, &directory, settings);
        if (result == SELECTED)
            play = run_launcher(window, renderer, &theme, &directory, settings, selection, game, content_only);
        if (result == SELECTED || result == CANCELLED)
            status = EXIT_SUCCESS;
    }

    free_theme(&theme);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();

    if (play) {
        char *const game_argv[] = {game, "--assets", directory, NULL};
        execv(game, game_argv);
        fprintf(stderr, "Could not start %s: %s\n", game, strerror(errno));
        status = EXIT_FAILURE;
    }

    free(directory);
    free(selection);
    free(settings);
    free(game);
    return status;
}
