#ifndef OPENCK2_TREE_HISTORY_H
#define OPENCK2_TREE_HISTORY_H
#include <stdbool.h>
#include <stddef.h>
struct tree_person {
    unsigned id, father, mother, dynasty;
    int birth, death;
    char name[96];
};
struct tree_history {
    struct tree_person *people;
    size_t count, capacity;
};
/* Parse CK2 history/characters data at a requested bookmark date. The parser
 * tracks only documented identity and family fields; it does not simulate CK2
 * births, marriages, adoption or bloodline propagation. */
bool tree_history_add_file(struct tree_history *, const char *, int date);
void tree_history_free(struct tree_history *);
const struct tree_person *tree_history_find(const struct tree_history *, unsigned id);
size_t tree_history_dynasty_count(const struct tree_history *, unsigned dynasty, bool living, int date);
#endif
