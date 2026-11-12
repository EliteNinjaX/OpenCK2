#ifndef OPENCK2_SETTLEMENT_STYLES_H
#define OPENCK2_SETTLEMENT_STYLES_H
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct settlement_culture {
    char key[72], group[72], graphics[64];
    uint32_t rgb;
    bool has_color;
};
struct settlement_actor {
    char key[112], path[256];
};
struct settlement_styles {
    struct settlement_culture *cultures;
    size_t culture_count, culture_capacity;
    struct settlement_actor *actors;
    size_t actor_count, actor_capacity;
};
bool settlement_styles_parse_cultures(struct settlement_styles *, const char *source);
bool settlement_styles_parse_actors(struct settlement_styles *, const char *source);
const struct settlement_culture *settlement_culture_find(const struct settlement_styles *, const char *key);
/* Returns NULL if this combination has no model in the original definition. */
const struct settlement_actor *settlement_actor_find(const struct settlement_styles *, const char *graphical,
                                                    const char *holding_type, unsigned tier);
void settlement_styles_destroy(struct settlement_styles *);
#endif
