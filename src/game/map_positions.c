#include "map_positions.h"
#include "ck2_lexer_core.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

static bool read_point(struct ck2_lexer *l, struct map_position *position)
{
    char x_text[48], y_text[48], *end;
    ck2_span_copy(x_text, sizeof x_text, ck2_lex(l));
    ck2_span_copy(y_text, sizeof y_text, ck2_lex(l));
    const double x = strtod(x_text, &end);
    const bool valid_x = x_text[0] && !*end && isfinite(x);
    const double y = strtod(y_text, &end);
    const bool valid_y = y_text[0] && !*end && isfinite(y);
    if (valid_x && valid_y && x >= 0 && y >= 0 && x < 200000 && y < 200000) {
        position->x=(float)x; position->y=(float)y; position->available=true;
    }
    ck2_skip_scope(l);
    return true;
}

static bool parse_id(struct map_positions *map, struct ck2_span id, struct ck2_lexer *lex)
{
    char text[24], *end;
    ck2_span_copy(text, sizeof text, id);
    const unsigned long number = strtoul(text, &end, 10);
    if (!text[0] || *end || !number || number > 100000ul) {
        ck2_skip_scope(lex); /* The block wasn't a province. */
        return true;
    }
    struct map_position position = {0};
    struct ck2_span key;
    while ((key = ck2_lex(lex)).length && !ck2_span_eq(key, "}")) {
        if (!ck2_span_eq(ck2_lex(lex), "=")) return false;
        const struct ck2_span value = ck2_lex(lex);
        if (ck2_span_eq(value, "{")) {
            if (ck2_span_eq(key, "position")) read_point(lex, &position);
            else if (ck2_span_eq(key, "port")) {
                struct map_position port={0};
                read_point(lex,&port);
                if (port.available) position.authored_port=true;
            } else ck2_skip_scope(lex);
        }
    }
    if (!ck2_span_eq(key, "}")) return false;
    if (!position.available) return true;
    if (number >= map->capacity) {
        size_t next = map->capacity ? map->capacity : 512;
        while (next <= number) next *= 2;
        struct map_position *items = realloc(map->by_id, next * sizeof *items);
        if (!items) return false;
        memset(items + map->capacity, 0, (next - map->capacity) * sizeof *items);
        map->by_id = items;
        map->capacity = next;
    }
    if (!map->by_id[number].available) ++map->count;
    map->by_id[number] = position;
    return true;
}

bool map_positions_parse(struct map_positions *map, const char *contents)
{
    if (!map || !contents) return false;
    struct ck2_lexer l = {contents};
    struct ck2_span id;
    while ((id = ck2_lex(&l)).length) {
        if (!ck2_span_eq(ck2_lex(&l), "=") || !ck2_span_eq(ck2_lex(&l), "{")) return false;
        if (!parse_id(map, id, &l)) return false;
    }
    return true;
}

const struct map_position *map_position_find(const struct map_positions *map, unsigned id)
{
    return map && id < map->capacity && map->by_id[id].available ? map->by_id + id : NULL;
}
void map_positions_destroy(struct map_positions *map)
{
    if (!map) return;
    free(map->by_id);
    *map = (struct map_positions){0};
}
