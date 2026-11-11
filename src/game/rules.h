#ifndef OPENCK2_RULES_H
#define OPENCK2_RULES_H

#include "settings.h"

struct rule_option {
    char id[48], label_key[96], label[96], description_key[96], description[800];
};

struct game_rule {
    char id[64], label_key[96], label[96], dlc[96];
    Uint64 groups;
    struct rule_option *options;
    int count, selected;
};

struct rules_menu {
    struct game_rule *rules;
    int count, first_row, filter, group_count;
    struct {
        char key[96], label[96];
    } groups[48];
    SDL_Texture *frame, *banner, *divider, *option, *left, *right, *font;
    struct glyph glyphs[256];
    char status[112];
};

int rules_load(struct rules_menu *, SDL_Renderer *, const char *);
void rules_free(struct rules_menu *);
void rules_draw(const struct rules_menu *, SDL_Renderer *, const struct menu_assets *,
                settings_text, SDL_Point);
void rules_scroll(struct rules_menu *, int);
bool rules_click(struct rules_menu *, SDL_Point);

#endif
