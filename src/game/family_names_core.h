#ifndef OPENCK2_FAMILY_NAMES_CORE_H
#define OPENCK2_FAMILY_NAMES_CORE_H
#include <stdbool.h>
#include <stddef.h>
/* Select a culture-authored name from common/cultures text. The deterministic
 * selector is an OpenCK2 approximation, not CK2's weighted name generator. */
bool family_name_choose(const char *source, const char *culture,
                        bool female, unsigned seed, char *out, size_t capacity);
#endif
