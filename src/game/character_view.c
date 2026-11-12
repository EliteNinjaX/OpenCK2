#include "character_view.h"
#include "era.h"
#include "../open_launcher/asset_paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NAME, AGE, DYNASTY, CULTURE, SPOUSE, TAB, PARENTS = 11, GRANDPARENTS, CHILDREN, SIBLINGS };

static bool bind_sprite(struct setup_widget *widget, const struct frontend_gui *gui, int parent,
                        const char *name, SDL_Point viewport, SDL_Renderer *renderer, const char *root)
{
    const int node = frontend_gui_find(gui, parent, name);
    SDL_Rect area;
    return frontend_gui_rect(gui, node, viewport, &area) &&
        setup_widget_load_node(widget, renderer, root, gui, node, area) == 0;
}

int character_view_load(struct character_view *view, SDL_Renderer *renderer, const char *root, SDL_Point viewport)
{
    struct frontend_gui gui = {0};
    if (frontend_gui_load_file(&gui, root, "interface/domestic_dynastic.gui") < 0)
        return -1;
    const int panel = frontend_gui_find(&gui, -1, "dynastic_view");
    const int info = frontend_gui_find(&gui, panel, "character_info");
    const int entry = frontend_gui_find(&gui, -1, "vassal_entry");
    const int title_entry = frontend_gui_find(&gui, -1, "character_title_entry");
    const int family = frontend_gui_find(&gui, panel, "char_family_window");
    const struct { struct setup_widget *widget; int parent; const char *name; } sprites[] = {
        {&view->background, panel, "bg_dynastic"}, {&view->close, panel, "button_close"},
        {&view->family, family, "charview_family_bg_2"}, {&view->frame, info, "character_portrait_frame"},
        {&view->spouse_frame, info, "spouse_portrait_frame"}, {&view->spouse_plate, info, "spouse_label_bg"},
        {&view->religion, info, "religion_icon"}, {&view->crown, info, "character_crown"},
        {&view->gold_icon, info, "icon_wealth"}, {&view->prestige_icon, info, "icon_prestige"},
        {&view->piety_icon, info, "icon_piety"},
        {&view->strong_claim, title_entry, "claim_strong_western"},
        {&view->title_crown, title_entry, "title_icon_crown"},
        {&view->vassal_bg, entry, "charview_list_bg"},
        {&view->vassal_frame, entry, "portrait_frame"},
        {&view->tabs[0], panel, "tab_1"}, {&view->tabs[1], panel, "tab_6"},
        {&view->tabs[2], panel, "tab_2"}, {&view->tabs[3], panel, "tab_3"},
        {&view->tabs[4], panel, "tab_4"}, {&view->tabs[5], panel, "tab_5"},
    };
    bool ok = panel >= 0 && info >= 0 && family >= 0;
    for (size_t i = 0; ok && i < SDL_arraysize(sprites); ++i)
        ok = bind_sprite(sprites[i].widget, &gui, sprites[i].parent, sprites[i].name, viewport, renderer, root);
    /* These controls exist in the original GUI but were never bound by OpenCK2.
       They remain visible even where their click actions are not implemented. */
    static const struct { const char *name; const char *label; int parent; } extras[] = {
        {"button_familytree", "FAMILYTREE_INTERFACE_LABEL", 0},
        {"button_realmtree", "REALM_TREE_INTERFACE_LABEL", 0},
        {"button_treasury", "Treasury (not implemented)", 0},
        {"button_bloodlines", "Bloodlines (not implemented)", 0},
        {"kill_list", "Kill list (not implemented)", 0},
        {"barber_button", "Barber (not implemented)", 0},
        {"government_icon", "Government", 1},
        {"quick_ambition_button", "Ambition (not implemented)", 1},
        {"quick_focus_button", "Focus (not implemented)", 1},
        {"quick_plot_button", "Plot (not implemented)", 0},
        {"toggle_forbidden_to_lead_armies", "Leading armies (toggle not implemented)", 0},
        {"open_wonder_view", "Great Works (view not implemented)", 0},
        {"icon_combat_skill", "Personal combat skill (data not loaded)", 1},
        {"icon_artifact_modifier", "Artifact modifiers (not implemented)", 1},
        {"icon_commandmodifier", "Commander modifiers (not implemented)", 1},
        {"icon_diplomatic_relations", "Diplomatic relations (not implemented)", 1},
    };
    for (size_t i = 0; i < SDL_arraysize(extras); ++i) {
        const int parent = extras[i].parent ? info : panel;
        const int node = frontend_gui_find(&gui, parent, extras[i].name);
        if (node >= 0) {
            SDL_Rect area;
            if (frontend_gui_rect(&gui, node, viewport, &area) &&
                setup_widget_load_node(&view->extra_icons[i], renderer, root, &gui, node, area) == 0)
                SDL_strlcpy(view->extra_icon_labels[i],
                            i < 2 ? ck2_localize(extras[i].label, i ? "Realm Tree" : "Family Tree") : extras[i].label,
                            sizeof view->extra_icon_labels[i]);
            else
                SDL_ClearError(); /* Optional assets must not block the character screen. */
        }
    }
    const char *const labels[] = {
        "name", "born", "dynasty", "culture", "spouse_text",
        "family_text", "relations_text", "vassals_text", "courtiers_text", "pacts_text", "abroad_text",
        "parents_text", "grandparents_text", "children_text", "siblings_text2",
    };
    struct ck2_translation terms[32];
    char keys[32][80];
    size_t count = 0;
    for (size_t i = 0; ok && i < SDL_arraysize(labels); ++i) {
        const int parent = i < TAB ? info : i < PARENTS ? panel : family;
        const int node = frontend_gui_find(&gui, parent, labels[i]);
        ok = frontend_gui_rect(&gui, node, viewport, &view->labels[i]);
        if (ok && (i == SPOUSE || i >= TAB)) {
            copy_token(keys[count], sizeof keys[count], field(gui.blocks[node].body, "text"));
            terms[count] = (struct ck2_translation){keys[count], view->text[i], sizeof view->text[i]};
            ++count;
        }
    }
    /* Combat rating is independent of the five main stats. */
    frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "combat_skill"),
                      viewport, &view->combat_skill_area);
    static const char *const sections[] = {"icon_traits", "icon_modifiers", "icon_titles", "icon_claims"};
    static const char *const resource_keys[RULER_RESOURCE_COUNT][2] = {
        {"STAT_WEALTH", "STAT_WEALTH_DESC"}, {"STAT_PRESTIGE", "STAT_PRESTIGE_DESC"},
        {"STAT_PIETY", "STAT_PIETY_DESC"}
    };
    static const char *const tooltip_fields[] = {"pdx_tooltip", "pdx_tooltip_delayed"};
    for (size_t i = 0; ok && i < SDL_arraysize(sections); ++i) {
        const int node = frontend_gui_find(&gui, info, sections[i]);
        ok = bind_sprite(&view->section_icons[i], &gui, info, sections[i], viewport, renderer, root);
        if (!ok)
            break;
        for (size_t j = 0; j < SDL_arraysize(tooltip_fields); ++j) {
            copy_token(keys[count], sizeof keys[count], field(gui.blocks[node].body, tooltip_fields[j]));
            terms[count] = (struct ck2_translation){keys[count], j ? view->section_help[i] : view->section_labels[i],
                j ? sizeof view->section_help[i] : sizeof view->section_labels[i]};
            ++count;
        }
    }
    for (size_t i = 0; i < RULER_RESOURCE_COUNT; ++i) {
        terms[count++] = (struct ck2_translation){resource_keys[i][0], view->resource_labels[i], sizeof view->resource_labels[i]};
        terms[count++] = (struct ck2_translation){resource_keys[i][1], view->resource_help[i], sizeof view->resource_help[i]};
    }
    terms[count++] = (struct ck2_translation){"FAM_HUSBAND", view->spouse_labels[0], sizeof view->spouse_labels[0]};
    terms[count++] = (struct ck2_translation){"FAM_WIFE", view->spouse_labels[1], sizeof view->spouse_labels[1]};
    const char *const rows[] = {"parents", "grandparents", "children", "siblings2"};
    for (size_t i = 0; ok && i < SDL_arraysize(rows); ++i)
        ok = frontend_gui_rect(&gui, frontend_gui_find(&gui, family, rows[i]), viewport, &view->family_rows[i]);
    ok = ok && frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "claims"), viewport, &view->claims_area) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "prestige"), viewport, &view->prestige) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "piety"), viewport, &view->piety) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "wealth"), viewport, &view->gold) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "character_portrait"), viewport, &view->portrait) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "spouse_portrait"), viewport, &view->spouse) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "character_shield"), viewport, &view->shield);
    /* Original GUI declares dynastic_shield distinctly from character_shield. */
    frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "dynastic_shield"),
                      viewport, &view->dynasty_shield_area);
    ok = ok && frontend_gui_rect(&gui, frontend_gui_find(&gui, panel, "vassal_listbox"), viewport, &view->vassal_list) &&
        frontend_gui_rect(&gui, entry, viewport, &view->vassal_entry) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, entry, "name"), viewport, &view->vassal_name) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, entry, "portrait"), viewport, &view->vassal_portrait) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, entry, "shield"), viewport, &view->vassal_shield);
    for (size_t i = 0; ok && i < RULER_STAT_COUNT; ++i) {
        char icon[32];
        snprintf(icon, sizeof icon, "icon_%s", ruler_stat_keys[i]);
        ok = bind_sprite(&view->stat_icons[i], &gui, info, icon, viewport, renderer, root) &&
            frontend_gui_rect(&gui, frontend_gui_find(&gui, info, ruler_stat_keys[i]),
                              viewport, &view->stat_values[i]);
    }
    ok = ok && frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "titles"), viewport, &view->titles_area) &&
        frontend_gui_rect(&gui, title_entry, viewport, &view->title_entry) && view->title_entry.w > 0;
    SDL_Rect trait_entry;
    ok = ok && frontend_gui_rect(&gui, frontend_gui_find(&gui, info, "traits"), viewport, &view->trait_area) &&
        frontend_gui_rect(&gui, frontend_gui_find(&gui, -1, "character_traits_entry"), viewport, &trait_entry) &&
        trait_entry.w > 0;
    if (ok)
        view->trait_step = trait_entry.w;
    frontend_gui_free(&gui);
    if (!ok) {
        character_view_free(view);
        return SDL_SetError("Could not bind interface/domestic_dynastic.gui");
    }
    ck2_translate(root, terms, count);
    static const char *const titles[2][6] = {
        {"", "baron", "count", "duke", "king", "emperor"},
        {"", "baron_female", "count_female", "duke_female", "king_female", "emperor_female"}
    };
    struct ck2_translation ranks[10];
    size_t rank_count = 0;
    for (size_t sex = 0; sex < 2; ++sex)
        for (size_t rank = 1; rank <= 5; ++rank)
            ranks[rank_count++] = (struct ck2_translation){titles[sex][rank],
                view->rank_names[sex][rank], sizeof view->rank_names[sex][rank]};
    ck2_translate(root, ranks, rank_count);
    /* Original CK2 data-driven dynasty CoA templates; missing files leave the
     * previous title shield fallback available without breaking character UI. */
    view->coa_runtime = dynasty_arms_open(root);
    view->background.area.y = SDL_max(0, view->background.area.y);
    for (size_t i = 0; i < SDL_arraysize(view->tabs); ++i) {
        view->tabs[i].area.w = view->tabs[i].area.w * 83 / 100;
        view->tabs[i].area.h = view->tabs[i].area.h * 83 / 100;
    }
    return 0;
}

static void relative_clear(struct character_relative *relative)
{
    SDL_DestroyTexture(relative->portrait);
    *relative = (struct character_relative){0};
}

static void clear_family(struct character_view *view)
{
    relative_clear(&view->partner);
    for (size_t row = 0; row < CHARACTER_FAMILY_ROWS; ++row)
        for (size_t i = 0; i < CHARACTER_FAMILY_LIMIT; ++i)
            relative_clear(&view->family_members[row][i]);
}

static void load_profiles(const char *root, struct campaign_date date, const unsigned *ids,
                          struct character_relative *const *slots, size_t count,
                          character_portrait_fn render, void *context)
{
    struct ruler_profile profiles[CHARACTER_FAMILY_ROWS * CHARACTER_FAMILY_LIMIT + 1];
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!ids[i])
            continue;
        size_t j = 0;
        while (j < used && profiles[j].id != ids[i])
            ++j;
        if (j == used)
            profiles[used++] = (struct ruler_profile){.id = ids[i]};
    }
    qsort(profiles, used, sizeof *profiles, ruler_compare);
    rulers_load(profiles, used, root, date);
    for (size_t i = 0; i < count; ++i) {
        struct ruler_profile key = {.id = ids[i]};
        const struct ruler_profile *profile = bsearch(&key, profiles, used, sizeof *profiles, ruler_compare);
        if (profile && profile->name[0] && profile->age_known) {
            slots[i]->profile = *profile;
            slots[i]->portrait = render(context, profile);
        }
    }
}

static void load_character_traits(struct character_view *view, SDL_Renderer *renderer, const char *root)
{
    struct ck2_translation labels[RULER_TRAIT_LIMIT];
    for (size_t i = 0; i < RULER_TRAIT_LIMIT; ++i) {
        SDL_DestroyTexture(view->trait_icons[i].texture);
        view->trait_icons[i] = (struct setup_widget){0};
    }
    memset(view->trait_bonus, 0, sizeof view->trait_bonus);
    for (size_t i = 0; i < view->ruler.trait_count; ++i) {
        const char *key = view->ruler.traits[i];
        char sprite[80];
        snprintf(sprite, sizeof sprite, "GFX_trait_%s", key);
        setup_widget_load_sprite(&view->trait_icons[i], renderer, root, "interface/traits.gfx", sprite, (SDL_Rect){0});
        SDL_strlcpy(view->trait_names[i], key, sizeof view->trait_names[i]);
        labels[i] = (struct ck2_translation){key, view->trait_names[i], sizeof view->trait_names[i]};
    }
    ck2_translate(root, labels, view->ruler.trait_count);
    struct ck2_asset_files files = ck2_list_assets(root, "common/traits");
    bool education[RULER_TRAIT_LIMIT] = {0};
    memset(view->trait_modifiers, 0, sizeof view->trait_modifiers);
    for (size_t f = 0; f < files.count; ++f) {
        const char *name = files.names[f];
        const size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 4, ".txt"))
            continue;
        char *data = load_from_directory(root, "common/traits", name);
        for (size_t i = 0; data && i < view->ruler.trait_count; ++i) {
            const char *body = data;
            if (!is(find(&body, view->ruler.traits[i]), "{"))
                continue;
            education[i] = is(field(body, "education"), "yes");
            for (size_t stat = 0; stat < RULER_STAT_COUNT; ++stat) {
                char raw[24], *end;
                copy_token(raw, sizeof raw, field(body, ruler_stat_keys[stat]));
                const long delta = strtol(raw, &end, 10);
                view->trait_modifiers[i][stat] = !*end && delta >= -1000 && delta <= 1000 ? (int)delta : 0;
            }
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
    for (size_t i = 0; i < view->ruler.trait_count; ++i)
        for (size_t stat = 0; stat < RULER_STAT_COUNT; ++stat)
            view->trait_bonus[stat] += view->trait_modifiers[i][stat];
    size_t position = 0;
    for (size_t i = 0; i < view->ruler.trait_count; ++i) {
        if (!education[i])
            continue;
        const struct setup_widget icon = view->trait_icons[i];
        char name[96];
        int modifiers[RULER_STAT_COUNT];
        memcpy(name, view->trait_names[i], sizeof name);
        memcpy(modifiers, view->trait_modifiers[i], sizeof modifiers);
        memmove(&view->trait_icons[position + 1], &view->trait_icons[position],
                (i - position) * sizeof icon);
        memmove(&view->trait_names[position + 1], &view->trait_names[position],
                (i - position) * sizeof name);
        memmove(&view->trait_modifiers[position + 1], &view->trait_modifiers[position],
                (i - position) * sizeof modifiers);
        view->trait_icons[position] = icon;
        memcpy(view->trait_names[position], name, sizeof name);
        memcpy(view->trait_modifiers[position], modifiers, sizeof modifiers);
        ++position;
    }
}

void character_view_set(struct character_view *view, const char *root, struct campaign_date date,
                        const struct ruler_profile *profile, SDL_Renderer *renderer, character_portrait_fn render, void *context)
{
    clear_family(view);
    view->ruler = *profile;
    view->show_vassals = false;
    view->vassal_scroll = 0;
    SDL_DestroyTexture(view->portrait_texture);
    view->portrait_texture = render(context, profile);
    load_character_traits(view, renderer, root);
    unsigned ids[3 + 2 * CHARACTER_FAMILY_LIMIT] = {profile->father, profile->mother, profile->spouse};
    struct character_relative *slots[SDL_arraysize(ids)] = {
        &view->family_members[0][0], &view->family_members[0][1], &view->partner
    };
    rulers_family_ids(root, profile, date, ids + 3, CHARACTER_FAMILY_LIMIT,
                      ids + 3 + CHARACTER_FAMILY_LIMIT, CHARACTER_FAMILY_LIMIT);
    for (size_t i = 0; i < CHARACTER_FAMILY_LIMIT; ++i) {
        slots[3 + i] = &view->family_members[2][i];
        slots[3 + CHARACTER_FAMILY_LIMIT + i] = &view->family_members[3][i];
    }
    load_profiles(root, date, ids, slots, SDL_arraysize(ids), render, context);
    const struct ruler_profile *father = &view->family_members[0][0].profile;
    const struct ruler_profile *mother = &view->family_members[0][1].profile;
    const unsigned grandparents[] = {father->father, father->mother, mother->father, mother->mother};
    for (size_t i = 0; i < SDL_arraysize(grandparents); ++i)
        slots[i] = &view->family_members[1][i];
    load_profiles(root, date, grandparents, slots, SDL_arraysize(grandparents), render, context);
}

void character_view_titles(struct character_view *view, const struct realm_map *realms,
                            SDL_Renderer *renderer, const char *root)
{
    view->has_display_title = realm_primary_title(realms, view->ruler.id, &view->display_title);
    if (!view->has_display_title && view->partner.profile.id && !view->partner.profile.deceased &&
        realm_primary_title(realms, view->partner.profile.id, &view->display_title))
        view->has_display_title = view->display_title.id[0] == 'k' || view->display_title.id[0] == 'e';

    SDL_DestroyTexture(view->arms);
    SDL_DestroyTexture(view->dynasty_arms);
    view->arms = view->has_display_title
        ? era_title_shield_big(renderer, root, view->display_title.id, view->ruler.religion_key) : NULL;
    /* Authored dynasty arms are resolved from common/dynasties and the original
     * interface/coat_of_arms catalog. Unauthored arms use a clearly provisional
     * deterministic design; missing textures retain the existing title proxy. */
    view->dynasty_arms_approximate = false;
    view->dynasty_arms_proxy = false;
    view->dynasty_arms = dynasty_arms_render(view->coa_runtime, renderer,
                                             view->ruler.dynasty_id,
                                             view->ruler.religion_key,
                                             &view->dynasty_arms_approximate);
    if (!view->dynasty_arms && view->ruler.dynasty_id && view->has_display_title) {
        view->dynasty_arms_proxy = true;
        view->dynasty_arms = era_title_shield(renderer, root, view->display_title.id,
                                              view->ruler.religion_key);
    }
    const size_t visible = (size_t)(view->titles_area.w / view->title_entry.w);
    view->title_count = realm_held_titles(realms, view->ruler.id, view->titles,
                                         SDL_min(visible, SDL_arraysize(view->titles)));
    for (size_t i = 0; i < SDL_arraysize(view->title_shields); ++i) {
        SDL_DestroyTexture(view->title_shields[i]);
        view->title_shields[i] = i < view->title_count
            ? era_title_shield(renderer, root, view->titles[i].id, view->ruler.religion_key) : NULL;
    }
    view->claim_count = 0;
    for (size_t i = 0; i < SDL_arraysize(view->claim_shields); ++i) {
        SDL_DestroyTexture(view->claim_shields[i]);
        view->claim_shields[i] = NULL;
    }
    for (size_t i = 0; i < view->ruler.claim_count; ++i) {
        const struct ruler_claim *claim = &view->ruler.claims[i];
        struct realm_title_info title;
        if (!realm_title_info(realms, claim->title, &title))
            continue;
        const size_t index = view->claim_count++;
        view->claim_weak[index] = claim->weak;
        view->claim_shields[index] = era_title_shield(renderer, root, title.id, view->ruler.religion_key);
        snprintf(view->claim_names[index], sizeof view->claim_names[index], "%s", title.name);
    }
}

void character_view_vassals(struct character_view *view, const struct realm_map *realms,
                            SDL_Renderer *renderer, const char *root, character_portrait_fn render, void *context)
{
    struct realm_title_info titles[CHARACTER_VASSAL_ROWS] = {0};
    view->vassal_count = realm_direct_vassals(realms, view->ruler.id, 0, NULL, 0);
    const size_t maximum = view->vassal_count > CHARACTER_VASSAL_ROWS
        ? view->vassal_count - CHARACTER_VASSAL_ROWS : 0;
    if (view->vassal_scroll > maximum)
        view->vassal_scroll = maximum;
    realm_direct_vassals(realms, view->ruler.id, view->vassal_scroll,
                         titles, SDL_arraysize(titles));
    for (size_t i = 0; i < SDL_arraysize(view->vassals); ++i) {
        SDL_DestroyTexture(view->vassals[i].portrait);
        SDL_DestroyTexture(view->vassals[i].shield);
        view->vassals[i].portrait = view->vassals[i].shield = NULL;
        view->vassals[i].label[0] = 0;
        view->vassals[i].id = 0;
        if (view->vassal_scroll + i >= view->vassal_count)
            continue;
        const struct ruler_profile *profile = realm_ruler(realms, titles[i].holder);
        view->vassals[i].id = titles[i].holder;
        view->vassals[i].portrait = render(context, profile);
        view->vassals[i].shield = era_title_shield(renderer, root, titles[i].id,
                                                  profile ? profile->religion_key : "");
        snprintf(view->vassals[i].label, sizeof view->vassals[i].label, "%s%s%s",
                 profile ? profile->name : "", profile ? " of " : "", titles[i].name);
    }
}

static int title_rank_frame(char rank)
{
    return rank == 'e' ? 5 : rank == 'k' ? 4 : rank == 'd' ? 3 : rank == 'c' ? 2 : rank == 'b' ? 1 : 0;
}

static void draw_frame(const struct setup_widget *widget, SDL_Renderer *renderer, int frame)
{
    if (!widget->texture)
        return;
    int width, height;
    SDL_QueryTexture(widget->texture, NULL, NULL, &width, &height);
    width /= widget->frames;
    const SDL_Rect source = {width * SDL_clamp(frame, 0, widget->frames - 1), 0, width, height};
    SDL_RenderCopy(renderer, widget->texture, &source, &widget->area);
}

static SDL_Rect relative_area(const struct character_view *view, size_t row, size_t index)
{
    const SDL_Rect bounds = view->family_rows[row];
    const int diameter = SDL_min(56, bounds.h);
    int count = 0, preceding = 0;
    for (size_t i = 0; i < CHARACTER_FAMILY_LIMIT; ++i) {
        if (view->family_members[row][i].portrait) {
            ++count;
            preceding += i < index;
        }
    }
    const int step = count > 0 ? SDL_min(diameter, bounds.w / count) : diameter;
    const int origin = bounds.x + (row < 2 ? SDL_max(0, (bounds.w - count * step) / 2) : 0);
    return (SDL_Rect){origin + preceding * step, bounds.y, diameter, diameter};
}

const struct ruler_profile *character_view_pick(const struct character_view *view, SDL_Point point, unsigned *id)
{
    *id = 0;
    if (view->partner.portrait && !view->partner.profile.deceased &&
        SDL_PointInRect(&point, &(SDL_Rect){view->spouse.x, view->spouse.y, 100, 100}))
        return &view->partner.profile;
    if (view->show_vassals) {
        for (size_t i = 0; i < SDL_arraysize(view->vassals); ++i) {
            const int y = view->vassal_list.y + 14 + (int)i * view->vassal_entry.h;
            const SDL_Rect row = {view->vassal_list.x, y, view->vassal_list.w, view->vassal_entry.h};
            if (SDL_PointInRect(&point, &row) && SDL_PointInRect(&point, &view->vassal_list)) {
                *id = view->vassals[i].id;
                break;
            }
        }
    } else {
        for (size_t row = CHARACTER_FAMILY_ROWS; row-- > 0;)
            for (size_t i = CHARACTER_FAMILY_LIMIT; i-- > 0;) {
                const struct character_relative *relative = &view->family_members[row][i];
                const SDL_Rect area = relative_area(view, row, i);
                if (relative->portrait && SDL_PointInRect(&point, &area))
                    return &relative->profile;
            }
    }
    return NULL;
}

static int text_width(const struct menu_assets *assets, const char *text, size_t length)
{
    int width = 0;
    for (size_t i = 0; i < length; ++i)
        width += assets->glyphs[(unsigned char)text[i]].advance;
    return width;
}

void panel_tooltip(SDL_Renderer *renderer, const struct menu_assets *assets, SDL_Point pointer,
                   const char *heading, const char *body, panel_text_fn text)
{
    enum { MAX_LINES = 28, LINE_HEIGHT = 18, PAD = 10 };
    struct { const char *text; size_t size; int width; } lines[MAX_LINES];
    size_t count = 0;
    int viewport_width, viewport_height;
    SDL_RenderGetLogicalSize(renderer, &viewport_width, &viewport_height);
    const int max_width = SDL_max(1, SDL_min(560, viewport_width - 2 * PAD));
    const size_t heading_length = strlen(heading);
    const int heading_width = SDL_min(max_width, text_width(assets, heading, heading_length));
    int width = SDL_max(120, heading_width);
    while (body && *body && count < SDL_arraysize(lines)) {
        const char *end = body, *space = NULL;
        int advance = 0;
        while (*end && *end != '\n') {
            const int next = assets->glyphs[(unsigned char)*end].advance;
            if (advance + next > max_width && end != body)
                break;
            if (*end == ' ')
                space = end;
            advance += next;
            ++end;
        }
        if (*end && *end != '\n' && space && space > body)
            end = space;
        lines[count].text = body;
        lines[count].size = (size_t)(end - body);
        lines[count].width = text_width(assets, body, lines[count].size);
        width = SDL_max(width, lines[count].width);
        ++count;
        body = end;
        if (*body == '\n')
            ++body;
        while (*body == ' ')
            ++body;
    }
    width += 2 * PAD;
    const int height = 30 + (count ? 8 + (int)count * LINE_HEIGHT : 0);
    const SDL_Rect box = {SDL_clamp(pointer.x + 8, 0, SDL_max(0, viewport_width - width)),
                          SDL_clamp(pointer.y + 18, 0, SDL_max(0, viewport_height - height)), width, height};
    SDL_BlendMode previous;
    SDL_GetRenderDrawBlendMode(renderer, &previous);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 12, 11, 13, 238);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawBlendMode(renderer, previous);
    SDL_SetRenderDrawColor(renderer, 132, 113, 72, 255);
    SDL_RenderDrawRect(renderer, &box);
    SDL_RenderSetClipRect(renderer, &box);
    if (!strcmp(heading, "Not developed yet"))
        SDL_SetTextureColorMod(assets->font, 245, 82, 77);
    else
        SDL_SetTextureColorMod(assets->font, 241, 209, 137);
    text(renderer, assets, heading, heading_length, box.x + PAD + heading_width / 2, box.y + 4);
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    if (count)
        SDL_RenderDrawLine(renderer, box.x + PAD, box.y + 26, box.x + box.w - PAD, box.y + 26);
    for (size_t i = 0; i < count; ++i)
        text(renderer, assets, lines[i].text, lines[i].size, box.x + PAD + lines[i].width / 2,
             box.y + 30 + (int)i * LINE_HEIGHT);
    SDL_RenderSetClipRect(renderer, NULL);
}

void character_view_draw(const struct character_view *view, SDL_Renderer *renderer,
                         const struct menu_assets *assets, bool player, const struct campaign *campaign, SDL_Point pointer, panel_text_fn text)
{
    draw_frame(&view->background, renderer, 0);
    if (!view->show_vassals)
        draw_frame(&view->family, renderer, 0);
    setup_widget_draw(&view->close, renderer, pointer);
    for (size_t i = 0; i < SDL_arraysize(view->extra_icons); ++i)
        if (view->extra_icons[i].texture)
            setup_widget_draw(&view->extra_icons[i], renderer, pointer);
    SDL_Rect area = view->portrait;
    area.w = area.h = 150;
    if (view->portrait_texture)
        SDL_RenderCopy(renderer, view->portrait_texture, NULL, &area);
    const struct realm_title_info *title = view->has_display_title ? &view->display_title : NULL;
    const char rank = title ? title->id[0] : '\0';
    const int frame = title_rank_frame(rank);
    draw_frame(&view->frame, renderer, frame);
    area = view->shield;
    if (view->arms) {
        SDL_QueryTexture(view->arms, NULL, NULL, &area.w, &area.h);
        SDL_RenderCopy(renderer, view->arms, NULL, &area);
        draw_frame(&view->crown, renderer, frame);
    }
    if (view->dynasty_arms && view->dynasty_shield_area.w >= 0) {
        area = view->dynasty_shield_area;
        /* Respect the original GUI widget's dimensions, not the atlas output. */
        if (area.w <= 0) area.w = 55;
        if (area.h <= 0) area.h = 62;
        SDL_RenderCopy(renderer, view->dynasty_arms, NULL, &area);
    }
    const struct setup_widget *const icons[] = {&view->gold_icon, &view->prestige_icon, &view->piety_icon};
    const SDL_Rect resource_values[] = {view->gold, view->prestige, view->piety};
    SDL_SetTextureColorMod(assets->font, 35, 25, 15);
    for (size_t i = 0; i < RULER_RESOURCE_COUNT; ++i) {
        char amount[32] = "-";
        if (player && campaign && i == RULER_WEALTH)
            snprintf(amount, sizeof amount, "%u", campaign->treasury);
        else if (player && campaign && i == RULER_PRESTIGE)
            snprintf(amount, sizeof amount, "%.0f", campaign->prestige);
        else if (player && campaign && i == RULER_PIETY)
            snprintf(amount, sizeof amount, "%.0f", campaign->piety);
        else if (view->ruler.known_resources & (1u << i))
            snprintf(amount, sizeof amount, "%.0f", view->ruler.resources[i]);
        draw_frame(icons[i], renderer, 0);
        text(renderer, assets, amount, strlen(amount), resource_values[i].x + resource_values[i].w / 2, resource_values[i].y);
    }
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    if (view->ruler.religion_icon)
        draw_frame(&view->religion, renderer, (int)view->ruler.religion_icon - 1);

    const bool partner = view->partner.portrait && !view->partner.profile.deceased;
    if (partner) {
        area = view->spouse;
        area.w = area.h = 100;
        SDL_RenderCopy(renderer, view->partner.portrait, NULL, &area);
        draw_frame(&view->spouse_frame, renderer, 0);
        draw_frame(&view->spouse_plate, renderer, 0);
    }
    char name[224], age[16];
    const char *rank_name = view->rank_names[view->ruler.female][frame];
    snprintf(name, sizeof name, "%s%s%s%s%s", rank_name, *rank_name ? " " : "", view->ruler.name,
             title ? " of " : "", title ? title->name : "");
    snprintf(age, sizeof age, "%u", view->ruler.age);
    for (size_t i = 0; i < SDL_arraysize(view->labels); ++i) {
        if ((i == SPOUSE && !partner) || (i == AGE && !view->ruler.age_known) ||
            (view->show_vassals && i >= PARENTS))
            continue;
        if (i >= TAB && i < PARENTS) {
            const struct setup_widget *tab = &view->tabs[i - TAB];
            draw_frame(tab, renderer, i == TAB + (view->show_vassals ? 2 : 0) ? 1 : 0);
        }
        const char *value = i == NAME ? name : i == AGE ? age : i == DYNASTY ? view->ruler.dynasty
            : i == CULTURE ? view->ruler.culture : i == SPOUSE ? view->spouse_labels[view->partner.profile.female] : view->text[i];
        area = view->labels[i];
        SDL_RenderSetClipRect(renderer, &area);
        const bool dark = i < TAB;
        SDL_SetTextureColorMod(assets->font, dark ? 35 : 255, dark ? 25 : 255, dark ? 15 : 255);
        text(renderer, assets, value, strlen(value), area.x + area.w / 2, area.y + (area.h - 22) / 2);
        SDL_RenderSetClipRect(renderer, NULL);
    }
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    if (view->show_vassals) {
        SDL_RenderSetClipRect(renderer, &view->vassal_list);
        for (size_t i = 0; i < SDL_arraysize(view->vassals) && view->vassals[i].label[0]; ++i) {
            const int x = view->vassal_list.x, y = view->vassal_list.y + 14 + (int)i * view->vassal_entry.h;
            struct setup_widget bg = view->vassal_bg, frame = view->vassal_frame;
            bg.area.x += x;
            bg.area.y += y;
            frame.area.x += x;
            frame.area.y += y;
            draw_frame(&bg, renderer, 0);
            SDL_RenderCopy(renderer, view->vassals[i].portrait, NULL, &(SDL_Rect){
                x + view->vassal_portrait.x, y + view->vassal_portrait.y, 50, 50});
            draw_frame(&frame, renderer, 0);
            SDL_RenderCopy(renderer, view->vassals[i].shield, NULL, &(SDL_Rect){
                x + view->vassal_shield.x, y + view->vassal_shield.y, 30, 34});
            const SDL_Rect label = view->vassal_name;
            const char *value = view->vassals[i].label;
            SDL_SetTextureColorMod(assets->font, 35, 25, 15);
            text(renderer, assets, value, strlen(value), x + label.x + label.w / 2, y + label.y);
            SDL_SetTextureColorMod(assets->font, 255, 255, 255);
        }
        SDL_RenderSetClipRect(renderer, NULL);
    }
    static const char *const stat_keys[] = {
        "DIPLOMACY_MOD", "MARTIAL_MOD", "STEWARDSHIP_MOD", "INTRIGUE_MOD", "LEARNING_MOD"
    };
    static const char *const stat_fallbacks[] = {
        "Diplomacy", "Martial", "Stewardship", "Intrigue", "Learning"
    };
    const char *hover = NULL;
    const char *help = NULL;
    char stat_hint[128], stat_detail[4096], claim_hint[120], relation_hint[128], relation_name[240];
    /* CK2 defines combat_skill separately from the five primary attributes.
       Display an explicit unknown value until a verified source supplies it. */
    if (view->combat_skill_area.w > 0)
        text(renderer, assets, "-", 1, view->combat_skill_area.x + view->combat_skill_area.w / 2,
             view->combat_skill_area.y);
    for (size_t i = 0; i < RULER_STAT_COUNT; ++i) {
        const SDL_Rect value = view->stat_values[i];
        const bool known = (view->ruler.known_stats & (1u << i)) != 0;
        char number[16] = "-";
        if (known)
            snprintf(number, sizeof number, "%lld", (long long)view->ruler.stats[i] + view->trait_bonus[i]);
        draw_frame(&view->stat_icons[i], renderer, 0);
        text(renderer, assets, number, strlen(number), value.x + value.w / 2, value.y);
        if (SDL_PointInRect(&pointer, &view->stat_icons[i].area) || SDL_PointInRect(&pointer, &value)) {
            snprintf(stat_hint, sizeof stat_hint, "%s: %s", ck2_localize(stat_keys[i], stat_fallbacks[i]), number);
            if (known)
                snprintf(stat_detail, sizeof stat_detail, "Base: %d", view->ruler.stats[i]);
            else
                SDL_strlcpy(stat_detail, "Base: unavailable", sizeof stat_detail);
            for (size_t trait = 0; trait < view->ruler.trait_count; ++trait) {
                const int bonus = view->trait_modifiers[trait][i];
                if (!bonus)
                    continue;
                const size_t offset = strlen(stat_detail);
                snprintf(stat_detail + offset, sizeof stat_detail - offset, "\n%s: %+d",
                         view->trait_names[trait], bonus);
            }
            hover = stat_hint;
        }
    }
    if (view->dynasty_arms && SDL_PointInRect(&pointer, &(SDL_Rect){
        view->dynasty_shield_area.x, view->dynasty_shield_area.y,
        view->dynasty_shield_area.w > 0 ? view->dynasty_shield_area.w : 55,
        view->dynasty_shield_area.h > 0 ? view->dynasty_shield_area.h : 62})) {
        hover = view->ruler.dynasty;
        help = ck2_localize("DYNASTY_DRO", "This is the dynasty of this character.");
    }
    if (view->ruler.religion[0] && SDL_PointInRect(&pointer, &view->religion.area))
        hover = view->ruler.religion;
    if (partner && SDL_PointInRect(&pointer, &(SDL_Rect){view->spouse.x, view->spouse.y, 100, 100}))
        hover = view->partner.profile.name;
    for (size_t row = 0; !view->show_vassals && row < CHARACTER_FAMILY_ROWS; ++row)
        for (size_t i = 0; i < CHARACTER_FAMILY_LIMIT; ++i) {
            const struct character_relative *relative = &view->family_members[row][i];
            if (!relative->portrait)
                continue;
            const SDL_Rect box = relative_area(view, row, i);
            SDL_RenderCopy(renderer, relative->portrait, NULL, &box);
            if (SDL_PointInRect(&pointer, &box))
                hover = relative->profile.name;
        }
    unsigned relation_id;
    const struct ruler_profile *relative = character_view_pick(view, pointer, &relation_id);
    if (relative) {
        hover = relative->name;
        if (relative == &view->partner.profile && view->has_display_title && frame >= 4) {
            snprintf(relation_name, sizeof relation_name, "%s %s of %s",
                     view->rank_names[relative->female][frame], relative->name, view->display_title.name);
            hover = relation_name;
        }
        if (relative->age_known) {
            snprintf(relation_hint, sizeof relation_hint, "%s%sAge: %u\n%s",
                     relative == &view->partner.profile ? view->spouse_labels[relative->female] : "",
                     relative == &view->partner.profile ? "\n" : "", relative->age,
                     relative->deceased ? "Deceased" : "Click to view character");
            help = relation_hint;
        }
    } else if (relation_id) {
        for (size_t i = 0; i < SDL_arraysize(view->vassals); ++i)
            if (view->vassals[i].id == relation_id) {
                hover = view->vassals[i].label;
                help = "Click to view character";
                break;
            }
    }
    for (size_t i = 0; i < view->title_count; ++i) {
        const SDL_Rect shield = {view->titles_area.x + (int)i * view->title_entry.w, view->titles_area.y,
                                 view->title_entry.w, view->title_entry.h};
        struct setup_widget crown = view->title_crown;
        crown.area.x += shield.x;
        crown.area.y += shield.y;
        if (view->title_shields[i]) {
            SDL_RenderCopy(renderer, view->title_shields[i], NULL, &shield);
            draw_frame(&crown, renderer, title_rank_frame(view->titles[i].id[0]));
        }
        if (SDL_PointInRect(&pointer, &shield))
            hover = view->titles[i].name;
    }
    const size_t visible_claims = SDL_min(view->claim_count, (size_t)(view->claims_area.w / view->title_entry.w));
    for (size_t i = 0; i < visible_claims; ++i) {
        const SDL_Rect shield = {view->claims_area.x + (int)i * view->title_entry.w, view->claims_area.y,
                                 view->title_entry.w, view->title_entry.h};
        if (view->claim_shields[i])
            SDL_RenderCopy(renderer, view->claim_shields[i], NULL, &shield);
        if (!view->claim_weak[i] && view->claim_shields[i]) {
            struct setup_widget strong = view->strong_claim;
            strong.area.x += shield.x;
            strong.area.y += shield.y;
            draw_frame(&strong, renderer, 0);
        }
        if (SDL_PointInRect(&pointer, &shield)) {
            snprintf(claim_hint, sizeof claim_hint, "%s", view->claim_names[i]);
            hover = claim_hint;
            help = view->claim_weak[i] ? "Historical weak claim" : "Historical claim";
        }
    }
    for (size_t i = 0; i < SDL_arraysize(view->section_icons); ++i) {
        draw_frame(&view->section_icons[i], renderer, 0);
        if (SDL_PointInRect(&pointer, &view->section_icons[i].area)) {
            hover = view->section_labels[i];
            help = view->section_help[i];
        }
    }
    for (size_t i = 0; i < RULER_RESOURCE_COUNT; ++i)
        if (SDL_PointInRect(&pointer, &icons[i]->area) || SDL_PointInRect(&pointer, &resource_values[i])) {
            hover = view->resource_labels[i];
            help = view->resource_help[i];
        }
    const size_t visible_traits = SDL_min(view->ruler.trait_count, (size_t)(view->trait_area.w / view->trait_step));
    for (size_t i = 0; i < visible_traits; ++i) {
        struct setup_widget icon = view->trait_icons[i];
        icon.area.x = view->trait_area.x + (int)i * view->trait_step;
        icon.area.y = view->trait_area.y;
        draw_frame(&icon, renderer, 0);
        if (SDL_PointInRect(&pointer, &icon.area))
            hover = view->trait_names[i];
    }
    for (size_t i = 1; i < SDL_arraysize(view->tabs); ++i)
        if (i != 2 && SDL_PointInRect(&pointer, &view->tabs[i].area))
            hover = "Not developed yet";
    for (size_t i = 0; i < SDL_arraysize(view->extra_icons); ++i)
        if (view->extra_icons[i].texture &&
            SDL_PointInRect(&pointer, &view->extra_icons[i].area)) {
            hover = view->extra_icon_labels[i];
            help = "Original CK2 interface control; action not yet connected";
        }
    if (hover && *hover)
        panel_tooltip(renderer, assets, pointer, hover, hover == stat_hint ? stat_detail : help, text);
}

void character_view_free(struct character_view *view)
{
    clear_family(view);
    SDL_DestroyTexture(view->arms);
    SDL_DestroyTexture(view->dynasty_arms);
    dynasty_arms_close(view->coa_runtime);
    SDL_DestroyTexture(view->portrait_texture);
    for (size_t i = 0; i < SDL_arraysize(view->vassals); ++i) {
        SDL_DestroyTexture(view->vassals[i].portrait);
        SDL_DestroyTexture(view->vassals[i].shield);
    }
    struct setup_widget *sprites[] = {&view->background, &view->family, &view->close, &view->frame,
                                      &view->spouse_frame, &view->spouse_plate, &view->religion,
                                      &view->vassal_bg, &view->vassal_frame, &view->crown, &view->gold_icon, &view->title_crown, &view->strong_claim, &view->prestige_icon, &view->piety_icon};
    for (size_t i = 0; i < SDL_arraysize(sprites); ++i)
        SDL_DestroyTexture(sprites[i]->texture);
    for (size_t i = 0; i < SDL_arraysize(view->extra_icons); ++i)
        SDL_DestroyTexture(view->extra_icons[i].texture);
    for (size_t i = 0; i < SDL_arraysize(view->tabs); ++i)
        SDL_DestroyTexture(view->tabs[i].texture);
    for (size_t i = 0; i < RULER_STAT_COUNT; ++i)
        SDL_DestroyTexture(view->stat_icons[i].texture);
    for (size_t i = 0; i < RULER_TRAIT_LIMIT; ++i)
        SDL_DestroyTexture(view->trait_icons[i].texture);
    for (size_t i = 0; i < SDL_arraysize(view->title_shields); ++i)
        SDL_DestroyTexture(view->title_shields[i]);
    for (size_t i = 0; i < SDL_arraysize(view->section_icons); ++i)
        SDL_DestroyTexture(view->section_icons[i].texture);
    for (size_t i = 0; i < SDL_arraysize(view->claim_shields); ++i)
        SDL_DestroyTexture(view->claim_shields[i]);
    *view = (struct character_view){0};
}
