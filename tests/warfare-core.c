#include "../src/game/warfare_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned owner(void *context, unsigned province)
{
    (void)context;
    return province <= 2 ? 100 : 200;
}
static bool has_event(const struct warfare_day_report *r, enum warfare_event_kind kind)
{
    for (size_t i = 0; i < r->count; ++i)
        if (r->events[i].kind == kind) return true;
    return false;
}
int main(void)
{
    struct warfare_graph graph = {0};
    assert(warfare_graph_init(&graph, 8));
    assert(warfare_graph_connect(&graph, 1, 2));
    assert(warfare_graph_connect(&graph, 2, 3));
    assert(warfare_graph_connect(&graph, 3, 4));
    assert(warfare_graph_connect(&graph, 4, 5));
    assert(!warfare_graph_connect(&graph, 0, 2));
    assert(!warfare_graph_adjacent(&graph, 1, 3));
    assert(warfare_graph_adjacent(&graph, 3, 2));
    unsigned *route = NULL; size_t n = 0;
    assert(warfare_graph_route(&graph, 1, 4, &route, &n));
    assert(n == 4 && route[0] == 1 && route[1] == 2 && route[2] == 3 && route[3] == 4);
    free(route);
    assert(!warfare_graph_route(&graph, 1, 7, &route, &n));
    struct warfare_state *s = calloc(1, sizeof *s);
    assert(s);
    assert(warfare_declare(s, 100, 200) == WARFARE_OK);
    assert(warfare_hostile(s, 200, 100));
    assert(warfare_declare(s, 200, 100) == WARFARE_ALREADY);
    unsigned one, two;
    assert(warfare_raise(s, 100, 1, 1000, &one) == WARFARE_OK);
    assert(warfare_raise(s, 200, 3, 300, &two) == WARFARE_OK);
    assert(warfare_raise(s, 100, 1, 1000, NULL) == WARFARE_ALREADY);
    assert(warfare_march(s, &graph, one, 3) == WARFARE_OK);
    struct warfare_day_report report;
    warfare_day(s, owner, NULL, &report);
    assert(warfare_army(s, one)->province_id == 1);
    warfare_day(s, owner, NULL, &report);
    assert(warfare_army(s, one)->province_id == 2 && has_event(&report, WARFARE_ARRIVED));
    warfare_day(s, owner, NULL, &report);
    warfare_day(s, owner, NULL, &report);
    assert(has_event(&report, WARFARE_BATTLE));
    assert(warfare_army(s, two) == NULL);
    assert(warfare_army(s, one)->troops == 800);
    assert(has_event(&report, WARFARE_SIEGE_STARTED));
    for (int i = 0; i < 30; ++i) warfare_day(s, owner, NULL, &report);
    assert(warfare_occupation_holder(s, 3) == 100);
    assert(has_event(&report, WARFARE_OCCUPIED));
    assert(warfare_peace(s, 100, 200) == WARFARE_OK);
    assert(!warfare_hostile(s, 100, 200));
    assert(warfare_occupation_holder(s, 3) == 0);
    assert(warfare_disband(s, one) == WARFARE_OK);
    assert(warfare_raise(s, 100, 1, 1000, NULL) == WARFARE_ALREADY);
    warfare_state_free(s);
    free(s);
    warfare_graph_free(&graph);
    puts("warfare core: all assertions passed");
    return 0;
}
