#include "../open_launcher/asset_paths.h"
#include "era.h"
#include "ck2_script.h"
#include "portraits.h"
#include "../open_launcher/install.h"

#include <SDL_image.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PANEL_X = 276, PANEL_Y = 84 };

static void load_ruler(struct era_ruler *ruler, const char *p)
{
    ruler->id = number(field(p, "id"));
    ruler->age = number(field(p, "age"));
    copy_token(ruler->name_key, sizeof ruler->name_key, field(p, "name"));
    copy_token(ruler->title_name_key, sizeof ruler->title_name_key, field(p, "title_name"));
    copy_token(ruler->title, sizeof ruler->title, field(p, "title"));
    if (is(find(&p, "character"), "{")) {
        copy_token(ruler->dna, sizeof ruler->dna, field(p, "dna"));
        copy_token(ruler->properties, sizeof ruler->properties, field(p, "properties"));
        copy_token(ruler->culture, sizeof ruler->culture, field(p, "culture"));
        copy_token(ruler->religion, sizeof ruler->religion, field(p, "religion"));
    }
}

static int load_bookmarks(struct era_menu *menu, const char *root)
{
    char *data = load_data(root, "common/bookmarks/00_bookmarks.txt");
    if (!data)
        return -1;
    const char *p = data;
    while (menu->count < ERA_COUNT && token(&p).size && is(token(&p), "=") && is(token(&p), "{")) {
        const char *bookmark = p;
        skip_block(&p);
        if (!is(field(bookmark, "era"), "yes"))
            continue;
        struct starting_era *era = &menu->eras[menu->count++];
        copy_token(era->name_key, sizeof era->name_key, field(bookmark, "name"));
        copy_token(era->description_key, sizeof era->description_key, field(bookmark, "desc"));
        char date[32], picture[64];
        copy_token(date, sizeof date, field(bookmark, "date"));
        copy_token(picture, sizeof picture, field(bookmark, "picture"));
        if (sscanf(date, "%d.%d.%d", &era->date.year, &era->date.month, &era->date.day) != 3 ||
            sscanf(picture, "GFX_pick_era_image_%d", &era->image) != 1 ||
            era->image < 1 || era->image > 6) {
            SDL_free(data);
            return SDL_SetError("Invalid CK2 starting-era bookmark");
        }
        while (era->count < ERA_RULERS && is(find(&bookmark, "selectable_character"), "{")) {
            struct era_ruler *ruler = &era->rulers[era->count++];
            load_ruler(ruler, bookmark);
            ruler->portrait_year = era->date.year;
            skip_block(&bookmark);
        }
    }
    SDL_free(data);
    return menu->count ? 0 : SDL_SetError("No CK2 starting-era bookmarks found");
}

static int date_order(struct campaign_date date)
{
    return date.year * 10000 + date.month * 100 + date.day;
}

static bool read_date(struct token token, struct campaign_date *date)
{
    char value[24], trailing;
    copy_token(value, sizeof value, token);
    return sscanf(value, "%4d.%2d.%2d%c", &date->year, &date->month, &date->day, &trailing) == 3
        && date->year > 0 && date->month >= 1 && date->month <= 12
        && date->day >= 1 && date->day <= 31;
}

static unsigned title_holder(const char *root, const char *title, struct campaign_date start)
{
    char path[128];
    SDL_snprintf(path, sizeof path, "history/titles/%s.txt", title);
    char *data = load_data(root, path);
    unsigned holder = 0;
    int latest = 0;
    const char *p = data;
    while (p && *p) {
        struct token key = token(&p);
        if (!key.size || !is(token(&p), "="))
            break;
        if (!is(token(&p), "{"))
            continue;
        struct campaign_date date;
        if (read_date(key, &date)) {
            const int when = date_order(date);
            const struct token id = field(p, "holder");
            if (id.size && when >= latest && when <= date_order(start)) {
                latest = when;
                holder = number(id);
            }
        }
        skip_block(&p);
    }
    SDL_free(data);
    return holder;
}

static struct campaign_date character_birth(const char *p)
{
    struct campaign_date date = {0};
    while (*p) {
        struct token key = token(&p);
        if (!key.size || is(key, "}") || !is(token(&p), "="))
            break;
        if (!is(token(&p), "{"))
            continue;
        struct campaign_date candidate;
        if (read_date(key, &candidate) && field(p, "birth").size) {
            date = candidate;
            break;
        }
        skip_block(&p);
    }
    return date;
}

static void load_character_history(struct era_menu *menu, const char *root)
{
    struct ck2_asset_files files = ck2_list_assets(root, "history/characters");
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t length = strlen(name);
        if (length < 4 || strcmp(name + length - 4, ".txt"))
            continue;
        char *data = load_from_directory(root, "history/characters", name);
        const char *p = data;
        while (p && *p) {
            struct token key = token(&p);
            if (!key.size || !is(token(&p), "="))
                break;
            if (!is(token(&p), "{"))
                continue;
            const unsigned id = number(key);
            if (id) {
                for (int i = 0; i < menu->count; ++i) {
                    struct starting_era *era = &menu->eras[i];
                    for (int j = 0; j < era->count; ++j) {
                        struct era_ruler *ruler = &era->rulers[j];
                        if (ruler->id == id)
                            ruler->birth = character_birth(p);
                    }
                }
            }
            skip_block(&p);
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
}

static void translate(char *out, size_t capacity, const char *key, struct token name,
                      struct token value)
{
    if (!*key || !is(name, key))
        return;
    size_t n = 0;
    for (size_t i = 0; i < value.size && n + 1 < capacity; ++i) {
        if (value.start[i] == '\\' && i + 1 < value.size && value.start[i + 1] == 'n') {
            out[n++] = ' ';
            ++i;
        } else if ((unsigned char)value.start[i] == 0x92) {
            out[n++] = '\'';
        } else {
            out[n++] = value.start[i];
        }
    }
    out[n] = '\0';
}

static void load_localization(struct era_menu *menu, const char *root)
{
    struct ck2_asset_files files = ck2_list_assets(root, "localisation");
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t length = strlen(name);
        if (length < 4 || strcmp(name + length - 4, ".csv"))
            continue;
        char *file = load_from_directory(root, "localisation", name);
        for (const char *line = file; line && *line;) {
            const char *end = strchr(line, '\n');
            if (!end)
                end = line + strlen(line);
            const char *separator = memchr(line, ';', (size_t)(end - line));
            if (separator && *line != '#') {
                const char *english = separator + 1;
                const char *last = memchr(english, ';', (size_t)(end - english));
                if (last) {
                    struct token key = {line, (size_t)(separator - line)};
                    struct token value = {english, (size_t)(last - english)};
                    for (int i = 0; i < menu->count; ++i) {
                        struct starting_era *era = &menu->eras[i];
                        translate(era->name, sizeof era->name, era->name_key, key, value);
                        translate(era->description, sizeof era->description, era->description_key,
                                  key, value);
                        for (int j = 0; j < era->count; ++j) {
                            struct era_ruler *ruler = &era->rulers[j];
                            translate(ruler->name, sizeof ruler->name, ruler->name_key, key, value);
                            translate(ruler->title_name, sizeof ruler->title_name,
                                      ruler->title_name_key, key, value);
                            static const char info_prefix[] = "ERA_CHAR_INFO_";
                            if (key.size > sizeof info_prefix - 1 &&
                                !memcmp(key.start, info_prefix, sizeof info_prefix - 1)) {
                                char info_key[64];
                                SDL_snprintf(info_key, sizeof info_key, "%s%u", info_prefix, ruler->id);
                                translate(ruler->biography, sizeof ruler->biography, info_key, key, value);
                            }
                        }
                    }
                }
            }
            line = *end ? end + 1 : end;
        }
        SDL_free(file);
    }
    ck2_free_asset_files(&files);
}

static bool pagan(const struct era_ruler *ruler)
{
    return strstr(ruler->religion, "pagan") != NULL;
}

static SDL_Texture *ruler_shield(SDL_Renderer *renderer, const char *root,
                                 const struct era_ruler *ruler)
{
    char relative[128];
    SDL_snprintf(relative, sizeof relative, "gfx/flags/%s.tga", ruler->title);
    char *path = ck2_asset_path(root, relative);
    SDL_Surface *flag = path ? IMG_Load(path) : NULL;
    free(path);
    if (!flag)
        return NULL;

    path = ck2_asset_path(root, pagan(ruler) ? "gfx/interface/shield_medium_mask_pagan.tga"
                                            : "gfx/interface/shield_medium_mask.tga");
    SDL_Surface *raw_mask = path ? IMG_Load(path) : NULL;
    free(path);
    SDL_Surface *mask = raw_mask ? SDL_ConvertSurfaceFormat(raw_mask, SDL_PIXELFORMAT_BGRA32, 0) : NULL;
    SDL_FreeSurface(raw_mask);
    SDL_Surface *shield = mask ? SDL_CreateRGBSurfaceWithFormat(0, mask->w, mask->h, 32,
                                                               SDL_PIXELFORMAT_BGRA32) : NULL;
    SDL_Texture *texture = NULL;
    if (shield) {
        SDL_SetSurfaceBlendMode(flag, SDL_BLENDMODE_NONE);
        if (SDL_BlitScaled(flag, NULL, shield, NULL) == 0) {
            for (int y = 0; y < shield->h; ++y) {
                Uint8 *out = (Uint8 *)shield->pixels + y * shield->pitch;
                const Uint8 *in = (const Uint8 *)mask->pixels + y * mask->pitch;
                for (int x = 0; x < shield->w; ++x)
                    out[4 * x + 3] = in[4 * x + 3];
            }
            texture = SDL_CreateTextureFromSurface(renderer, shield);
        }
    }
    SDL_FreeSurface(shield);
    SDL_FreeSurface(mask);
    SDL_FreeSurface(flag);
    return texture;
}

int era_menu_load(struct era_menu *menu, SDL_Renderer *renderer, const char *root)
{
    if (load_bookmarks(menu, root) < 0)
        return -1;
    load_localization(menu, root);
    load_character_history(menu, root);
    struct {
        SDL_Texture **target;
        const char *path;
    } art[] = {
        {&menu->background, "gfx/interface/pick_era_bg.dds"},
        {&menu->info, "gfx/interface/pick_era_info_bg.dds"},
        {&menu->banner, "gfx/interface/pick_era_portrait_banner.dds"},
        {&menu->frame, "gfx/interface/charframe_100.dds"},
        {&menu->selected_frame, "gfx/interface/pick_era_selected_char.dds"},
        {&menu->ornament, "gfx/interface/shattered_random/era_shattered_random_button_ornament.dds"},
        {&menu->shield_frame, "gfx/interface/shield_medium_overlay.dds"},
        {&menu->pagan_shield_frame, "gfx/interface/shield_medium_overlay_pagan.dds"},
        {&menu->shattered, "gfx/interface/shattered_random/era_shattered_button.dds"},
        {&menu->random, "gfx/interface/shattered_random/era_random_button.dds"},
        {&menu->small_font, "gfx/fonts/vic_18.tga"},
        {&menu->button, "gfx/interface/big_button_220.dds"},
    };
    for (size_t i = 0; i < SDL_arraysize(art); ++i) {
        *art[i].target = ck2_load_texture(renderer, root, art[i].path);
        if (!*art[i].target)
            return -1;
    }
    menu->button_hover = ck2_load_hover(renderer, root, art[SDL_arraysize(art) - 1].path);
    if (!menu->button_hover ||
        ck2_load_font_metrics(menu->small_glyphs, root, "gfx/fonts/vic_18.fnt") < 0)
        return -1;
    struct portrait_source portraits = {0};
    portraits_open(&portraits, root);
    for (int i = 0; i < menu->count; ++i) {
        struct starting_era *era = &menu->eras[i];
        char path[128];
        SDL_snprintf(path, sizeof path, "gfx/interface/pick_era_image_%d.dds", era->image);
        era->card = ck2_load_texture(renderer, root, path);
        if (!era->card) {
            portraits_close(&portraits);
            return -1;
        }
        for (int j = 0; j < era->count; ++j) {
            struct era_ruler *ruler = &era->rulers[j];
            ruler->portrait = portrait_render(&portraits, renderer, root, ruler);
            if (ruler->title[0] && strspn(ruler->title, "abcdefghijklmnopqrstuvwxyz_0123456789") ==
                strlen(ruler->title)) {
                ruler->title_holder = title_holder(root, ruler->title, era->date);
                ruler->shield = ruler_shield(renderer, root, ruler);
            }
        }
    }
    portraits_close(&portraits);
    menu->selected = menu->ruler = -1;
    return 0;
}

void era_menu_free(struct era_menu *menu)
{
    SDL_Texture *art[] = {menu->background, menu->info, menu->banner, menu->frame,
        menu->selected_frame, menu->ornament, menu->shattered, menu->random,
        menu->shield_frame, menu->pagan_shield_frame,
        menu->button, menu->button_hover, menu->small_font};
    for (size_t i = 0; i < SDL_arraysize(art); ++i) {
        if (art[i])
            SDL_DestroyTexture(art[i]);
    }
    for (int i = 0; i < menu->count; ++i) {
        struct starting_era *era = &menu->eras[i];
        if (era->card)
            SDL_DestroyTexture(era->card);
        for (int j = 0; j < era->count; ++j) {
            if (era->rulers[j].portrait)
                SDL_DestroyTexture(era->rulers[j].portrait);
            if (era->rulers[j].shield)
                SDL_DestroyTexture(era->rulers[j].shield);
        }
    }
}

static SDL_Rect card_area(int i)
{
    return (SDL_Rect){PANEL_X + 73 + 181 * i, PANEL_Y + 121, 172, 216};
}

static SDL_Rect ruler_area(int i)
{
    return (SDL_Rect){PANEL_X + 87 + 143 * i, PANEL_Y + 364, 143, 146};
}

static void text_line(SDL_Renderer *renderer, const struct menu_assets *assets, settings_text text,
                      const char *line, int x, int y)
{
    text(renderer, assets, line, strlen(line), x, y);
}

static void wrapped(SDL_Renderer *renderer, const struct menu_assets *assets, settings_text text,
                    const char *line, int x, int y, int width, int rows, bool centered)
{
    for (int row = 0; *line && row < rows; ++row) {
        while (*line == ' ')
            ++line;
        const char *end = line, *space = NULL;
        int size = 0, at_space = 0;
        while (*end) {
            const int advance = assets->glyphs[(unsigned char)*end].advance;
            if (size + advance > width && end != line)
                break;
            if (*end == ' ') {
                space = end;
                at_space = size;
            }
            size += advance;
            ++end;
        }
        if (*end && space && space != line) {
            end = space;
            size = at_space;
        }
        text(renderer, assets, line, (size_t)(end - line), centered ? x : x + size / 2, y + row * 19);
        line = end;
    }
}

static void draw_card(const struct era_menu *menu, const struct starting_era *era, SDL_Rect area,
                      SDL_Renderer *renderer, const struct menu_assets *assets, settings_text text,
                      SDL_Point pointer)
{
    const SDL_Rect crop = {0, 0, 172, 216};
    const int center = area.x + area.w / 2;
    if (SDL_PointInRect(&pointer, &area))
        SDL_SetTextureColorMod(era->card, 220, 220, 220);
    SDL_RenderCopy(renderer, era->card, &crop, &area);
    SDL_SetTextureColorMod(era->card, 255, 255, 255);
    text_line(renderer, assets, text, era->name, center, area.y + 6);
    char date[32];
    SDL_snprintf(date, sizeof date, "%d A.D.", era->date.year);
    text_line(renderer, assets, text, date, center, area.y + 24);
    SDL_RenderCopy(renderer, menu->ornament, NULL,
                   &(SDL_Rect){center - 60, area.y + 182, 121, 46});
    SDL_RenderCopy(renderer, menu->shattered, NULL,
                   &(SDL_Rect){center - 35, area.y + 190, 34, 34});
    SDL_RenderCopy(renderer, menu->random, NULL,
                   &(SDL_Rect){center + 2, area.y + 190, 34, 34});
}

void era_draw_portrait(const struct era_menu *menu, SDL_Renderer *renderer, SDL_Texture *portrait,
                       char rank, SDL_Rect area)
{
    if (!portrait)
        return;
    const int frame = rank == 'e' ? 5 : rank == 'k' ? 4 : rank == 'd' ? 3 : 2;
    const int margin = area.w * 8 / 116;
    const SDL_Rect face = {area.x + margin, area.y + margin, area.w - 2 * margin,
                           area.h - 2 * margin};
    SDL_RenderCopy(renderer, portrait, NULL, &face);
    SDL_RenderCopy(renderer, menu->frame, &(SDL_Rect){frame * 116, 0, 116, 116}, &area);
}

static void draw_rulers(const struct era_menu *menu, const struct starting_era *era,
                        SDL_Renderer *renderer, const struct menu_assets *assets, settings_text text)
{
    for (int i = 0; i < era->count; ++i) {
        const struct era_ruler *ruler = &era->rulers[i];
        const SDL_Rect area = ruler_area(i);
        if (i == menu->ruler)
            SDL_RenderCopy(renderer, menu->selected_frame, NULL,
                           &(SDL_Rect){area.x + 6, area.y - 7, 131, 128});
        era_draw_portrait(menu, renderer, ruler->portrait, ruler->title[0],
                          (SDL_Rect){area.x + 13, area.y - 1, 116, 116});
        if (ruler->shield) {
            SDL_RenderCopy(renderer, ruler->shield, NULL,
                           &(SDL_Rect){area.x + 10, area.y + 50, 40, 40});
            SDL_Texture *trim = pagan(ruler) ? menu->pagan_shield_frame : menu->shield_frame;
            SDL_RenderCopy(renderer, trim, NULL, &(SDL_Rect){area.x - 2, area.y + 38, 64, 64});
        }
        SDL_RenderCopy(renderer, menu->banner, NULL,
                       &(SDL_Rect){area.x, area.y + 88, 143, 56});
        SDL_SetTextureColorMod(assets->font, 36, 26, 16);
        wrapped(renderer, assets, text, ruler->name, area.x + area.w / 2, area.y + 96, 125, 2, true);
        SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    }
}

void era_menu_draw(const struct era_menu *menu, SDL_Renderer *renderer,
                   const struct menu_assets *assets, settings_text text, SDL_Point pointer)
{
    struct menu_assets small = {.font = menu->small_font};
    memcpy(small.glyphs, menu->small_glyphs, sizeof small.glyphs);
    SDL_RenderCopy(renderer, menu->background, NULL, &(SDL_Rect){PANEL_X, PANEL_Y, 1048, 732});
    text_line(renderer, assets, text, "Choose Your Starting Era", 800, PANEL_Y + 19);
    SDL_SetTextureColorMod(small.font, 36, 26, 16);
    text_line(renderer, &small, text, "Crusader Kings II spans a period of almost 700 years. Choose where in the timeline you want your game to start.", 800, PANEL_Y + 85);
    SDL_SetTextureColorMod(small.font, 255, 255, 255);
    if (menu->selected < 0) {
        for (int i = 0; i < menu->count; ++i)
            draw_card(menu, &menu->eras[i], card_area(i), renderer, assets, text, pointer);
        return;
    }

    const struct starting_era *era = &menu->eras[menu->selected];
    draw_card(menu, era, card_area(0), renderer, assets, text, pointer);
    SDL_RenderCopy(renderer, menu->info, NULL, &(SDL_Rect){PANEL_X + 259, PANEL_Y + 123, 708, 212});
    SDL_SetTextureColorMod(small.font, 36, 26, 16);
    wrapped(renderer, &small, text, era->description, PANEL_X + 291, PANEL_Y + 139, 642, 9, false);
    SDL_SetTextureColorMod(small.font, 255, 255, 255);
    draw_rulers(menu, era, renderer, &small, text);

    SDL_SetTextureColorMod(small.font, 36, 26, 16);
    if (menu->ruler >= 0) {
        const struct era_ruler *ruler = &era->rulers[menu->ruler];
        char summary[224];
        const char *title = ruler->title_name[0] ? ruler->title_name : ruler->title;
        if (ruler->birth.year && ruler->title_holder == ruler->id)
            SDL_snprintf(summary, sizeof summary, "%s, born %d | %s", ruler->name,
                         ruler->birth.year, title);
        else if (ruler->birth.year)
            SDL_snprintf(summary, sizeof summary, "%s, born %d", ruler->name, ruler->birth.year);
        else
            SDL_snprintf(summary, sizeof summary, "%s, age %u", ruler->name, ruler->age);
        text_line(renderer, &small, text, summary, 800, PANEL_Y + 513);
        wrapped(renderer, &small, text, ruler->biography, PANEL_X + 110, PANEL_Y + 534, 825, 4, false);
    } else {
        text_line(renderer, &small, text, "Start Game opens the game rules for this date.",
                  800, PANEL_Y + 550);
    }
    SDL_SetTextureColorMod(small.font, 255, 255, 255);
}

bool era_menu_click(struct era_menu *menu, SDL_Point point)
{
    if (era_menu_deferred(menu, point))
        return true;
    const int cards = menu->selected < 0 ? menu->count : 1;
    for (int i = 0; i < cards; ++i) {
        const SDL_Rect area = card_area(i);
        if (SDL_PointInRect(&point, &area)) {
            menu->selected = menu->selected < 0 ? i : -1;
            menu->ruler = -1;
            return true;
        }
    }
    if (menu->selected >= 0) {
        for (int i = 0; i < menu->eras[menu->selected].count; ++i) {
            const SDL_Rect area = ruler_area(i);
            if (SDL_PointInRect(&point, &area)) {
                menu->ruler = menu->ruler == i ? -1 : i;
                return true;
            }
        }
    }
    return false;
}

bool era_menu_deferred(const struct era_menu *menu, SDL_Point point)
{
    const int cards = menu->selected < 0 ? menu->count : 1;
    for (int i = 0; i < cards; ++i) {
        const SDL_Rect card = card_area(i);
        const SDL_Rect icons = {card.x + 51, card.y + 190, 71, 34};
        if (SDL_PointInRect(&point, &icons))
            return true;
    }
    return false;
}

bool era_menu_can_start(const struct era_menu *menu)
{
    return menu->selected >= 0;
}
