#ifndef OPENCK2_MAP_THEME_CORE_H
#define OPENCK2_MAP_THEME_CORE_H
#include <stdbool.h>
#include <stdint.h>
/* Deterministic display palettes, not CK2's original undocumented shader. */
uint32_t map_theme_key_color(const char *key);
uint32_t map_theme_economy_color(double annual_tax, bool known);
#endif
