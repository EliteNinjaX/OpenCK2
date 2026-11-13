#ifndef OPENCK2_WONDER_NAVIGATION_CORE_H
#define OPENCK2_WONDER_NAVIGATION_CORE_H
#include <stddef.h>
/* Return the index of a built Great Work at current province, or the
 * nearest next/previous entry. Missing current defaults to first work. */
size_t wonder_navigation_index(const unsigned *province_ids, size_t count,
                               unsigned current, int direction);
#endif
