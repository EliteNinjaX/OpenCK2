#ifndef OPENCK2_RELIGION_GFX_CORE_H
#define OPENCK2_RELIGION_GFX_CORE_H
#include <stdbool.h>
#include <stddef.h>
/* Resolve a religion's graphical_culture or its group's inherited graphical_culture.
 * Source: original CK2 common/religions directory, with no hardcoded religion IDs. */
bool religion_gfx_from_text(const char *source, const char *religion,
                            char *style, size_t size);
#endif
