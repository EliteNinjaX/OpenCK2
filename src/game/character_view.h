#ifndef OPENCK2_CHARACTER_VIEW_H
#define OPENCK2_CHARACTER_VIEW_H

#include "setup_toolbar.h"
#include "realms.h"

enum { CHARACTER_FAMILY_ROWS = 4, CHARACTER_FAMILY_LIMIT = 12, CHARACTER_VASSAL_ROWS = 4 };

struct character_relative {
    struct ruler_profile profile;
    SDL_Texture *portrait;
};

struct character_view {
    struct setup_widget background, family, close, tabs[6], frame, spouse_frame, spouse_plate, religion, crown, gold_icon;
    struct setup_widget stat_icons[RULER_STAT_COUNT];
    /* Optional original CK2 character-panel controls (visual placeholders). */
    struct setup_widget extra_icons[16];
    char extra_icon_labels[16][64];
    SDL_Rect stat_values[RULER_STAT_COUNT], trait_area, combat_skill_area;
    int trait_step, trait_bonus[RULER_STAT_COUNT];
    int trait_modifiers[RULER_TRAIT_LIMIT][RULER_STAT_COUNT];
    struct setup_widget section_icons[4], prestige_icon, piety_icon;
    char section_labels[4][96], section_help[4][512];
    char resource_labels[RULER_RESOURCE_COUNT][64], resource_help[RULER_RESOURCE_COUNT][512];
    char spouse_labels[2][32];
    SDL_Rect prestige, piety;
    struct setup_widget trait_icons[RULER_TRAIT_LIMIT];
    char trait_names[RULER_TRAIT_LIMIT][96], rank_names[2][6][32];
    SDL_Rect labels[15], family_rows[CHARACTER_FAMILY_ROWS], portrait, spouse, shield, gold;
    char text[15][96];
    struct ruler_profile ruler;
    struct character_relative family_members[CHARACTER_FAMILY_ROWS][CHARACTER_FAMILY_LIMIT], partner;
    struct setup_widget vassal_bg, vassal_frame;
    SDL_Rect vassal_list, vassal_entry, vassal_name, vassal_portrait, vassal_shield;
    struct {
        char label[220];
        unsigned id;
        SDL_Texture *portrait, *shield;
    } vassals[CHARACTER_VASSAL_ROWS];
    size_t vassal_count, vassal_scroll;
    bool show_vassals;
    struct setup_widget title_crown;
    SDL_Rect titles_area, claims_area, title_entry;
    struct setup_widget strong_claim;
    SDL_Texture *title_shields[12];
    struct realm_title_info titles[12];
    size_t title_count;
    SDL_Texture *claim_shields[RULER_CLAIM_LIMIT];
    char claim_names[RULER_CLAIM_LIMIT][96];
    bool claim_weak[RULER_CLAIM_LIMIT];
    size_t claim_count;
    SDL_Texture *portrait_texture;
    struct realm_title_info display_title;
    bool has_display_title;
    SDL_Texture *arms;
    bool open;
};

typedef SDL_Texture *(*character_portrait_fn)(void *, const struct ruler_profile *);
int character_view_load(struct character_view *, SDL_Renderer *, const char *, SDL_Point);
void character_view_set(struct character_view *, const char *, struct campaign_date,
                        const struct ruler_profile *, SDL_Renderer *, character_portrait_fn, void *);
void character_view_titles(struct character_view *, const struct realm_map *, SDL_Renderer *, const char *);
void character_view_vassals(struct character_view *, const struct realm_map *, SDL_Renderer *,
                            const char *, character_portrait_fn, void *);
typedef void (*panel_text_fn)(SDL_Renderer *, const struct menu_assets *, const char *, size_t, int, int);
void panel_tooltip(SDL_Renderer *, const struct menu_assets *, SDL_Point, const char *, const char *, panel_text_fn);
const struct ruler_profile *character_view_pick(const struct character_view *, SDL_Point, unsigned *);
void character_view_draw(const struct character_view *, SDL_Renderer *, const struct menu_assets *,
                         bool, unsigned, SDL_Point, panel_text_fn);
void character_view_free(struct character_view *);

#endif
