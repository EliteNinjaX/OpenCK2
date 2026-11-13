#ifndef OPENCK2_BLOODLINE_BROWSER_CORE_H
#define OPENCK2_BLOODLINE_BROWSER_CORE_H
#include <stddef.h>

/* The original 661-pixel list holds two 340-pixel bloodline_entry cards per
 * row. We render three rows; scrolling keeps complete rows together. */
size_t bloodline_browser_page(size_t first, size_t count, int direction);
#endif
