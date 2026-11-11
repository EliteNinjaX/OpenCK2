#include "ck2_script.h"
#include "holdings.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct campaign_date date;
    char extra;
    if (argc != 3 || sscanf(argv[2], "%4d.%2d.%2d%c", &date.year, &date.month, &date.day, &extra) != 3 ||
        date.year < 1 || date.month < 1 || date.month > 12 || date.day < 1 || date.day > 31) {
        fprintf(stderr, "Usage: holding-audit CK2_DIRECTORY YEAR.MONTH.DAY\n");
        return 1;
    }

    struct province_map map = {0};
    if (province_map_load(&map, argv[1]) < 0) {
        fprintf(stderr, "%s\n", SDL_GetError());
        province_map_free(&map);
        return 1;
    }

    int status = 0;
    puts("province\tmap_name\thistory_file\tcounty\tcapacity\tholdings");
    for (size_t i = 0; i < map.count; ++i) {
        const struct province *province = &map.provinces[i];
        if (!province->land)
            continue;
        char *data = province_history_read(province, argv[1]);
        if (!data) {
            fprintf(stderr, "Could not read province %u from %s\n", province->id, province->history_path);
            status = 1;
            continue;
        }
        const struct token title = field(data, "title");
        struct county_holdings holdings;
        holdings_read(&holdings, data, date);
        printf("%u\t%s\t%s\t%.*s\t%zu\t", province->id, province->name,
               province->history_path, (int)title.size, title.size ? title.start : "",
               holdings.capacity);
        for (size_t slot = 0; slot < holdings.count; ++slot) {
            static const char *const types[] = {"empty", "castle", "city", "temple", "tribal", "nomad"};
            printf("%s%s=%s", slot ? "," : "", holdings.slots[slot].id, types[holdings.slots[slot].type]);
        }
        putchar('\n');
        SDL_free(data);
    }
    province_map_free(&map);
    return status;
}
