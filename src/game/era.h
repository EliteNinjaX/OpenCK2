#ifndef OPENCK2_ERA_H
#define OPENCK2_ERA_H

#include "campaign.h"
#include "settings.h"

enum { ERA_COUNT = 5, ERA_RULERS = 6 };

struct era_ruler {
    char name_key[64], name[96], title[64], dna[32], properties[48], religion[48], culture[48];
    char biography[1400], title_name_key[64], title_name[96];
    unsigned id, age, title_holder;
    int portrait_year;
    struct campaign_date birth;
    SDL_Texture *portrait, *shield;
};

struct starting_era {
    struct campaign_date date;
    char name_key[64], name[96], description_key[64], description[2304];
    int image, count;
    SDL_Texture *card;
    struct era_ruler rulers[ERA_RULERS];
};

struct era_menu {
    struct starting_era eras[ERA_COUNT];
    SDL_Texture *background, *info, *banner, *frame, *selected_frame, *ornament, *shattered, *random;
    SDL_Texture *shield_frame, *pagan_shield_frame;
    SDL_Texture *button, *button_hover, *small_font;
    struct glyph small_glyphs[256];
    int selected, ruler, count;
};

int era_menu_load(struct era_menu *, SDL_Renderer *, const char *);
void era_menu_free(struct era_menu *);
void era_draw_portrait(const struct era_menu *, SDL_Renderer *, SDL_Texture *, char rank, SDL_Rect);
void era_menu_draw(const struct era_menu *, SDL_Renderer *, const struct menu_assets *,
                   settings_text, SDL_Point);
bool era_menu_click(struct era_menu *, SDL_Point);
bool era_menu_deferred(const struct era_menu *, SDL_Point);
bool era_menu_can_start(const struct era_menu *);

#endif
