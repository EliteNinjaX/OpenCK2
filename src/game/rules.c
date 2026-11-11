#define _POSIX_C_SOURCE 200809L
#include "rules.h"
#include "ck2_script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { X = 277, Y = 52, ROWS = 3 };
static const SDL_Rect filter = {X + 174, Y + 166, 185, 25};
static const SDL_Rect save = {X + 390, Y + 694, 82, 25};
static const SDL_Rect reset = {X + 483, Y + 694, 80, 25};
static const SDL_Rect load = {X + 574, Y + 694, 82, 25};

static void field_copy(char *out, size_t size, const char *block, const char *key)
{
    copy_token(out, size, field(block, key));
}

static int read_rules(struct rules_menu *menu, const char *root)
{
    char *data = load_data(root, "common/game_rules/00_game_rules.txt");
    if (!data)
        return -1;
    const char *p = data;
    struct token id;
    while ((id = token(&p)).size && is(token(&p), "=") && is(token(&p), "{")) {
        const char *block = p;
        skip_block(&p);
        struct game_rule *next = realloc(menu->rules, (size_t)(menu->count + 1) * sizeof *next);
        if (!next)
            goto oom;
        menu->rules = next;
        struct game_rule *rule = &menu->rules[menu->count++];
        *rule = (struct game_rule){0};
        copy_token(rule->id, sizeof rule->id, id);
        field_copy(rule->label_key, sizeof rule->label_key, block, "name");
        field_copy(rule->dlc, sizeof rule->dlc, block, "dlc");
        const char *groups = block;
        for (struct token key; (key = find(&groups, "group")).size;) {
            char group_key[96];
            copy_token(group_key, sizeof group_key, key);
            int group = 0;
            while (group < menu->group_count && strcmp(menu->groups[group].key, group_key))
                ++group;
            if (group == (int)SDL_arraysize(menu->groups))
                continue;
            if (group == menu->group_count) {
                SDL_strlcpy(menu->groups[group].key, group_key, sizeof menu->groups[group].key);
                ++menu->group_count;
            }
            rule->groups |= (Uint64)1 << group;
        }
        while (is(find(&block, "option"), "{")) {
            struct rule_option *options = realloc(rule->options, (size_t)(rule->count + 1) * sizeof *options);
            if (!options)
                goto oom;
            rule->options = options;
            struct rule_option *option = &options[rule->count++];
            *option = (struct rule_option){0};
            field_copy(option->id, sizeof option->id, block, "name");
            field_copy(option->label_key, sizeof option->label_key, block, "text");
            field_copy(option->description_key, sizeof option->description_key, block, "desc");
            skip_block(&block);
        }
    }
    SDL_free(data);
    return menu->count ? 0 : SDL_SetError("No CK2 game rules found");

oom:
    SDL_free(data);
    return SDL_OutOfMemory();
}

static int localize(struct rules_menu *menu, const char *root)
{
    size_t count = (size_t)(menu->group_count + menu->count);
    for (int i = 0; i < menu->count; ++i)
        count += 2 * (size_t)menu->rules[i].count;
    struct ck2_translation *terms = malloc(count * sizeof *terms);
    if (!terms)
        return SDL_OutOfMemory();
    size_t n = 0;
    for (int i = 0; i < menu->group_count; ++i) {
        terms[n++] = (struct ck2_translation){menu->groups[i].key, menu->groups[i].label,
                                              sizeof menu->groups[i].label};
    }
    for (int i = 0; i < menu->count; ++i) {
        struct game_rule *rule = &menu->rules[i];
        terms[n++] = (struct ck2_translation){rule->label_key, rule->label, sizeof rule->label};
        for (int j = 0; j < rule->count; ++j) {
            struct rule_option *option = &rule->options[j];
            terms[n++] = (struct ck2_translation){option->label_key, option->label, sizeof option->label};
            terms[n++] = (struct ck2_translation){option->description_key, option->description,
                                                  sizeof option->description};
        }
    }
    ck2_translate(root, terms, n);
    free(terms);
    return 0;
}

int rules_load(struct rules_menu *menu, SDL_Renderer *renderer, const char *root)
{
    if (read_rules(menu, root) < 0 || localize(menu, root) < 0)
        return -1;
    struct {
        SDL_Texture **texture;
        const char *path;
    } art[] = {
        {&menu->frame, "gfx/interface/rule_bg.dds"},
        {&menu->banner, "gfx/interface/rule_wide_banner.dds"},
        {&menu->divider, "gfx/interface/rule_divider.dds"},
        {&menu->option, "gfx/interface/rule_option_bg.dds"},
        {&menu->left, "gfx/interface/slider_left.dds"},
        {&menu->right, "gfx/interface/slider_right.dds"},
        {&menu->font, "gfx/fonts/vic_18.tga"},
    };
    for (size_t i = 0; i < SDL_arraysize(art); ++i) {
        *art[i].texture = ck2_load_texture(renderer, root, art[i].path);
        if (!*art[i].texture)
            return -1;
    }
    return ck2_load_font_metrics(menu->glyphs, root, "gfx/fonts/vic_18.fnt");
}

void rules_free(struct rules_menu *menu)
{
    SDL_Texture *art[] = {menu->frame, menu->banner, menu->divider, menu->option,
                          menu->left, menu->right, menu->font};
    for (size_t i = 0; i < SDL_arraysize(art); ++i)
        if (art[i])
            SDL_DestroyTexture(art[i]);
    for (int i = 0; i < menu->count; ++i)
        free(menu->rules[i].options);
    free(menu->rules);
    *menu = (struct rules_menu){0};
}

static bool visible(const struct rules_menu *menu, const struct game_rule *rule)
{
    return rule->count && (!menu->filter ||
           (rule->groups & ((Uint64)1 << (menu->filter - 1))));
}

static int visible_count(const struct rules_menu *menu)
{
    int count = 0;
    for (int i = 0; i < menu->count; ++i)
        count += visible(menu, &menu->rules[i]);
    return count;
}

static int max_scroll(const struct rules_menu *menu)
{
    return SDL_max(0, (visible_count(menu) + 1) / 2 - ROWS);
}

void rules_scroll(struct rules_menu *menu, int rows)
{
    menu->first_row = SDL_clamp(menu->first_row + rows, 0, max_scroll(menu));
}

static SDL_Rect tile(int slot)
{
    return (SDL_Rect){X + 72 + 451 * (slot % 2), Y + 216 + 125 * (slot / 2), 440, 124};
}

static void text_line(SDL_Renderer *r, const struct menu_assets *a, settings_text text,
                      const char *s, int x, int y)
{
    text(r, a, s, strlen(s), x, y);
}

static void text_wrapped(SDL_Renderer *r, const struct menu_assets *a, settings_text text,
                         const char *s, int x, int y, int width)
{
    for (int row = 0; *s && row < 4; ++row) {
        while (*s == ' ')
            ++s;
        const char *end = s, *space = NULL;
        int used = 0;
        while (*end) {
            const int advance = a->glyphs[(unsigned char)*end].advance;
            if (used + advance > width && end > s)
                break;
            if (*end == ' ')
                space = end;
            used += advance;
            ++end;
        }
        if (*end && space && space > s)
            end = space;
        text(r, a, s, (size_t)(end - s), x, y + row * 16);
        s = end;
    }
}

static void small_button(const struct rules_menu *m, SDL_Renderer *r, const struct menu_assets *a,
                         settings_text text, SDL_Rect area, const char *label, SDL_Point pointer)
{
    const SDL_Rect source = {SDL_PointInRect(&pointer, &area) ? 201 : 0, 0, 201, 22};
    SDL_RenderCopy(r, m->option, &source, &area);
    text_line(r, a, text, label, area.x + area.w / 2, area.y + 2);
}

void rules_draw(const struct rules_menu *menu, SDL_Renderer *r, const struct menu_assets *assets,
                settings_text text, SDL_Point pointer)
{
    struct menu_assets small = {.font = menu->font};
    memcpy(small.glyphs, menu->glyphs, sizeof small.glyphs);
    SDL_RenderCopy(r, menu->frame, NULL, &(SDL_Rect){X, Y, 1046, 796});
    text_line(r, assets, text, "Game Rules", 800, Y + 78);
    const char *filter_label = menu->filter ? menu->groups[menu->filter - 1].label : "All Rules";
    small_button(menu, r, &small, text, filter, filter_label, pointer);
    SDL_RenderCopy(r, menu->banner, NULL, &(SDL_Rect){X + 390, Y + 151, 266, 45});
    SDL_RenderCopy(r, menu->banner, NULL, &(SDL_Rect){X + 390, Y + 640, 266, 45});
    SDL_SetTextureColorMod(small.font, 47, 26, 15);
    text_line(r, &small, text, "Ironman Mode: Disabled", 800, Y + 165);
    text_line(r, &small, text, "Achievements: Disabled", 800, Y + 653);
    SDL_SetTextureColorMod(small.font, 255, 255, 255);

    int slot = -2 * menu->first_row;
    for (int i = 0; i < menu->count && slot < 2 * ROWS; ++i) {
        const struct game_rule *rule = &menu->rules[i];
        if (!visible(menu, rule) || slot++ < 0)
            continue;
        const SDL_Rect area = tile(slot - 1);
        const int center = area.x + area.w / 2;
        const struct rule_option *option = &rule->options[rule->selected];
        SDL_RenderCopy(r, menu->divider, NULL, &(SDL_Rect){area.x, area.y - 3, area.w, 12});
        SDL_SetTextureColorMod(small.font, 47, 26, 15);
        char heading[200];
        const char *label = *rule->label ? rule->label : rule->id;
        if (*rule->dlc && !strstr(label, rule->dlc))
            SDL_snprintf(heading, sizeof heading, "%s: %s", rule->dlc, label);
        else
            SDL_strlcpy(heading, label, sizeof heading);
        text_line(r, &small, text, heading, center, area.y + 12);
        text_wrapped(r, &small, text, option->description, center, area.y + 73, 418);
        SDL_SetTextureColorMod(small.font, 255, 255, 255);
        small_button(menu, r, &small, text, (SDL_Rect){center - 100, area.y + 37, 200, 22},
                     *option->label ? option->label : option->id, pointer);
        SDL_RenderCopy(r, menu->left, NULL, &(SDL_Rect){center - 125, area.y + 37, 22, 22});
        SDL_RenderCopy(r, menu->right, NULL, &(SDL_Rect){center + 103, area.y + 37, 22, 22});
    }
    const SDL_Rect track = {X + 971, Y + 218, 12, 375};
    SDL_SetRenderDrawColor(r, 66, 53, 36, 255);
    SDL_RenderFillRect(r, &track);
    const int limit = max_scroll(menu);
    const int thumb = limit ? SDL_max(24, track.h * ROWS / (limit + ROWS)) : track.h;
    SDL_SetRenderDrawColor(r, 164, 135, 83, 255);
    SDL_RenderFillRect(r, &(SDL_Rect){track.x + 2, track.y +
                       (limit ? (track.h - thumb) * menu->first_row / limit : 0), 8, thumb});
    small_button(menu, r, &small, text, save, "Save Rules", pointer);
    small_button(menu, r, &small, text, reset, "Reset", pointer);
    small_button(menu, r, &small, text, load, "Load Rules", pointer);
    SDL_SetTextureColorMod(small.font, 47, 26, 15);
    text_line(r, &small, text, menu->status, 800, Y + 724);
    SDL_SetTextureColorMod(small.font, 255, 255, 255);
}

static void preset(struct rules_menu *menu, bool writing)
{
    char *pref = SDL_GetPrefPath("OpenCK2", "Game");
    char *path = pref ? SDL_malloc(strlen(pref) + 32) : NULL;
    char *temporary = path && writing ? SDL_malloc(strlen(pref) + 48) : NULL;
    if (path)
        sprintf(path, "%sgame-rules.txt", pref);
    SDL_free(pref);
    FILE *file = NULL;
    if (temporary) {
        sprintf(temporary, "%s.XXXXXX", path);
        const int fd = mkstemp(temporary);
        if (fd >= 0) {
            file = fdopen(fd, "w");
            if (!file) {
                close(fd);
                unlink(temporary);
            }
        }
    } else if (path && !writing) {
        file = fopen(path, "r");
    }
    if (!file) {
        SDL_free(path);
        SDL_free(temporary);
        SDL_strlcpy(menu->status, writing ? "Could not save rule preset." : "No rule preset found.",
                    sizeof menu->status);
        return;
    }
    if (writing) {
        for (int i = 0; i < menu->count; ++i) {
            const struct game_rule *rule = &menu->rules[i];
            if (rule->count)
                fprintf(file, "%s %s\n", rule->id, rule->options[rule->selected].id);
        }
    } else {
        char key[64], value[48];
        while (fscanf(file, "%63s %47s", key, value) == 2) {
            for (int i = 0; i < menu->count; ++i) {
                struct game_rule *rule = &menu->rules[i];
                if (!strcmp(rule->id, key))
                    for (int j = 0; j < rule->count; ++j)
                        if (!strcmp(rule->options[j].id, value))
                            rule->selected = j;
            }
        }
    }
    bool failed = ferror(file);
    if (fclose(file) != 0)
        failed = true;
    if (writing && (failed || rename(temporary, path) != 0)) {
        unlink(temporary);
        failed = true;
    }
    SDL_free(temporary);
    SDL_free(path);
    SDL_strlcpy(menu->status, failed ? "Could not finish reading or writing rule preset."
                : writing ? "Rule preset saved." : "Rule preset loaded.", sizeof menu->status);
}

bool rules_click(struct rules_menu *menu, SDL_Point pointer)
{
    if (SDL_PointInRect(&pointer, &filter)) {
        menu->filter = (menu->filter + 1) % (menu->group_count + 1);
        menu->first_row = 0;
        return true;
    }
    if (SDL_PointInRect(&pointer, &save) || SDL_PointInRect(&pointer, &load)) {
        preset(menu, SDL_PointInRect(&pointer, &save));
        return true;
    }
    if (SDL_PointInRect(&pointer, &reset)) {
        for (int i = 0; i < menu->count; ++i)
            menu->rules[i].selected = 0;
        SDL_strlcpy(menu->status, "Default options restored.", sizeof menu->status);
        return true;
    }
    if (SDL_PointInRect(&pointer, &(SDL_Rect){X + 961, Y + 218, 28, 375})) {
        menu->first_row = max_scroll(menu) * (pointer.y - (Y + 218)) / 374;
        return true;
    }
    int slot = -2 * menu->first_row;
    for (int i = 0; i < menu->count && slot < 2 * ROWS; ++i) {
        struct game_rule *rule = &menu->rules[i];
        if (!visible(menu, rule) || slot++ < 0)
            continue;
        const SDL_Rect area = tile(slot - 1);
        const int center = area.x + area.w / 2;
        if (SDL_PointInRect(&pointer, &(SDL_Rect){center - 125, area.y + 37, 250, 22})) {
            const int delta = pointer.x < center - 100 ? -1 : 1;
            rule->selected = (rule->selected + delta + rule->count) % rule->count;
            return true;
        }
    }
    return false;
}
