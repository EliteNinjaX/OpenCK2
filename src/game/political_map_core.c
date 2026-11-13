#include "political_map_core.h"

static enum political_edge check(const int *realm, const uint32_t *province,
                                 size_t a, size_t b)
{
    if (realm[a] < 0 || realm[b] < 0) return POLITICAL_NO_EDGE;
    if (realm[a] != realm[b]) return POLITICAL_REALM_EDGE;
    if (province[a] && province[b] && province[a] != province[b])
        return POLITICAL_COUNTY_EDGE;
    return POLITICAL_NO_EDGE;
}
enum political_edge political_map_edge(const int *realm, const uint32_t *province,
                                       size_t width, size_t height, size_t x, size_t y)
{
    if (!realm || !province || !width || !height || x >= width || y >= height)
        return POLITICAL_NO_EDGE;
    const size_t at = y * width + x;
    if (realm[at] < 0) return POLITICAL_NO_EDGE;
    enum political_edge edge = POLITICAL_NO_EDGE;
    if (x > 0) {
        enum political_edge other = check(realm, province, at, at - 1);
        if (other > edge) edge = other;
    }
    if (y > 0) {
        enum political_edge other = check(realm, province, at, at - width);
        if (other > edge) edge = other;
    }
    if (x + 1 < width) {
        enum political_edge other = check(realm, province, at, at + 1);
        if (other > edge) edge = other;
    }
    if (y + 1 < height) {
        enum political_edge other = check(realm, province, at, at + width);
        if (other > edge) edge = other;
    }
    return edge;
}
uint32_t political_map_pixel(uint32_t rgb, enum political_edge edge)
{
    const uint32_t r = (rgb >> 16) & 255u, g = (rgb >> 8) & 255u, b = rgb & 255u;
    const uint32_t neutral = (r + g + b + 145u) / 4u;
    const uint32_t blended = ((4u*r + neutral)/5u << 16) |
                             ((4u*g + neutral)/5u << 8) | ((4u*b + neutral)/5u);
    if (edge == POLITICAL_REALM_EDGE) return 0xed25202cu;
    if (edge == POLITICAL_COUNTY_EDGE) return 0xc0000000u | ((blended & 0xfefefeu) >> 1);
    return 0xa0000000u | blended;
}
