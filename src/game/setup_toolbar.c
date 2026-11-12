#include "setup_toolbar.h"
#include "assets.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const names[SETUP_WIDGET_COUNT] = {
    [SETUP_TERRAIN] = "mapmode_terrain",
    [SETUP_REALMS] = "mapmode_realms",
    [SETUP_COUNTIES] = "mapmode_defacto_counties",
    [SETUP_DUCHIES] = "mapmode_defacto_duchies",
    [SETUP_KINGDOMS] = "mapmode_defacto_kingdoms",
    [SETUP_RELIGION] = "mapmode_religion",
    [SETUP_CULTURE] = "mapmode_culture",
    [SETUP_GOVERNMENTS] = "mapmode_governments",
    [SETUP_SHATTERED] = "alternate_start_shattered_button",
    [SETUP_RANDOM] = "alternate_start_random_button",
    [SETUP_LEFT_ORNAMENT] = "shattered_ornament",
    [SETUP_RIGHT_ORNAMENT] = "random_ornament",
    [SETUP_BANNER] = "frontend_banner1",
};

struct gfx_source {
    char *data;
    size_t priority;
};

static const char *const gfx_files[] = {
    "interface/main.gfx", "interface/frontend.gfx", "interface/alternate_start.gfx",
    "interface/generalstuff.gfx", "interface/domestic_dynastic.gfx", "interface/wonder.gfx",
};

static bool integer(struct token t, int *result)
{
    char value[24], *end;
    if (!t.size || t.size >= sizeof value)
        return false;
    copy_token(value, sizeof value, t);
    const long n = strtol(value, &end, 10);
    if (*end || n < -32768 || n > 32767)
        return false;
    *result = (int)n;
    return true;
}

/* Restrict the lookup to direct children once the containing window is found. */
static const char *named_block(const char *scope, struct token name, unsigned depth)
{
    if (!depth)
        return NULL;
    for (struct token key; (key = token(&scope)).size && !is(key, "}");) {
        if (!is(token(&scope), "="))
            return NULL;
        if (!is(token(&scope), "{"))
            continue;
        const struct token label = field(scope, "name");
        if (label.size == name.size && label.size && !memcmp(label.start, name.start, label.size))
            return scope;
        const char *child = named_block(scope, name, depth - 1);
        if (child)
            return child;
        skip_block(&scope);
    }
    return NULL;
}

static SDL_Texture *load_sprite(SDL_Renderer *renderer, const char *root, const char *path)
{
    char *local = ck2_asset_path(root, path);
    /* CK2 3.3.5.1 ships some declared TGA sprites only as same-stem DDS files. */
    FILE *file = local ? fopen(local, "rb") : NULL;
    const bool missing = !local || (!file && errno == ENOENT);
    if (file)
        fclose(file);
    free(local);
    const size_t size = strlen(path);
    if (missing && size >= 4 && !SDL_strcasecmp(path + size - 4, ".tga")) {
        char dds[256];
        SDL_strlcpy(dds, path, sizeof dds);
        memcpy(dds + size - 4, ".dds", 5);
        return ck2_load_texture(renderer, root, dds);
    }
    return ck2_load_texture(renderer, root, path);
}

static int load_sprite_definition(struct setup_widget *widget, SDL_Renderer *renderer, const char *root,
                                  const char *definition, struct token sprite, const char *name, SDL_Rect area)
{
    struct token texture = field(definition, "texturefile");
    if (!texture.size)
        texture = field(definition, "textureFile");
    if (!texture.size && is(sprite, "GFX_difficulty_progress"))
        texture = field(definition, "textureFile2");
    char path[256];
    if (!texture.size || texture.size >= sizeof path)
        return SDL_SetError("CK2 sprite '%.*s': missing or invalid texture path",
                            (int)sprite.size, sprite.start);
    copy_token(path, sizeof path, texture);
    for (char *p = path; *p; ++p)
        if (*p == '\\')
            *p = '/';

    const struct token frames = field(definition, "noOfFrames");
    widget->frames = 1;
    if (frames.size && (!integer(frames, &widget->frames) || widget->frames < 1))
        return SDL_SetError("CK2 sprite '%.*s': invalid frame count", (int)sprite.size, sprite.start);
    widget->texture = load_sprite(renderer, root, path);
    if (!widget->texture)
        return SDL_SetError("CK2 toolbar widget %s: could not load %s", name, path);
    int sheet_width, height;
    if (SDL_QueryTexture(widget->texture, NULL, NULL, &sheet_width, &height) < 0 ||
        sheet_width < widget->frames || sheet_width % widget->frames)
        return SDL_SetError("CK2 toolbar widget %s: invalid sprite sheet dimensions", name);
    widget->area = (SDL_Rect){area.x, area.y,
                             sheet_width / widget->frames, height};
    const char *size = definition;
    if (is(sprite, "GFX_difficulty_progress") && is(find(&size, "size"), "{")) {
        if (!integer(field(size, "x"), &widget->area.w) || !integer(field(size, "y"), &widget->area.h) ||
            widget->area.w <= 0 || widget->area.h <= 0)
            return SDL_SetError("Invalid CK2 difficulty progress dimensions");
    }
    return 0;
}

int setup_widget_load_sprite(struct setup_widget *widget, SDL_Renderer *renderer, const char *root,
                            const char *file, const char *name, SDL_Rect area)
{
    char *data = load_data(root, file);
    const char *scope = data;
    const struct token sprite = {name, strlen(name)};
    const char *definition = scope && is(find(&scope, "spriteTypes"), "{") ? named_block(scope, sprite, 1) : NULL;
    const int status = definition ? load_sprite_definition(widget, renderer, root, definition, sprite, name, area)
                                  : SDL_SetError("%s: missing sprite %s", file, name);
    SDL_free(data);
    return status;
}

static int load_widget(struct setup_widget *widget, SDL_Renderer *renderer, const char *root,
                       const char *gui, const struct gfx_source gfx[], const char *name, SDL_Rect area)
{
    struct token sprite = field(gui, "quadTextureSprite");
    if (!sprite.size)
        sprite = field(gui, "spriteType");
    const char *definition = NULL;
    size_t priority = 0;
    for (size_t i = 0; i < SDL_arraysize(gfx_files); ++i) {
        const char *scope = gfx[i].data;
        if (!scope || (definition && gfx[i].priority <= priority) ||
            !is(find(&scope, "spriteTypes"), "{"))
            continue;
        const char *candidate = named_block(scope, sprite, 1);
        if (candidate) {
            definition = candidate;
            priority = gfx[i].priority;
        }
    }
    if (!definition)
        return SDL_SetError("interface/frontend.gui: missing sprite '%.*s' for %s",
                            (int)sprite.size, sprite.size ? sprite.start : "", name);
    if (load_sprite_definition(widget, renderer, root, definition, sprite, name, area) < 0)
        return -1;
    const struct token shortcut = field(gui, "shortcut");
    if (shortcut.size) {
        char key[32];
        copy_token(key, sizeof key, shortcut);
        widget->shortcut = SDL_GetKeyFromName(key);
    }
    return 0;
}

int setup_widget_load_node(struct setup_widget *widget, SDL_Renderer *renderer, const char *root,
                           const struct frontend_gui *gui, int node, SDL_Rect area)
{
    if (node < 0)
        return SDL_SetError("Missing CK2 widget");
    struct gfx_source gfx[SDL_arraysize(gfx_files)] = {0};
    for (size_t i = 0; i < SDL_arraysize(gfx); ++i) {
        char *path = ck2_asset_path_with_priority(root, gfx_files[i], &gfx[i].priority);
        if (path)
            gfx[i].data = SDL_LoadFile(path, NULL);
        free(path);
    }
    char name[80];
    copy_token(name, sizeof name, gui->blocks[node].name);
    const int status = load_widget(widget, renderer, root, gui->blocks[node].body, gfx, name, area);
    for (size_t i = 0; i < SDL_arraysize(gfx); ++i)
        SDL_free(gfx[i].data);
    return status;
}

struct lobby_binding {
    const char *path;
    bool sprite;
};

static const struct lobby_binding lobby_bindings[LOBBY_ELEMENT_COUNT] = {
    [LOBBY_LEFT] = {"left_window/left_sidepanel", true},
    [LOBBY_CHARACTERS] = {"left_window/lobby_int_chars", true},
    [LOBBY_RIGHT] = {"right_window/right_sidepanel_char", true},
    [LOBBY_BOTTOM] = {"bottom_window/chatpanel", true},
    [LOBBY_LOAD] = {"left_window/savedgame_tab", true},
    [LOBBY_BACK] = {"bottom_window/back_button", true},
    [LOBBY_PLAY] = {"bottom_window/play_button", true},
    [LOBBY_DIFFICULTY] = {"right_window/singleplayer/difficulty", true},
    [LOBBY_DIFFICULTY_LABEL] = {"right_window/singleplayer/difficulty_label", false},
    [LOBBY_DIPLOMACY_LABEL] = {"right_window/singleplayer/diplo_status_label", false},
    [LOBBY_BOOKMARKS] = {"left_window/bookmarks/bookmarks_list", false},
    [LOBBY_FEATURED] = {"left_window/bookmarks/Chars_Listbox", false},
    [LOBBY_VASSALS] = {"right_window/singleplayer/Vassals_Listbox", false},
    [LOBBY_BOOKMARK_ROW] = {"/bookmark_entry", false},
    [LOBBY_BOOKMARK_ART] = {"/bookmark_entry/bookmark", true},
    [LOBBY_BOOKMARK_TITLE] = {"/bookmark_entry/title", false},
    [LOBBY_BOOKMARK_DATE] = {"/bookmark_entry/date", false},
    [LOBBY_CHARACTER_ROW] = {"/character_list_entry", false},
    [LOBBY_BOOKMARK_LABEL] = {"left_window/bookmarks/bookmarks_label", false},
    [LOBBY_FEATURED_LABEL] = {"left_window/interesting_characters_label", false},
    [LOBBY_VASSALS_LABEL] = {"right_window/singleplayer/vassals_label", false},
    [LOBBY_VASSALS_AMOUNT] = {"right_window/singleplayer/vassals_amount", false},
    [LOBBY_RULER] = {"right_window/singleplayer/selected_ruler", false},
    [LOBBY_PORTRAIT] = {"right_window/singleplayer/player_portrait", true},
    [LOBBY_DIFFICULTY_TRACK] = {"right_window/singleplayer/difficultybar", true},
    [LOBBY_PORTRAIT_FRAME] = {"right_window/singleplayer/player_portrait_frame", true},
    [LOBBY_CROWN] = {"right_window/singleplayer/player_crown", true},
    [LOBBY_SHIELD] = {"right_window/singleplayer/player_shield", false},
    [LOBBY_ROW_PORTRAIT] = {"/character_list_entry/portrait", true},
    [LOBBY_ROW_FRAME] = {"/character_list_entry/portrait_frame", true},
    [LOBBY_ROW_SHIELD] = {"/character_list_entry/shield", false},
    [LOBBY_ROW_NAME] = {"/character_list_entry/name", false},
    [LOBBY_DYNASTY] = {"right_window/singleplayer/selected_dynasty", false},
    [LOBBY_TITLE] = {"right_window/singleplayer/selected_nation_label", false},
    [LOBBY_STATUS] = {"right_window/singleplayer/selected_nation_status_label", false},
    [LOBBY_CULTURE] = {"right_window/singleplayer/selected_culture", false},
    [LOBBY_RELIGION] = {"right_window/singleplayer/selected_religion", false},
    [LOBBY_DESCRIPTION] = {"bottom_window/scenario_description", false},
    [LOBBY_EXPANSIONS] = {"bottom_window/expansion_list_subwindow/expansion_list", false},
    [LOBBY_EXPANSION_ROW] = {"/lobby_expansion_entry", false},
    [LOBBY_PROMPT] = {"top_window/datewidget/select_label", false},
    [LOBBY_YEAR] = {"top_window/datewidget/year", false},
    [LOBBY_DAYMONTH] = {"top_window/datewidget/daymonth", false},
    [LOBBY_YEAR_UP1] = {"top_window/datewidget/year_up1", true},
    [LOBBY_YEAR_DOWN1] = {"top_window/datewidget/year_down1", true},
    [LOBBY_YEAR_UP2] = {"top_window/datewidget/year_up2", true},
    [LOBBY_YEAR_DOWN2] = {"top_window/datewidget/year_down2", true},
    [LOBBY_YEAR_UP3] = {"top_window/datewidget/year_up3", true},
    [LOBBY_YEAR_DOWN3] = {"top_window/datewidget/year_down3", true},
    [LOBBY_MONTH_UP] = {"top_window/datewidget/month_up", true},
    [LOBBY_MONTH_DOWN] = {"top_window/datewidget/month_down", true},
    [LOBBY_DAY_UP] = {"top_window/datewidget/day_up", true},
    [LOBBY_DAY_DOWN] = {"top_window/datewidget/day_down", true},
};

static int find_path(const struct frontend_gui *gui, int parent, const char *path)
{
    if (*path == '/') {
        parent = -1;
        ++path;
    }
    do {
        const char *slash = strchr(path, '/');
        if (!slash)
            return frontend_gui_find(gui, parent, path);
        char name[64];
        copy_token(name, sizeof name, (struct token){path, (size_t)(slash - path)});
        parent = frontend_gui_find(gui, parent, name);
        path = slash + 1;
    } while (parent >= 0);
    return -1;
}

static int load_scrollbar(struct setup_toolbar *toolbar, SDL_Renderer *renderer, const char *root,
                          const struct gfx_source gfx[], const char *list)
{
    char *document = load_data(root, "interface/EU3_core.gui");
    const struct token type = field(list, "scrollbartype");
    const char *definition = document ? named_block(document, type, 8) : NULL;
    static const char *const keys[SETUP_SCROLL_PARTS] = {"leftbutton", "rightbutton", "track", "slider"};
    int result = definition ? 0 : SDL_SetError("Could not resolve the CK2 vassal scrollbar");
    for (int i = 0; i < SETUP_SCROLL_PARTS && result == 0; ++i) {
        const char *part = named_block(definition, field(definition, keys[i]), 1);
        if (part)
            result = load_widget(&toolbar->scroll[i], renderer, root, part, gfx, keys[i], (SDL_Rect){0});
        else
            result = SDL_SetError("Missing CK2 vassal scrollbar part %s", keys[i]);
    }
    SDL_free(document);
    return result;
}

static int load_setup(struct setup_toolbar *out, SDL_Renderer *renderer, const char *root,
                      SDL_Point viewport, bool full)
{
    struct setup_toolbar next = {0};
    const bool reuse_frontend = full && out->frontend.source;
    if (!reuse_frontend && frontend_gui_load(&next.frontend, root) < 0)
        return -1;
    const struct frontend_gui *gui = reuse_frontend ? &out->frontend : &next.frontend;
    const int lobby = frontend_gui_find(gui, -1, "lobby");
    const int toolbar = frontend_gui_find(gui, lobby, "top_window");
    if (toolbar < 0) {
        setup_toolbar_free(&next);
        return SDL_SetError("interface/frontend.gui: missing top_window");
    }
    struct gfx_source gfx[SDL_arraysize(gfx_files)] = {0};
    for (size_t i = 0; i < SDL_arraysize(gfx); ++i) {
        char *path = ck2_asset_path_with_priority(root, gfx_files[i], &gfx[i].priority);
        if (path)
            gfx[i].data = SDL_LoadFile(path, NULL);
        free(path);
    }
    int status = 0;
    for (size_t i = 0; i < SDL_arraysize(names) && status == 0; ++i) {
        const int node = frontend_gui_find(gui, toolbar, names[i]);
        SDL_Rect area;
        if (!frontend_gui_rect(gui, node, viewport, &area))
            status = SDL_SetError("interface/frontend.gui: missing widget or invalid position/orientation for %s", names[i]);
        else
            status = load_widget(&next.widgets[i], renderer, root, gui->blocks[node].body,
                                 gfx, names[i], area);
    }

    struct ck2_translation labels[] = {
        {NULL, next.bookmarks_label, sizeof next.bookmarks_label},
        {NULL, next.featured_label, sizeof next.featured_label},
        {NULL, next.vassals_label, sizeof next.vassals_label},
        {NULL, next.load_label, sizeof next.load_label},
        {NULL, next.prompt, sizeof next.prompt},
        {NULL, next.back_label, sizeof next.back_label},
        {NULL, next.play_label, sizeof next.play_label},
        {NULL, next.difficulty_label, sizeof next.difficulty_label},
        {NULL, next.diplomacy_label, sizeof next.diplomacy_label}
    };
    char keys[SDL_arraysize(labels)][96] = {{0}};
    for (size_t i = 0; full && i < SDL_arraysize(lobby_bindings) && status == 0; ++i) {
        const struct lobby_binding *binding = &lobby_bindings[i];
        const int node = find_path(gui, lobby, binding->path);
        SDL_Rect area;
        if (!frontend_gui_rect(gui, node, viewport, &area)) {
            status = SDL_SetError("interface/frontend.gui: invalid lobby widget %s", binding->path);
            break;
        }
        const char *body = gui->blocks[node].body;
        next.lobby[i].area = area;
        if (i == LOBBY_EXPANSIONS)
            integer(field(body, "spacing"), &next.expansion_spacing);
        if (i == LOBBY_VASSALS)
            status = load_scrollbar(&next, renderer, root, gfx, body);
        if (binding->sprite)
            status = load_widget(&next.lobby[i], renderer, root, body, gfx, binding->path, area);
        const int label = i == LOBBY_BOOKMARK_LABEL ? 0 : i == LOBBY_FEATURED_LABEL ? 1
            : i == LOBBY_VASSALS_LABEL ? 2 : i == LOBBY_LOAD ? 3 : i == LOBBY_PROMPT ? 4 :
              i == LOBBY_BACK ? 5 : i == LOBBY_PLAY ? 6 : i == LOBBY_DIFFICULTY_LABEL ? 7 :
              i == LOBBY_DIPLOMACY_LABEL ? 8 : -1;
        if (label >= 0) {
            copy_token(keys[label], sizeof keys[label], field(body, i == LOBBY_LOAD || i == LOBBY_BACK || i == LOBBY_PLAY ? "buttonText" : "text"));
            labels[label].key = keys[label];
            SDL_strlcpy(labels[label].text, keys[label], labels[label].capacity);
        }
    }
    if (full && status == 0) {
        ck2_translate(root, labels, SDL_arraysize(labels));
        next.detail_font = ck2_load_texture(renderer, root, "gfx/fonts/Arial12.tga");
        if (!next.detail_font ||
            ck2_load_font_metrics(next.detail_glyphs, root, "gfx/fonts/Arial12.fnt") < 0)
            status = SDL_SetError("Could not load the CK2 lobby detail font");
    }

    for (size_t i = 0; i < SDL_arraysize(gfx); ++i)
        SDL_free(gfx[i].data);
    if (status < 0) {
        setup_toolbar_free(&next);
        return status;
    }
    if (reuse_frontend) {
        next.frontend = out->frontend;
        out->frontend = (struct frontend_gui){0};
    }
    setup_toolbar_free(out);
    *out = next;
    return 0;
}

int setup_toolbar_load(struct setup_toolbar *out, SDL_Renderer *renderer, const char *root, int width)
{
    return load_setup(out, renderer, root, (SDL_Point){width, 0}, false);
}

int setup_lobby_load(struct setup_toolbar *out, SDL_Renderer *renderer, const char *root, SDL_Point viewport)
{
    return load_setup(out, renderer, root, viewport, true);
}

void setup_toolbar_free(struct setup_toolbar *toolbar)
{
    for (size_t i = 0; i < SDL_arraysize(toolbar->widgets); ++i)
        if (toolbar->widgets[i].texture)
            SDL_DestroyTexture(toolbar->widgets[i].texture);
    for (size_t i = 0; i < SDL_arraysize(toolbar->lobby); ++i)
        if (toolbar->lobby[i].texture)
            SDL_DestroyTexture(toolbar->lobby[i].texture);
    for (size_t i = 0; i < SDL_arraysize(toolbar->scroll); ++i)
        if (toolbar->scroll[i].texture)
            SDL_DestroyTexture(toolbar->scroll[i].texture);
    if (toolbar->detail_font)
        SDL_DestroyTexture(toolbar->detail_font);
    frontend_gui_free(&toolbar->frontend);
    *toolbar = (struct setup_toolbar){0};
}

void setup_widget_draw(const struct setup_widget *w, SDL_Renderer *renderer, SDL_Point pointer)
{
    const bool hover = w->frames > 1 && SDL_PointInRect(&pointer, &w->area);
    const SDL_Rect source = {hover ? w->area.w : 0, 0, w->area.w, w->area.h};
    SDL_RenderCopy(renderer, w->texture, &source, &w->area);
}

bool setup_lobby_deferred(const struct setup_toolbar *toolbar, SDL_Point pointer)
{
    if (SDL_PointInRect(&pointer, &toolbar->lobby[LOBBY_LOAD].area))
        return true;
    for (int i = LOBBY_YEAR_UP1; i <= LOBBY_DAY_DOWN; ++i)
        if (SDL_PointInRect(&pointer, &toolbar->lobby[i].area))
            return true;
    return false;
}

void setup_toolbar_draw(const struct setup_toolbar *toolbar, SDL_Renderer *renderer, SDL_Point pointer)
{
    static const enum setup_control art[] = {
        SETUP_LEFT_ORNAMENT, SETUP_RIGHT_ORNAMENT, SETUP_SHATTERED, SETUP_RANDOM, SETUP_BANNER,
        SETUP_TERRAIN, SETUP_REALMS, SETUP_COUNTIES, SETUP_DUCHIES, SETUP_KINGDOMS,
        SETUP_RELIGION, SETUP_CULTURE, SETUP_GOVERNMENTS,
    };
    for (size_t i = 0; i < SDL_arraysize(art); ++i)
        setup_widget_draw(&toolbar->widgets[art[i]], renderer, pointer);
}

enum setup_control setup_toolbar_hit(const struct setup_toolbar *toolbar, SDL_Point point)
{
    for (int i = SETUP_TERRAIN; i <= SETUP_RANDOM; ++i)
        if (SDL_PointInRect(&point, &toolbar->widgets[i].area))
            return (enum setup_control)i;
    return SETUP_NO_CONTROL;
}

enum setup_control setup_toolbar_shortcut(const struct setup_toolbar *toolbar, SDL_Keycode key)
{
    if (key != SDLK_UNKNOWN)
        for (int i = SETUP_TERRAIN; i <= SETUP_REALMS; ++i)
            if (toolbar->widgets[i].shortcut == key)
                return (enum setup_control)i;
    return SETUP_NO_CONTROL;
}
