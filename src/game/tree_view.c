#include "tree_view.h"
#include "tree_layout_core.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include "era.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int bookmark_date(struct campaign_date d)
{
    return d.year * 10000 + d.month * 100 + d.day;
}

/* All node links use indices into view->nodes, never CK2 character IDs. */
static int add_node(struct tree_view *view, unsigned id, const char *name, const char *title,
                    int column, int row, int parent)
{
    if (!id) return -1;
    for (size_t i = 0; i < view->count; ++i)
        if (view->nodes[i].id == id) return (int)i;
    if (view->count >= TREE_VISIBLE_LIMIT) return -1;
    const int result = (int)view->count++;
    struct tree_node *n = &view->nodes[result];
    *n = (struct tree_node){.id=id,.column=column,.row=row,.parent=parent,
                            .other_parent=-1,.spouse=-1};
    SDL_strlcpy(n->name, name && *name ? name : "?", sizeof n->name);
    if (title) SDL_strlcpy(n->title, title, sizeof n->title);
    return result;
}

static int add_relative(struct tree_view *view, const struct character_relative *relative,
                        int column, int row, int parent)
{
    return add_node(view, relative->profile.id, relative->profile.name, "", column, row, parent);
}

static void family_graph(struct tree_view *view, const struct character_view *character)
{
    const struct ruler_profile *r = &character->ruler;
    const int self = add_node(view, r->id, r->name, "", 2, 0, -1);
    int parent[2] = {-1,-1};
    for (int i = 0; i < 2; ++i)
        parent[i] = add_relative(view, &character->family_members[0][i], 3, i, -1);
    if (self >= 0) {
        view->nodes[self].parent = parent[0];
        view->nodes[self].other_parent = parent[1];
    }
    for (int i = 0; i < 4; ++i) {
        const int grand = add_relative(view, &character->family_members[1][i], 4, i, -1);
        if (parent[i/2] >= 0 && grand >= 0) {
            struct tree_node *n = &view->nodes[parent[i/2]];
            if (i % 2 == 0) n->parent = grand;
            else n->other_parent = grand;
        }
    }
    for (int i = 0; i < CHARACTER_FAMILY_LIMIT; ++i)
        add_relative(view, &character->family_members[2][i], 1, i, self);
    for (int i = 0; i < CHARACTER_FAMILY_LIMIT; ++i) {
        int sib = add_relative(view, &character->family_members[3][i], 2, i + 1,
                                parent[0] >= 0 ? parent[0] : parent[1]);
        if (sib >= 0 && parent[0] >= 0 && parent[1] >= 0)
            view->nodes[sib].other_parent = parent[1];
    }
    int spouse = add_relative(view, &character->partner, 2, 1, -1);
    if (self >= 0 && spouse >= 0) view->nodes[self].spouse = spouse;
}

static void load_history(struct tree_history *history, const char *root, int date)
{
    struct ck2_asset_files files = ck2_list_assets(root, "history/characters");
    for (size_t i = 0; i < files.count; ++i) {
        size_t n = strlen(files.names[i]);
        if (n < 4 || strcmp(files.names[i] + n - 4, ".txt")) continue;
        char *data = load_from_directory(root, "history/characters", files.names[i]);
        if (data) {
            if (!tree_history_add_file(history, data, date)) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Character genealogy truncated: allocation failed");
                SDL_free(data);
                break;
            }
            SDL_free(data);
        }
    }
    ck2_free_asset_files(&files);
}

static void dynasty_graph(struct tree_view *view, const struct character_view *character,
                          const struct tree_history *history, int date)
{
    const unsigned dynasty = character->ruler.dynasty_id;
    view->total = tree_history_dynasty_count(history, dynasty, false, date);
    view->living = tree_history_dynasty_count(history, dynasty, true, date);
    const int self = add_node(view, character->ruler.id, character->ruler.name, "", 2, 0, -1);
    const struct tree_person *focus = tree_history_find(history, character->ruler.id);
    if (!focus || self < 0 || !dynasty) return;
    unsigned parent_ids[2] = {focus->father, focus->mother};
    int parents[2] = {-1, -1};
    for (int i = 0; i < 2; ++i) {
        const struct tree_person *p = tree_history_find(history, parent_ids[i]);
        if (!p || p->dynasty != dynasty) continue;
        parents[i] = add_node(view, p->id, p->name, "", 1, i, -1);
        for (int g = 0; g < 2; ++g) {
            const struct tree_person *grand = tree_history_find(history,
                                                        g ? p->mother : p->father);
            if (!grand || grand->dynasty != dynasty) continue;
            int gi = add_node(view, grand->id, grand->name, "", 0, i*2+g, -1);
            if (parents[i] >= 0) {
                if (g == 0) view->nodes[parents[i]].parent = gi;
                else view->nodes[parents[i]].other_parent = gi;
            }
        }
    }
    view->nodes[self].parent = parents[0];
    view->nodes[self].other_parent = parents[1];
    int child_row = 0, sibling_row = 1, grand_row = 0;
    /* Only show relatives whose parentage is recorded. The previous version
     * filled the remaining space with unrelated dynasty members as 'peers'. */
    for (size_t i = 0; i < history->count && view->count < TREE_VISIBLE_LIMIT; ++i) {
        const struct tree_person *person = &history->people[i];
        if (person->dynasty != dynasty || person->id == focus->id) continue;
        if (person->father == focus->id || person->mother == focus->id) {
            add_node(view, person->id, person->name, "", 3, child_row++, self);
        } else if (parents[0] >= 0 && person->father == parent_ids[0]) {
            add_node(view, person->id, person->name, "", 2, sibling_row++, parents[0]);
        } else if (parents[1] >= 0 && person->mother == parent_ids[1]) {
            add_node(view, person->id, person->name, "", 2, sibling_row++, parents[1]);
        }
    }
    const size_t child_end = view->count;
    for (size_t j = 1; j < child_end && view->count < TREE_VISIBLE_LIMIT; ++j) {
        if (view->nodes[j].column != 3) continue;
        const unsigned child = view->nodes[j].id;
        for (size_t i = 0; i < history->count && view->count < TREE_VISIBLE_LIMIT; ++i) {
            const struct tree_person *person = &history->people[i];
            if (person->dynasty == dynasty && (person->father == child || person->mother == child))
                add_node(view, person->id, person->name, "", 4, grand_row++, (int)j);
        }
    }
}

static void realm_graph(struct tree_view *view, const struct character_view *character,
                        const struct realm_map *realms)
{
    const unsigned id = character->ruler.id;
    add_node(view, id, character->ruler.name,
             character->has_display_title ? character->display_title.name : "", 0, 0, -1);
    struct realm_title_info direct[18] = {0};
    const size_t total = realm_direct_vassals(realms, id, 0, direct, 18);
    view->total = total;
    for (size_t i = 0; i < total && i < 18 && view->count < TREE_VISIBLE_LIMIT; ++i) {
        const struct ruler_profile *profile = realm_ruler(realms, direct[i].holder);
        add_node(view, direct[i].holder, profile ? profile->name : direct[i].name,
                 direct[i].name, 1, (int)i, 0);
    }
    const size_t upper = view->count;
    int grand_row = 0;
    for (size_t i = 1; i < upper && view->count < TREE_VISIBLE_LIMIT; ++i) {
        struct realm_title_info next[3] = {0};
        const size_t count = realm_direct_vassals(realms, view->nodes[i].id, 0, next, 3);
        for (size_t j = 0; j < count && j < 3 && view->count < TREE_VISIBLE_LIMIT; ++j) {
            const struct ruler_profile *profile = realm_ruler(realms, next[j].holder);
            add_node(view, next[j].holder, profile ? profile->name : next[j].name,
                     next[j].name, 2, grand_row++, (int)i);
        }
    }
}

static void position_nodes(struct tree_view *view)
{
    for (size_t i = 0; i < view->count; ++i) {
        struct tree_node *n = &view->nodes[i];
        n->area = (SDL_Rect){view->viewport.x + n->world_x - view->scroll_x,
                             view->viewport.y + n->world_y - view->scroll_y,
                             TREE_LAYOUT_NODE_W,TREE_LAYOUT_NODE_H};
    }
}

static void place_nodes(struct tree_view *view)
{
    int top = view->mode == TREE_DYNASTY ? 162 : view->mode == TREE_FAMILY ? 107 : 132;
    view->viewport = (SDL_Rect){view->area.x + 22, view->area.y + top,
                                view->area.w - 44, view->area.h - top - 24};
    struct tree_layout_item items[TREE_VISIBLE_LIMIT];
    for (size_t i = 0; i < view->count; ++i)
        items[i] = (struct tree_layout_item){.column=view->nodes[i].column};
    const int columns = view->mode == TREE_REALM ? 3 : 5;
    struct tree_layout_bounds bounds = tree_layout_build(items, view->count, columns);
    view->world_width = bounds.width;
    view->world_height = bounds.height;
    for (size_t i = 0; i < view->count; ++i) {
        view->nodes[i].world_x = items[i].x;
        view->nodes[i].world_y = items[i].y;
    }
    position_nodes(view);
}

void tree_view_scroll(struct tree_view *view, int dx, int dy)
{
    if (!view || !view->open) return;
    const struct tree_layout_bounds bounds = {view->world_width,view->world_height};
    tree_layout_scroll(&view->scroll_x,&view->scroll_y,dx,dy,
                       bounds,view->viewport.w,view->viewport.h);
    position_nodes(view);
}

static bool bind(struct setup_widget *widget, const struct frontend_gui *gui, int parent,
                 const char *name, SDL_Renderer *renderer, const char *root, int shift_x, int shift_y)
{
    const int node = frontend_gui_find(gui, parent, name);
    SDL_Rect area;
    if (node < 0 || !frontend_gui_rect(gui, node, (SDL_Point){800,600}, &area)) return false;
    /* Correct only the window origin, without hardcoding child sprite locations. */
    area.x += shift_x; area.y += shift_y;
    return setup_widget_load_node(widget, renderer, root, gui, node, area) == 0;
}

void tree_view_close(struct tree_view *view)
{
    SDL_DestroyTexture(view->shield);
    SDL_DestroyTexture(view->background.texture);
    SDL_DestroyTexture(view->decorative.texture);
    SDL_DestroyTexture(view->close.texture);
    SDL_DestroyTexture(view->banner.texture);
    SDL_DestroyTexture(view->shield_frame.texture);
    SDL_DestroyTexture(view->member_frame.texture);
    for (size_t i = 0; i < view->count; ++i) SDL_DestroyTexture(view->nodes[i].portrait);
    *view = (struct tree_view){0};
}

int tree_view_open(struct tree_view *view, enum tree_mode mode, SDL_Renderer *renderer,
                   const char *root, const struct character_view *character,
                   const struct realm_map *realms, struct campaign_date date,
                   character_portrait_fn portrait, void *context)
{
    tree_view_close(view);
    view->mode = mode;
    view->area = mode == TREE_FAMILY ? (SDL_Rect){420,130,760,608} : (SDL_Rect){400,135,800,600};
    if (mode == TREE_DYNASTY)
        ck2_locale_format(view->heading, sizeof view->heading, "DYNATREE_HOUSEOF",
                          "House $HOUSENAME$", "HOUSENAME", character->ruler.dynasty);
    else
        SDL_strlcpy(view->heading, mode == TREE_REALM && character->has_display_title ?
                    character->display_title.name : character->ruler.name,
                    sizeof view->heading);
    if (mode == TREE_FAMILY) family_graph(view, character);
    else if (mode == TREE_REALM) realm_graph(view, character, realms);
    else {
        struct tree_history history = {0};
        load_history(&history, root, bookmark_date(date));
        dynasty_graph(view, character, &history, bookmark_date(date));
        tree_history_free(&history);
        /* Reuse character screen's authored dynasty coat-of-arms; we own a new copy. */
        view->shield = dynasty_arms_render(character->coa_runtime, renderer,
                   character->ruler.dynasty_id, character->ruler.religion_key, NULL);
    }
    struct frontend_gui gui = {0};
    if (frontend_gui_load_file(&gui, root, "interface/dynasty_tree.gui") == 0) {
        const char *parent_name = mode == TREE_FAMILY ? "FamilyTree" :
                                  mode == TREE_REALM ? "RealmTree" : "DynastyTree";
        const int parent = frontend_gui_find(&gui, -1, parent_name);
        const int dx = view->area.x - (mode == TREE_FAMILY ? 150 : 50);
        const int dy = view->area.y - (mode == TREE_FAMILY ? 100 : 50);
        if (parent >= 0) {
            bind(&view->background, &gui, parent, mode == TREE_FAMILY ? "dynatree_family_bg" : "Background",
                 renderer, root, dx, dy);
            bind(&view->close, &gui, parent, "Exit", renderer, root, dx, dy);
            if (mode != TREE_FAMILY) {
                bind(&view->banner, &gui, parent, "dynatree_dynbanner", renderer, root, dx, dy);
                bind(&view->decorative, &gui, parent, "dynatree_shield_deco", renderer, root, dx, dy);
                bind(&view->shield_frame, &gui, parent, "dynastic_shield", renderer, root, dx, dy);
            }
        }
        frontend_gui_free(&gui);
    }
    /* Use the original member-card artwork whenever it is present. */
    setup_widget_load_sprite(&view->member_frame, renderer, root,
          "interface/dynasty_tree.gfx", mode == TREE_REALM ?
          "GFX_realmtree_title_bg" : "GFX_DynaTree_Member", (SDL_Rect){0});
    place_nodes(view);
    /* Only construct portraits for the bounded graph, never for every dynasty member. */
    struct ruler_profile profiles[TREE_VISIBLE_LIMIT] = {0};
    for (size_t i = 0; i < view->count; ++i) profiles[i].id = view->nodes[i].id;
    qsort(profiles, view->count, sizeof *profiles, ruler_compare);
    rulers_load(profiles, view->count, root, date);
    for (size_t i = 0; i < view->count; ++i) {
        struct tree_node *n = &view->nodes[i];
        for (size_t j = 0; j < view->count; ++j) if (profiles[j].id == n->id) {
            if (profiles[j].name[0]) SDL_strlcpy(n->name, profiles[j].name, sizeof n->name);
            if (portrait) n->portrait = portrait(context, &profiles[j]);
            break;
        }
    }
    view->open = true;
    return 0;
}

void tree_view_draw(const struct tree_view *view, SDL_Renderer *renderer,
                    const struct menu_assets *assets, panel_text_fn text)
{
    if (!view->open) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 211, 194, 162, 255);
    SDL_RenderFillRect(renderer, &view->area);
    if (view->background.texture) setup_widget_draw(&view->background, renderer, (SDL_Point){-1,-1});
    if (view->banner.texture) setup_widget_draw(&view->banner, renderer, (SDL_Point){-1,-1});
    if (view->decorative.texture) setup_widget_draw(&view->decorative, renderer, (SDL_Point){-1,-1});
    if (view->shield_frame.texture) setup_widget_draw(&view->shield_frame, renderer, (SDL_Point){-1,-1});
    if (view->shield && view->mode == TREE_DYNASTY)
        SDL_RenderCopy(renderer, view->shield, NULL,
                       &(SDL_Rect){view->area.x + 68,view->area.y + 65,62,73});
    SDL_SetTextureColorMod(assets->font, 30, 23, 19);
    const char *head = view->mode == TREE_FAMILY ? ck2_localize("FAMILYTREE_INTERFACE_LABEL", "Family Tree") :
                       view->mode == TREE_REALM ? ck2_localize("REALM_TREE_INTERFACE_LABEL", "Realm Tree") :
                       ck2_localize("DYNASTYTREE_INTERFACE_LABEL", "Dynasty Tree");
    text(renderer, assets, head, strlen(head), view->area.x + view->area.w/2,view->area.y+17);
    text(renderer, assets, view->heading, strlen(view->heading),
         view->area.x + view->area.w/2,view->area.y+47);
    if (view->mode == TREE_DYNASTY) {
        char count[32], stats[192];
        snprintf(count, sizeof count, "%zu", view->living);
        ck2_locale_format(stats, sizeof stats, "DYNATREE_LIVINGMEMBERS",
                          "Living members: $LIVING$", "LIVING", count);
        text(renderer, assets, stats, strlen(stats),view->area.x+view->area.w/2,view->area.y+89);
        snprintf(count, sizeof count, "%zu", view->total);
        ck2_locale_format(stats, sizeof stats, "DYNATREE_TOTALMEMBERS",
                          "Total members: $TOTAL$", "TOTAL", count);
        text(renderer, assets, stats, strlen(stats),view->area.x+view->area.w/2,view->area.y+112);
    }
    /* The original window clips the scrollable tree, not its toolbar or banner. */
    SDL_RenderSetClipRect(renderer, &view->viewport);
    for (size_t i = 0; i < view->count; ++i) {
        const struct tree_node *node = &view->nodes[i];
        const int relatives[2] = {node->parent, node->other_parent};
        for (int j = 0; j < 2; ++j) {
            if (relatives[j] < 0 || (size_t)relatives[j] >= view->count) continue;
            const SDL_Rect p = view->nodes[relatives[j]].area;
            const int right = p.x < node->area.x;
            const int x1 = right ? p.x+p.w : p.x;
            const int x2 = right ? node->area.x : node->area.x+node->area.w;
            const int y1 = p.y+p.h/2, y2 = node->area.y+node->area.h/2;
            const int mid = x1 + (x2-x1)/2;
            SDL_SetRenderDrawColor(renderer, 102, 80, 61, 255);
            SDL_RenderDrawLine(renderer,x1,y1,mid,y1);
            SDL_RenderDrawLine(renderer,mid,y1,mid,y2);
            SDL_RenderDrawLine(renderer,mid,y2,x2,y2);
        }
        if (node->spouse >= 0 && (size_t)node->spouse < view->count) {
            const SDL_Rect spouse = view->nodes[node->spouse].area;
            SDL_SetRenderDrawColor(renderer, 139, 87, 75, 255);
            SDL_RenderDrawLine(renderer, node->area.x+node->area.w/2,
               node->area.y+node->area.h/2, spouse.x+spouse.w/2,spouse.y+spouse.h/2);
        }
    }
    for (size_t i = 0; i < view->count; ++i) {
        const struct tree_node *node = &view->nodes[i];
        if (view->member_frame.texture)
            SDL_RenderCopy(renderer, view->member_frame.texture, NULL, &node->area);
        else {
            SDL_SetRenderDrawColor(renderer, 55, 45, 34, 225);
            SDL_RenderFillRect(renderer, &node->area);
        }
        SDL_SetRenderDrawColor(renderer, i == 0 ? 219 : 165,
                               i == 0 ? 176 : 140, 95, 255);
        SDL_RenderDrawRect(renderer, &node->area);
        if (node->portrait)
            SDL_RenderCopy(renderer, node->portrait, NULL,
                &(SDL_Rect){node->area.x + 39,node->area.y + 2,40,40});
        if (view->member_frame.texture && view->mode != TREE_REALM)
            SDL_SetTextureColorMod(assets->font, 48, 36, 26);
        else SDL_SetTextureColorMod(assets->font, 255, 245, 211);
        text(renderer, assets, node->name, strlen(node->name),
             node->area.x + node->area.w/2,node->area.y + 43);
        if (node->title[0])
            text(renderer, assets, node->title, strlen(node->title),
                 node->area.x + node->area.w/2,node->area.y + 57);
    }
    SDL_RenderSetClipRect(renderer, NULL);
    SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    if (view->close.texture) setup_widget_draw(&view->close, renderer, (SDL_Point){-1,-1});
    else {
        SDL_SetRenderDrawColor(renderer, 85, 32, 30, 255);
        SDL_RenderFillRect(renderer, &(SDL_Rect){view->area.x + view->area.w - 35,view->area.y+9,27,27});
        text(renderer, assets, "X", 1, view->area.x+view->area.w-21,view->area.y+11);
    }
}

unsigned tree_view_click(const struct tree_view *view, SDL_Point point)
{
    if (!view->open) return 0;
    SDL_Rect close = view->close.texture ? view->close.area :
        (SDL_Rect){view->area.x + view->area.w - 35,view->area.y+9,27,27};
    if (SDL_PointInRect(&point, &close)) return UINT_MAX;
    if (!SDL_PointInRect(&point, &view->viewport)) return 0;
    for (size_t i = 0; i < view->count; ++i)
        if (SDL_PointInRect(&point, &view->nodes[i].area)) return view->nodes[i].id;
    return 0;
}
