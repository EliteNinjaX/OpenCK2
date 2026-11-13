#ifndef OPENCK2_WARFARE_CORE_H
#define OPENCK2_WARFARE_CORE_H
#include <stdbool.h>
#include <stddef.h>

/* Runtime warfare prototype; numbers are provisional and NOT CK2 combat rules.
 * The original landed-title history remains read-only. */
enum { WARFARE_MAX_WARS = 32, WARFARE_MAX_ARMIES = 96,
       WARFARE_MAX_OCCUPATIONS = 512, WARFARE_MAX_EVENTS = 32 };
enum warfare_status {
    WARFARE_OK, WARFARE_INVALID, WARFARE_NO_WAR, WARFARE_ALREADY,
    WARFARE_NO_ARMY, WARFARE_NO_PATH, WARFARE_FULL, WARFARE_NO_TROOPS
};
struct warfare_edge { unsigned to; int next; };
struct warfare_graph {
    int *head;
    size_t node_count;
    struct warfare_edge *edges;
    size_t edge_count, edge_capacity;
};
struct warfare_war { unsigned attacker, defender; bool active; };
struct warfare_army {
    unsigned id, ruler_id, province_id, troops, origin, target;
    unsigned *route;
    size_t route_count, route_step;
    unsigned travel_days, siege_days;
};
struct warfare_occupation { unsigned province_id, occupier_id, defender_id; };
struct warfare_levy_source { unsigned ruler_id, province_id; };
enum warfare_event_kind {
    WARFARE_ARRIVED, WARFARE_BATTLE, WARFARE_SIEGE_STARTED,
    WARFARE_OCCUPIED, WARFARE_ARMY_DESTROYED
};
struct warfare_event {
    enum warfare_event_kind kind;
    unsigned province_id, actor_id, opponent_id, army_id, actor_losses, opponent_losses;
};
struct warfare_day_report {
    struct warfare_event events[WARFARE_MAX_EVENTS];
    size_t count;
};
struct warfare_state {
    struct warfare_war wars[WARFARE_MAX_WARS];
    struct warfare_army armies[WARFARE_MAX_ARMIES];
    struct warfare_occupation occupations[WARFARE_MAX_OCCUPATIONS];
    struct warfare_levy_source levy_sources[WARFARE_MAX_OCCUPATIONS];
    size_t war_count, army_count, occupation_count, levy_source_count;
    unsigned next_army_id;
};
typedef unsigned (*warfare_owner_lookup)(void *userdata, unsigned province_id);

bool warfare_graph_init(struct warfare_graph *graph, size_t node_count);
bool warfare_graph_connect(struct warfare_graph *graph, unsigned a, unsigned b);
bool warfare_graph_adjacent(const struct warfare_graph *graph, unsigned a, unsigned b);
bool warfare_graph_route(const struct warfare_graph *graph, unsigned start, unsigned end,
                         unsigned **route, size_t *length);
void warfare_graph_free(struct warfare_graph *graph);
void warfare_state_free(struct warfare_state *state);
enum warfare_status warfare_declare(struct warfare_state *state, unsigned attacker, unsigned defender);
bool warfare_hostile(const struct warfare_state *state, unsigned a, unsigned b);
enum warfare_status warfare_peace(struct warfare_state *state, unsigned a, unsigned b);
enum warfare_status warfare_raise(struct warfare_state *state, unsigned ruler,
                                  unsigned province, unsigned troops, unsigned *army_id);
struct warfare_army *warfare_army(struct warfare_state *state, unsigned id);
enum warfare_status warfare_disband(struct warfare_state *state, unsigned id);
enum warfare_status warfare_march(struct warfare_state *state, const struct warfare_graph *graph,
                                  unsigned army_id, unsigned destination);
void warfare_day(struct warfare_state *state, warfare_owner_lookup owner, void *userdata,
                 struct warfare_day_report *report);
unsigned warfare_occupation_holder(const struct warfare_state *state, unsigned province_id);
const char *warfare_status_text(enum warfare_status status);
#endif
