#include "warfare_core.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

bool warfare_graph_init(struct warfare_graph *g, size_t n)
{
    if (!g || !n || n > INT_MAX / 2) return false;
    int *head = malloc(n * sizeof *head);
    if (!head) return false;
    for (size_t i = 0; i < n; ++i) head[i] = -1;
    warfare_graph_free(g);
    g->head = head;
    g->node_count = n;
    return true;
}
void warfare_graph_free(struct warfare_graph *g)
{
    if (!g) return;
    free(g->head); free(g->edges);
    *g = (struct warfare_graph){0};
}
bool warfare_graph_adjacent(const struct warfare_graph *g, unsigned a, unsigned b)
{
    if (!g || !g->head || a >= g->node_count || b >= g->node_count) return false;
    for (int edge = g->head[a]; edge >= 0; edge = g->edges[edge].next)
        if (g->edges[edge].to == b) return true;
    return false;
}
bool warfare_graph_connect(struct warfare_graph *g, unsigned a, unsigned b)
{
    if (!g || !g->head || !a || !b || a == b || a >= g->node_count || b >= g->node_count)
        return false;
    if (warfare_graph_adjacent(g, a, b)) return true;
    if (g->edge_count > INT_MAX - 2) return false;
    if (g->edge_count + 2 > g->edge_capacity) {
        size_t capacity = g->edge_capacity ? g->edge_capacity * 2 : 128;
        if (capacity < g->edge_count + 2 || capacity > SIZE_MAX / sizeof *g->edges) return false;
        void *memory = realloc(g->edges, capacity * sizeof *g->edges);
        if (!memory) return false;
        g->edges = memory;
        g->edge_capacity = capacity;
    }
    g->edges[g->edge_count] = (struct warfare_edge){b, g->head[a]};
    g->head[a] = (int)g->edge_count++;
    g->edges[g->edge_count] = (struct warfare_edge){a, g->head[b]};
    g->head[b] = (int)g->edge_count++;
    return true;
}
bool warfare_graph_route(const struct warfare_graph *g, unsigned start, unsigned end,
                         unsigned **route, size_t *length)
{
    if (!route || !length) return false;
    *route = NULL; *length = 0;
    if (!g || !g->head || !start || !end || start >= g->node_count || end >= g->node_count)
        return false;
    unsigned *parent = malloc(g->node_count * sizeof *parent);
    unsigned *queue = malloc(g->node_count * sizeof *queue);
    if (!parent || !queue) { free(parent); free(queue); return false; }
    for (size_t i = 0; i < g->node_count; ++i) parent[i] = UINT_MAX;
    size_t front = 0, back = 0;
    parent[start] = start;
    queue[back++] = start;
    while (front < back && parent[end] == UINT_MAX) {
        unsigned at = queue[front++];
        for (int edge = g->head[at]; edge >= 0; edge = g->edges[edge].next) {
            unsigned to = g->edges[edge].to;
            if (parent[to] == UINT_MAX) {
                parent[to] = at;
                queue[back++] = to;
            }
        }
    }
    if (parent[end] == UINT_MAX) { free(parent); free(queue); return false; }
    size_t count = 1;
    for (unsigned at = end; at != start; at = parent[at]) ++count;
    unsigned *path = malloc(count * sizeof *path);
    if (!path) { free(parent); free(queue); return false; }
    size_t i = count;
    for (unsigned at = end; ; at = parent[at]) {
        path[--i] = at;
        if (at == start) break;
    }
    free(parent); free(queue);
    *route = path; *length = count;
    return true;
}
void warfare_state_free(struct warfare_state *s)
{
    if (!s) return;
    for (size_t i = 0; i < s->army_count; ++i) free(s->armies[i].route);
    *s = (struct warfare_state){0};
}
bool warfare_hostile(const struct warfare_state *s, unsigned a, unsigned b)
{
    if (!s || !a || !b || a == b) return false;
    for (size_t i = 0; i < s->war_count; ++i) {
        const struct warfare_war *w = &s->wars[i];
        if (w->active && ((w->attacker == a && w->defender == b) ||
                          (w->attacker == b && w->defender == a))) return true;
    }
    return false;
}
enum warfare_status warfare_declare(struct warfare_state *s, unsigned a, unsigned b)
{
    if (!s || !a || !b || a == b) return WARFARE_INVALID;
    if (warfare_hostile(s, a, b)) return WARFARE_ALREADY;
    for (size_t i = 0; i < s->war_count; ++i) {
        if (!s->wars[i].active) {
            s->wars[i] = (struct warfare_war){a, b, true};
            return WARFARE_OK;
        }
    }
    if (s->war_count == WARFARE_MAX_WARS) return WARFARE_FULL;
    s->wars[s->war_count++] = (struct warfare_war){a, b, true};
    return WARFARE_OK;
}
enum warfare_status warfare_peace(struct warfare_state *s, unsigned a, unsigned b)
{
    if (!s || !a || !b) return WARFARE_INVALID;
    for (size_t i = 0; i < s->war_count; ++i) {
        struct warfare_war *war = s->wars + i;
        if (!war->active || !((war->attacker == a && war->defender == b) ||
                              (war->attacker == b && war->defender == a))) continue;
        war->active = false;
        for (size_t j = 0; j < s->occupation_count;) {
            struct warfare_occupation *o = s->occupations + j;
            if ((o->occupier_id == a && o->defender_id == b) ||
                (o->occupier_id == b && o->defender_id == a)) {
                s->occupations[j] = s->occupations[--s->occupation_count];
            } else ++j;
        }
        return WARFARE_OK;
    }
    return WARFARE_NO_WAR;
}
struct warfare_army *warfare_army(struct warfare_state *s, unsigned id)
{
    if (!s || !id) return NULL;
    for (size_t i = 0; i < s->army_count; ++i)
        if (s->armies[i].id == id) return s->armies + i;
    return NULL;
}
static void delete_army(struct warfare_state *s, size_t index)
{
    free(s->armies[index].route);
    s->armies[index] = s->armies[--s->army_count];
}
enum warfare_status warfare_raise(struct warfare_state *s, unsigned ruler,
                                  unsigned province, unsigned troops, unsigned *army_id)
{
    if (!s || !ruler || !province) return WARFARE_INVALID;
    if (!troops) return WARFARE_NO_TROOPS;
    if (s->army_count == WARFARE_MAX_ARMIES ||
        s->levy_source_count == WARFARE_MAX_OCCUPATIONS || s->next_army_id == UINT_MAX)
        return WARFARE_FULL;
    for (size_t i = 0; i < s->levy_source_count; ++i)
        if (s->levy_sources[i].ruler_id == ruler && s->levy_sources[i].province_id == province)
            return WARFARE_ALREADY; /* No infinite re-raising after disband/casualties. */
    unsigned id = ++s->next_army_id;
    s->armies[s->army_count++] = (struct warfare_army){
        .id = id, .ruler_id = ruler, .province_id = province,
        .origin = province, .troops = troops
    };
    s->levy_sources[s->levy_source_count++] = (struct warfare_levy_source){ruler, province};
    if (army_id) *army_id = id;
    return WARFARE_OK;
}
enum warfare_status warfare_disband(struct warfare_state *s, unsigned id)
{
    if (!s || !id) return WARFARE_INVALID;
    for (size_t i = 0; i < s->army_count; ++i) if (s->armies[i].id == id) {
        delete_army(s, i);
        return WARFARE_OK;
    }
    return WARFARE_NO_ARMY;
}
enum warfare_status warfare_march(struct warfare_state *s, const struct warfare_graph *g,
                                  unsigned id, unsigned destination)
{
    struct warfare_army *a = warfare_army(s, id);
    if (!a || !destination) return WARFARE_NO_ARMY;
    unsigned *path; size_t count;
    if (!warfare_graph_route(g, a->province_id, destination, &path, &count)) return WARFARE_NO_PATH;
    if (count == 1) {
        free(path); free(a->route);
        a->route = NULL; a->route_count = a->route_step = a->travel_days = 0;
        a->target = destination;
        return WARFARE_OK;
    }
    free(a->route);
    a->route = path;
    a->route_count = count;
    a->route_step = 0;
    a->target = destination;
    a->travel_days = count > 1 ? 2 : 0; /* Placeholder per-edge travel speed. */
    a->siege_days = 0;
    return WARFARE_OK;
}
unsigned warfare_occupation_holder(const struct warfare_state *s, unsigned province)
{
    if (!s || !province) return 0;
    for (size_t i = 0; i < s->occupation_count; ++i)
        if (s->occupations[i].province_id == province) return s->occupations[i].occupier_id;
    return 0;
}
static void report_event(struct warfare_day_report *report, struct warfare_event event)
{
    if (report && report->count < WARFARE_MAX_EVENTS) report->events[report->count++] = event;
}
static unsigned losses(unsigned troops, unsigned numerator, unsigned denominator)
{
    return (unsigned)(((unsigned long long)troops * numerator + denominator - 1) / denominator);
}
void warfare_day(struct warfare_state *s, warfare_owner_lookup lookup, void *userdata,
                 struct warfare_day_report *report)
{
    if (report) *report = (struct warfare_day_report){0};
    if (!s) return;
    /* Move at most one adjacent land edge every two days. */
    for (size_t i = 0; i < s->army_count; ++i) {
        struct warfare_army *a = s->armies + i;
        if (a->route && a->route_step + 1 < a->route_count && a->travel_days) {
            if (--a->travel_days == 0) {
                a->province_id = a->route[++a->route_step];
                a->siege_days = 0;
                report_event(report, (struct warfare_event){.kind = WARFARE_ARRIVED,
                    .army_id = a->id, .actor_id = a->ruler_id, .province_id = a->province_id});
                if (a->route_step + 1 < a->route_count) a->travel_days = 2;
                else { free(a->route); a->route = NULL; a->route_count = 0; a->route_step = 0; }
            }
        }
    }
    /* Resolve pairs at war only. No battles between neutral or allied armies. */
    for (size_t i = 0; i < s->army_count; ++i) {
        for (size_t j = i + 1; j < s->army_count;) {
            struct warfare_army *a = s->armies + i, *b = s->armies + j;
            if (a->province_id != b->province_id || !warfare_hostile(s, a->ruler_id, b->ruler_id)) {
                ++j; continue;
            }
            const bool a_wins = a->troops >= b->troops;
            size_t loser_index = a_wins ? j : i, winner_index = a_wins ? i : j;
            unsigned loser_troops = s->armies[loser_index].troops;
            unsigned winner_troops = s->armies[winner_index].troops;
            unsigned loser_losses = losses(loser_troops, 2, 5);
            unsigned winner_losses = losses(winner_troops, 1, 5);
            struct warfare_event e = { .kind = WARFARE_BATTLE,
                .province_id = a->province_id,
                .actor_id = s->armies[winner_index].ruler_id,
                .opponent_id = s->armies[loser_index].ruler_id,
                .army_id = s->armies[winner_index].id,
                .actor_losses = winner_losses, .opponent_losses = loser_losses };
            s->armies[winner_index].troops -= winner_losses;
            /* Prototype: defeated army routed off the map, not a real CK2 retreat. */
            delete_army(s, loser_index);
            report_event(report, e);
            if (loser_index == i) { --i; break; }
            /* The swapped army at j must still be examined. */
        }
    }
    /* Unopposed besiegers occupy after a provisional 30 days. This is separate
     * from historical title ownership, and cleared by a peace treaty. */
    if (!lookup) return;
    for (size_t i = 0; i < s->army_count; ++i) {
        struct warfare_army *a = s->armies + i;
        if (a->travel_days || !a->troops) continue;
        unsigned defender = lookup(userdata, a->province_id);
        if (!warfare_hostile(s, a->ruler_id, defender) ||
            warfare_occupation_holder(s, a->province_id)) { a->siege_days = 0; continue; }
        if (!a->siege_days) {
            a->siege_days = 30;
            report_event(report, (struct warfare_event){.kind = WARFARE_SIEGE_STARTED,
                .actor_id = a->ruler_id, .opponent_id = defender,
                .province_id = a->province_id, .army_id = a->id});
        } else if (--a->siege_days == 0 && s->occupation_count < WARFARE_MAX_OCCUPATIONS) {
            s->occupations[s->occupation_count++] = (struct warfare_occupation){
                a->province_id, a->ruler_id, defender};
            report_event(report, (struct warfare_event){.kind = WARFARE_OCCUPIED,
                .actor_id = a->ruler_id, .opponent_id = defender,
                .province_id = a->province_id, .army_id = a->id});
        }
    }
}
const char *warfare_status_text(enum warfare_status status)
{
    static const char *const names[] = {
        "OK", "Invalid command or ID", "No active war", "Already active or raised",
        "Army not found", "No land route", "Warfare limit reached", "No troops available"
    };
    return (unsigned)status < sizeof names / sizeof *names ? names[status] : "Unknown error";
}
