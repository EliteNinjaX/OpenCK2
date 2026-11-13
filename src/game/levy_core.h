#ifndef OPENCK2_LEVY_CORE_H
#define OPENCK2_LEVY_CORE_H

#include <stdbool.h>
#include <stddef.h>

enum levy_troop_type {
    LEVY_LIGHT_INFANTRY, LEVY_HEAVY_INFANTRY, LEVY_PIKEMEN, LEVY_ARCHERS,
    LEVY_LIGHT_CAVALRY, LEVY_HEAVY_CAVALRY, LEVY_HORSE_ARCHERS,
    LEVY_CAMEL_CAVALRY, LEVY_WAR_ELEPHANTS, LEVY_TROOP_COUNT
};

extern const char *const levy_troop_names[LEVY_TROOP_COUNT];

/* Authored static/building modifiers; no live army state. All troop counts
 * and levy_size/garrison_size are additive within a building level. */
struct levy_effect {
    double troops[LEVY_TROOP_COUNT];
    double levy_size, garrison_size, levy_reinforce_rate, fort_level;
};

struct levy_potential {
    double troops[LEVY_TROOP_COUNT];
    double total, garrison_size_modifier, reinforcement_modifier, fort_level;
    bool known;
};

/* Zero is a valid authored troop amount. Unknown keys are ignored. */
bool levy_effect_assign(struct levy_effect *effect, const char *key, double amount);
void levy_effect_add(struct levy_effect *dst, const struct levy_effect *src);
struct levy_potential levy_potential_calculate(const struct levy_effect *effect);

#endif
