#include "wonder_navigation_core.h"
size_t wonder_navigation_index(const unsigned *ids, size_t count,
                               unsigned current, int direction)
{
    if (!ids || !count) return 0;
    size_t found = count;
    for (size_t i = 0; i < count; ++i)
        if (current && ids[i] == current) {found = i; break;}
    if (found == count) return 0;
    if (direction > 0) return (found + 1) % count;
    if (direction < 0) return (found + count - 1) % count;
    return found;
}
