#include "map_theme_core.h"
#include <math.h>
#include <stddef.h>

uint32_t map_theme_key_color(const char *key)
{
    if (!key || !*key) return 0;
    uint32_t hash = 2166136261u;
    for (const unsigned char *p = (const unsigned char *)key; *p; ++p) {
        hash ^= *p;
        hash *= 16777619u;
    }
    /* Stable saturated colors; the original title/religion palette may differ. */
    const uint32_t r = 60u + ((hash >> 16) & 127u);
    const uint32_t g = 65u + ((hash >> 8) & 127u);
    const uint32_t b = 63u + (hash & 127u);
    return r << 16 | g << 8 | b;
}

uint32_t map_theme_economy_color(double annual_tax, bool known)
{
    if (!known || !isfinite(annual_tax) || annual_tax < 0) return 0;
    double t = fmin(1., annual_tax / 60.);
    const uint32_t r = (uint32_t)(197. - 141. * t);
    const uint32_t g = (uint32_t)(70. + 127. * t);
    const uint32_t b = (uint32_t)(49. + 73. * t);
    return r << 16 | g << 8 | b;
}
