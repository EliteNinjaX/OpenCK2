#include "../src/game/tree_history.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *fixture = "1 = { name = Andronikos dynasty = 100 father = 10 "
       "1250.1.1 = { birth = yes } 1300.1.1 = { dynasty = 200 } }\n"
       "2 = { name = Michael dynasty = 100 father = 1 1290.1.1 = { birth = yes } "
       "1340.1.1 = { death = yes } }\n"
       "3 = { name = Future dynasty = 100 1400.1.1 = { birth = yes } }\n";
    struct tree_history history = {0};
    assert(tree_history_add_file(&history, fixture, 13370101));
    assert(history.count == 2);
    assert(tree_history_find(&history, 1)->dynasty == 200);
    assert(tree_history_find(&history, 2)->father == 1);
    assert(tree_history_dynasty_count(&history, 100, true, 13370101) == 1);
    tree_history_free(&history);
    puts("tree history: all assertions passed");
}
