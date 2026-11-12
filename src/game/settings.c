#include "settings.h"
#include "ck2_script.h"
#include "localization_core.h"
#include <stdio.h>

#include <string.h>

enum { TAB, LEFT, RIGHT, THUMB, CHECKBOX, SPRITE_COUNT };
enum kind { CHOICE, SLIDER, CHECK };
enum option { DEFERRED, RESOLUTION, WINDOW_MODE, MASTER_VOLUME, MUSIC_VOLUME, EFFECT_VOLUME, LANGUAGE };

struct row {
    const char *label;
    enum kind kind;
    enum option option;
    const char *preview;
    int initial;
};

static const struct row game[] = {
    {"Difficulty", CHOICE, DEFERRED, "Normal", 0},
    {"Autosave Interval", CHOICE, DEFERRED, "Yearly", 0},
    {"Language", CHOICE, LANGUAGE, "English", 0},
    {"Disable Wikipedia Links", CHECK, DEFERRED, NULL, 0},
    {"Disable All Hints", CHECK, DEFERRED, NULL, 1},
    {"Autosave to Cloud", CHECK, DEFERRED, NULL, 0},
    {"Compress Autosaves", CHECK, DEFERRED, NULL, 0},
    {"Steam Rich Presence", CHECK, DEFERRED, NULL, 1},
};
static const struct row video[] = {
    {"Resolution", CHOICE, RESOLUTION, NULL, 0},
    {"Window Mode", CHOICE, WINDOW_MODE, NULL, 0},
    {"Multisample Level", CHOICE, DEFERRED, "0", 0},
    {"Refresh Rate", CHOICE, DEFERRED, "60 Hz", 0},
    {"UI Scale (experimental)", CHOICE, DEFERRED, "1.00", 0},
    {"Gamma", SLIDER, DEFERRED, NULL, 50},
    {"Realm-colored Wastelands", CHECK, DEFERRED, NULL, 1},
    {"Transparent Wastelands", CHECK, DEFERRED, NULL, 1},
    {"VSync", CHECK, DEFERRED, NULL, 1},
    {"Province Embellishment", CHECK, DEFERRED, NULL, 1},
};
static const struct row audio[] = {
    {"Master", SLIDER, MASTER_VOLUME, NULL, 100},
    {"Effect", SLIDER, EFFECT_VOLUME, NULL, 100},
    {"Music", SLIDER, MUSIC_VOLUME, NULL, 100},
    {"Ambient", SLIDER, DEFERRED, NULL, 50},
    {"Dynamic Ambient Sounds", CHECK, DEFERRED, NULL, 0},
    {"Context Sensitive Music", CHECK, DEFERRED, NULL, 1},
    {"Death Sounds", CHECK, DEFERRED, NULL, 1},
};
static const struct row controls[] = {
    {"Scroll Speed", SLIDER, DEFERRED, NULL, 50},
    {"Zoom Speed", SLIDER, DEFERRED, NULL, 50},
    {"Camera Smoothness", SLIDER, DEFERRED, NULL, 50},
    {"Camera Tilt", SLIDER, DEFERRED, NULL, 100},
    {"Zoom in to Cursor", CHECK, DEFERRED, NULL, 1},
};
static const struct {
    const char *label;
    const struct row *rows;
    int count;
} tabs[] = {
    {"Game", game, SDL_arraysize(game)}, {"Video", video, SDL_arraysize(video)},
    {"Audio", audio, SDL_arraysize(audio)}, {"Controls", controls, SDL_arraysize(controls)},
};
static const SDL_Point resolutions[] = {
    {960, 540}, {1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}
};

enum { TAB_COUNT = SDL_arraysize(tabs), TRACK_START = 838, TRACK_END = 928 };
/* Keys correspond to the original CK2 localisation archive. Unsupported rows
 * deliberately keep an English fallback rather than inventing translations. */
static const char *setting_row_key(const char *label)
{
    static const struct { const char *label; const char *key; } mappings[] = {
        {"Difficulty", "SM_DIFFICULTY"},
        {"Autosave Interval", "SM_AUTOSAVE_INTERVAL"},
        {"Language", "SM_LANGUAGE"},
        {"Disable Wikipedia Links", "SM_DISABLEWIKIPEDIA"},
        {"Disable All Hints", "SM_DISABLEHINTS"},
        {"Autosave to Cloud", "SM_AUTOSAVETOCLOUD"},
        {"Compress Autosaves", "SM_COMPRESSAUTOSAVES"},
        {"Steam Rich Presence", "SM_STEAMRICH"},
        {"Resolution", "SM_RESOLUTION"},
        {"Window Mode", "SM_WINDOWMODE"},
        {"Multisample Level", "SM_MULTISAMPLE"},
        {"Refresh Rate", "SM_REFRESH_RATE"},
        {"UI Scale (experimental)", "SM_UI_SCALE"},
        {"Gamma", "SM_GAMMA"},
        {"Master", "SM_MASTER_VOLUME"},
        {"Effect", "SM_EFFECT_VOLUME"},
        {"Music", "SM_MUSIC_VOLUME"},
        {"Scroll Speed", "SM_SCROLL_SPEED"},
        {"Zoom Speed", "SM_ZOOM_SPEED"},
        {"Camera Tilt", "SM_CAMERA_TILT"},
        {"Death Sounds", "SM_DEATH_SOUND"},
        {"Context Sensitive Music", "SM_CONTEXTMUSIC"},
        {"Dynamic Ambient Sounds", "SM_CONTEXTSOUND"},
    };
    for (size_t i = 0; i < SDL_arraysize(mappings); ++i)
        if (strcmp(label, mappings[i].label) == 0) return mappings[i].key;
    return NULL;
}

static const SDL_Rect frame = {580, 140, 440, 630};
static const SDL_Rect heading = {700, 249, 200, 44};
static const SDL_Rect back = {663, 662, 127, 35};
static const SDL_Rect apply = {810, 662, 127, 35};

int settings_load(struct settings *settings, SDL_Renderer *renderer, const char *root)
{
    static const char *const paths[SPRITE_COUNT] = {
        "gfx/interface/settings_tab_new.dds", "gfx/interface/slider_left.dds",
        "gfx/interface/slider_right.dds", "gfx/interface/scrollbar_slider_2.dds",
        "gfx/interface/checkbox_default.dds",
    };
    settings->asset_root = root;
    settings->applied = (struct settings_values){.resolution = 1, .master = 100, .music = 100, .effects = 100};
    /* Language is a user preference, never written into CK2's original files. */
    char *prefs = SDL_GetPrefPath("OpenCK2", "Launcher");
    if (prefs) {
        char filename[1024];
        snprintf(filename, sizeof filename, "%s%s", prefs, "language.txt");
        FILE *saved = fopen(filename, "r");
        int language = 0;
        if (saved) {
            if (fscanf(saved, "%d", &language) == 1 && language >= 0 && language < CK2_LANGUAGE_COUNT)
                settings->applied.language = language;
            fclose(saved);
        }
        SDL_free(prefs);
    }
    if (ck2_locale_reload(root, settings->applied.language) < 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "CK2 localisation not available; UI fallbacks retained");
    for (int i = 0; i < SPRITE_COUNT; ++i) {
        settings->sprites[i] = ck2_load_texture(renderer, root, paths[i]);
        if (!settings->sprites[i])
            return -1;
    }
    return 0;
}

void settings_free(struct settings *settings)
{
    for (int i = 0; i < SPRITE_COUNT; ++i) {
        if (settings->sprites[i])
            SDL_DestroyTexture(settings->sprites[i]);
    }
}

void settings_open(struct settings *settings, SDL_Window *window)
{
    settings->applied.borderless = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
    settings->draft = settings->applied;
    settings->focus = settings->dragging = -1;
    settings->pointer = (SDL_Point){-1, -1};
}

static SDL_Rect tab_area(int index)
{
    return (SDL_Rect){642 + 79 * index, 297, 79, 34};
}

static SDL_Rect row_area(const struct settings *settings, int index)
{
    static const int spacing[] = {33, 30, 38, 48};
    return (SDL_Rect){644, 340 + index * spacing[settings->tab], 312, spacing[settings->tab]};
}

static int row_at(const struct settings *settings, SDL_Point point)
{
    for (int i = 0; i < tabs[settings->tab].count; ++i) {
        SDL_Rect area = row_area(settings, i);
        if (SDL_PointInRect(&point, &area))
            return i;
    }
    return -1;
}

static void label(SDL_Renderer *renderer, const struct menu_assets *assets, settings_text text,
                  const char *value, int x, int y)
{
    text(renderer, assets, value, strlen(value), x, y);
}

static void left_label(SDL_Renderer *renderer, const struct menu_assets *assets, settings_text text,
                       const char *value, int x, int y)
{
    int width = 0;
    for (const unsigned char *c = (const unsigned char *)value; *c; ++c)
        width += assets->glyphs[*c].advance;
    label(renderer, assets, text, value, x + width / 2, y);
}

void settings_unavailable_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets,
                                  settings_text text, SDL_Point pointer)
{
    int width, height;
    SDL_RenderGetLogicalSize(renderer, &width, &height);
    const SDL_Rect box = {SDL_clamp(pointer.x + 14, 0, width - 196),
                          SDL_clamp(pointer.y + 19, 0, height - 34), 192, 30};
    SDL_SetRenderDrawColor(renderer, 22, 17, 17, 255);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 123, 75, 69, 255);
    SDL_RenderDrawRect(renderer, &box);
    SDL_SetTextureColorMod(assets->font, 255, 105, 105);
    label(renderer, assets, text, ck2_localize("NOT_IMPLEMENTED", "Not developed yet"), box.x + box.w / 2, box.y + 4);
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
}

static int slider_value(const struct settings *settings, const struct row *row)
{
    if (row->option == MASTER_VOLUME)
        return settings->draft.master;
    if (row->option == MUSIC_VOLUME)
        return settings->draft.music;
    if (row->option == EFFECT_VOLUME)
        return settings->draft.effects;
    return row->initial;
}

static void draw_row(const struct settings *settings, SDL_Renderer *renderer,
                     const struct menu_assets *assets, settings_text text, int index)
{
    const struct row *row = &tabs[settings->tab].rows[index];
    const SDL_Rect area = row_area(settings, index);
    const int y = area.y + (area.h - 22) / 2;
    const bool stacked = settings->tab == 3 && row->kind == SLIDER;
    const int control_y = stacked ? area.y + 25 : y + 2;
    const int offset = stacked ? -83 : 0;
    SDL_Texture *const *sprites = settings->sprites;
    if (index == settings->focus || SDL_PointInRect(&settings->pointer, &area)) {
        SDL_SetRenderDrawColor(renderer, 57, 55, 59, 255);
        SDL_RenderFillRect(renderer, &area);
    }

    const Uint8 brightness = row->option == DEFERRED ? 176 : 255;
    SDL_SetTextureColorMod(assets->font, brightness, brightness, brightness);
    const char *key = setting_row_key(row->label);
    const char *localized = key ? ck2_localize(key, row->label) : row->label;
    if (stacked)
        label(renderer, assets, text, localized, 800, area.y);
    else
        left_label(renderer, assets, text, localized, row->kind == CHECK ? 680 : 650, y);
    if (row->kind == CHECK) {
        const SDL_Rect sprite = {row->initial * 24, 0, 24, 24};
        SDL_RenderCopy(renderer, sprites[CHECKBOX], &sprite, &(SDL_Rect){650, y - 1, 24, 24});
    } else if (row->kind == SLIDER) {
        const int thumb = TRACK_START + offset +
            slider_value(settings, row) * (TRACK_END - TRACK_START) / 100;
        SDL_SetRenderDrawColor(renderer, 12, 12, 16, 255);
        SDL_RenderFillRect(renderer, &(SDL_Rect){TRACK_START + offset, control_y + 4,
                                               TRACK_END - TRACK_START, 12});
        SDL_RenderCopy(renderer, sprites[THUMB], &(SDL_Rect){0, 0, 16, 16},
                       &(SDL_Rect){thumb - 9, control_y, 18, 18});
    } else {
        const char *value = row->preview;
        char dimensions[24];
        if (row->option == RESOLUTION) {
            const SDL_Point size = resolutions[settings->draft.resolution];
            SDL_snprintf(dimensions, sizeof dimensions, "%dx%d", size.x, size.y);
            value = dimensions;
        } else if (row->option == WINDOW_MODE) {
            value = settings->draft.borderless ? "Borderless" : "Windowed";
        } else if (row->option == LANGUAGE) {
            value = loc_language_name((enum ck2_language)settings->draft.language);
        }
        label(renderer, assets, text, value, 883, y);
    }
    if (row->kind != CHECK) {
        SDL_RenderCopy(renderer, sprites[LEFT], NULL,
                       &(SDL_Rect){TRACK_START - 21 + offset, control_y, 19, 19});
        SDL_RenderCopy(renderer, sprites[RIGHT], NULL,
                       &(SDL_Rect){TRACK_END + 2 + offset, control_y, 19, 19});
    }
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
}

void settings_draw(const struct settings *settings, SDL_Renderer *renderer,
                   const struct menu_assets *assets, settings_text text)
{
    SDL_RenderCopy(renderer, assets->dialog_frame, NULL, &frame);
    SDL_RenderCopy(renderer, assets->dialog_heading, NULL, &heading);
    label(renderer, assets, text, ck2_localize("SM_SETTINGS", "Settings"), 800, 260);
    for (int i = 0; i < TAB_COUNT; ++i) {
        const SDL_Rect area = tab_area(i);
        const SDL_Rect sprite = {i == settings->tab ? 64 : 0, 0, 64, 32};
        SDL_RenderCopy(renderer, settings->sprites[TAB], &sprite, &area);
        static const char *const tab_keys[] = {"SM_GAME", "SM_VIDEO", "SM_AUDIO", "SM_CONTROLS"};
        label(renderer, assets, text, ck2_localize(tab_keys[i], tabs[i].label), area.x + area.w / 2, area.y + 7);
    }

    for (int i = 0; i < tabs[settings->tab].count; ++i)
        draw_row(settings, renderer, assets, text, i);
    SDL_RenderCopy(renderer, SDL_PointInRect(&settings->pointer, &back) ?
                   assets->compact_hover : assets->compact_button, NULL, &back);
    SDL_RenderCopy(renderer, SDL_PointInRect(&settings->pointer, &apply) ?
                   assets->compact_hover : assets->compact_button, NULL, &apply);
    label(renderer, assets, text, ck2_localize("SM_BACK", "Back"), back.x + back.w / 2, back.y + 6);
    label(renderer, assets, text, ck2_localize("SM_APPLY", "Apply"), apply.x + apply.w / 2, apply.y + 6);

    const int hovered = row_at(settings, settings->pointer);
    if (hovered >= 0 && tabs[settings->tab].rows[hovered].option == DEFERRED)
        settings_unavailable_tooltip(renderer, assets, text, settings->pointer);
}

static void apply_settings(struct settings *settings, SDL_Window *window,
                           const struct menu_music *music)
{
    const struct settings_values draft = settings->draft;
    const struct settings_values applied = settings->applied;
    if (draft.borderless != applied.borderless &&
        SDL_SetWindowFullscreen(window, draft.borderless ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Display mode", SDL_GetError(), window);
        return;
    }
    if (!draft.borderless &&
        (draft.resolution != applied.resolution || draft.borderless != applied.borderless)) {
        const SDL_Point size = resolutions[draft.resolution];
        SDL_SetWindowSize(window, size.x, size.y);
    }
    menu_music_set_volume(music, draft.master, draft.music, draft.effects);
    if (draft.language != applied.language) {
        if (ck2_locale_reload(settings->asset_root, draft.language) < 0) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Language", "The selected CK2 localisation was not found", window);
            return;
        }
        char *prefs = SDL_GetPrefPath("OpenCK2", "Launcher");
        if (prefs) {
            char filename[1024];
            snprintf(filename, sizeof filename, "%s%s", prefs, "language.txt");
            FILE *saved = fopen(filename, "w");
            if (saved) {fprintf(saved, "%d\n", draft.language); fclose(saved);}
            SDL_free(prefs);
        }
    }
    settings->applied = draft;
}

static int *volume_for(struct settings *settings, int index)
{
    const enum option option = tabs[settings->tab].rows[index].option;
    if (option == MASTER_VOLUME)
        return &settings->draft.master;
    if (option == MUSIC_VOLUME)
        return &settings->draft.music;
    if (option == EFFECT_VOLUME)
        return &settings->draft.effects;
    return NULL;
}

static void adjust_row(struct settings *settings, int index, int direction)
{
    int *volume = volume_for(settings, index);
    switch (tabs[settings->tab].rows[index].option) {
    case RESOLUTION:
        settings->draft.resolution = SDL_clamp(settings->draft.resolution + direction,
                                                0, (int)SDL_arraysize(resolutions) - 1);
        break;
    case WINDOW_MODE:
        settings->draft.borderless = direction > 0;
        break;
    case LANGUAGE:
        settings->draft.language = (settings->draft.language + direction + CK2_LANGUAGE_COUNT) % CK2_LANGUAGE_COUNT;
        break;
    case MASTER_VOLUME:
    case MUSIC_VOLUME:
    case EFFECT_VOLUME:
        *volume = SDL_clamp(*volume + direction * 5, 0, 100);
        break;
    case DEFERRED:
        break;
    }
}

static void change_tab(struct settings *settings, int tab)
{
    settings->tab = tab;
    settings->focus = settings->dragging = -1;
}

static void click_row(struct settings *settings, int x, int index)
{
    settings->focus = index;
    int *volume = volume_for(settings, index);
    if (x >= TRACK_START && x <= TRACK_END && volume) {
        settings->dragging = index;
        *volume = (x - TRACK_START) * 100 / (TRACK_END - TRACK_START);
    } else if (x >= TRACK_START - 21 && x < TRACK_START) {
        adjust_row(settings, index, -1);
    } else if (x > TRACK_END && x <= TRACK_END + 21) {
        adjust_row(settings, index, 1);
    }
}

bool settings_event(struct settings *settings, SDL_Window *window, struct menu_music *music,
                    const SDL_Event *event)
{
    switch (event->type) {
    case SDL_WINDOWEVENT:
        if (event->window.event == SDL_WINDOWEVENT_LEAVE)
            settings->pointer = (SDL_Point){-1, -1};
        if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            settings->dragging = -1;
        break;
    case SDL_MOUSEMOTION:
        settings->pointer = (SDL_Point){event->motion.x, event->motion.y};
        if (settings->dragging >= 0) {
            int *volume = volume_for(settings, settings->dragging);
            const int x = SDL_clamp(event->motion.x, TRACK_START, TRACK_END);
            *volume = (x - TRACK_START) * 100 / (TRACK_END - TRACK_START);
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            settings->pointer = (SDL_Point){event->button.x, event->button.y};
            const int row = row_at(settings, settings->pointer);
            if (row >= 0) {
                if (tabs[settings->tab].rows[row].option != DEFERRED)
                    menu_sound_play(music, CUE_GENERIC);
                click_row(settings, settings->pointer.x, row);
            }
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            const bool dragging = settings->dragging >= 0;
            settings->dragging = -1;
            if (dragging)
                break;
            settings->pointer = (SDL_Point){event->button.x, event->button.y};
            for (int i = 0; i < TAB_COUNT; ++i) {
                const SDL_Rect area = tab_area(i);
                if (SDL_PointInRect(&settings->pointer, &area)) {
                    change_tab(settings, i);
                    menu_sound_play(music, CUE_MENU);
                }
            }
            if (SDL_PointInRect(&settings->pointer, &back))
                return true;
            if (SDL_PointInRect(&settings->pointer, &apply)) {
                apply_settings(settings, window, music);
                menu_sound_play(music, CUE_MENU);
            }
        }
        break;
    case SDL_KEYDOWN: {
        const SDL_Keycode key = event->key.keysym.sym;
        switch (key) {
        case SDLK_ESCAPE:
            return !event->key.repeat;
        case SDLK_TAB: {
            const int direction = event->key.keysym.mod & KMOD_SHIFT ? -1 : 1;
            change_tab(settings, (settings->tab + direction + TAB_COUNT) % TAB_COUNT);
            break;
        }
        case SDLK_UP:
        case SDLK_DOWN:
            settings->focus = SDL_clamp(settings->focus + (key == SDLK_UP ? -1 : 1),
                                         0, tabs[settings->tab].count - 1);
            break;
        case SDLK_LEFT:
        case SDLK_RIGHT:
            if (settings->focus >= 0)
                adjust_row(settings, settings->focus, key == SDLK_LEFT ? -1 : 1);
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!event->key.repeat)
                apply_settings(settings, window, music);
            break;
        }
        break;
    }
    }
    return false;
}
