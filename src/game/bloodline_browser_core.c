#include "bloodline_browser_core.h"
size_t bloodline_browser_page(size_t first, size_t count, int direction)
{
    const size_t rows = (count / 2) + (count % 2);
    const size_t max_first = rows > 3 ? (rows - 3) * 2 : 0;
    first = first > max_first ? max_first : first - first % 2;
    if (direction > 0) return first < 2 ? 0 : first - 2;
    if (direction < 0) return first >= max_first ? max_first : first + 2;
    return first;
}
