#include "levy_core.h"
#include <math.h>
#include <string.h>

const char *const levy_troop_names[LEVY_TROOP_COUNT] = {
    "light_infantry", "heavy_infantry", "pikemen", "archers",
    "light_cavalry", "heavy_cavalry", "horse_archers",
    "camel_cavalry", "war_elephants"
};

bool levy_effect_assign(struct levy_effect *effect, const char *key, double amount)
{
    if (!effect || !key || !isfinite(amount)) return false;
    for (size_t i = 0; i < LEVY_TROOP_COUNT; ++i) {
        if (!strcmp(key, levy_troop_names[i])) {
            effect->troops[i] += amount;
            return true;
        }
    }
    struct modifier { const char *name; double *value; } keys[] = {
        {"levy_size", &effect->levy_size},
        {"garrison_size", &effect->garrison_size},
        {"levy_reinforce_rate", &effect->levy_reinforce_rate},
        {"fort_level", &effect->fort_level}
    };
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; ++i)
        if (!strcmp(key, keys[i].name)) { *keys[i].value += amount; return true; }
    return false;
}

void levy_effect_add(struct levy_effect *dst, const struct levy_effect *src)
{
    if (!dst || !src) return;
    for (size_t i = 0; i < LEVY_TROOP_COUNT; ++i)
        dst->troops[i] += src->troops[i];
    dst->levy_size += src->levy_size;
    dst->garrison_size += src->garrison_size;
    dst->levy_reinforce_rate += src->levy_reinforce_rate;
    dst->fort_level += src->fort_level;
}

struct levy_potential levy_potential_calculate(const struct levy_effect *effect)
{
    struct levy_potential out = {0};
    if (!effect) return out;
    out.known = true;
    /* This is the potential baseline from static modifiers and built buildings,
     * NOT the CK2 live levy formula (martial, tech, laws, religion, raised pool,
     * occupation, and vassal obligations have not been applied). */
    const double multiplier = fmax(0., 1. + effect->levy_size);
    for (size_t i = 0; i < LEVY_TROOP_COUNT; ++i) {
        out.troops[i] = fmax(0., effect->troops[i] * multiplier);
        out.total += out.troops[i];
    }
    out.garrison_size_modifier = effect->garrison_size;
    out.reinforcement_modifier = effect->levy_reinforce_rate;
    out.fort_level = effect->fort_level;
    return out;
}
