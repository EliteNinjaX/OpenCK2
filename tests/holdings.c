#define _XOPEN_SOURCE 700
#include "asset_paths.h"
#include "holdings.h"
#include "ck2_script.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void fixture(const char *root, const char *relative, const char *text)
{
    char path[256];
    SDL_snprintf(path, sizeof path, "%s/%s", root, relative);
    if (!text) {
        assert(remove(path) == 0);
        return;
    }
    FILE *file = fopen(path, "w");
    assert(file && fputs(text, file) >= 0 && fclose(file) == 0);
}

static void directory(const char *root, const char *relative, bool create)
{
    char path[256];
    SDL_snprintf(path, sizeof path, "%s/%s", root, relative);
    assert((create ? mkdir(path, 0700) : rmdir(path)) == 0);
}

static void check(const char *root, const char *title, const char *holding)
{
    struct province_map map = {0};
    assert(province_map_load(&map, root) == 0);
    const struct province *province = province_by_id(&map, 1);
    assert(province);
    char *data = province_history_read(province, root);
    assert(data && is(field(data, "title"), title));
    SDL_free(data);

    struct county_holdings county;
    holdings_load(&county, root, province, (struct campaign_date){769, 1, 1});
    assert(county.count == 1 && county.capacity == 3);
    assert(county.slots[0].type == HOLDING_CITY && !strcmp(county.slots[0].id, holding));

    holdings_load(&county, root, province, (struct campaign_date){850, 1, 1});
    assert(county.count == 2 && county.capacity == 3);
    assert(county.slots[1].type == HOLDING_TEMPLE);
    holdings_load(&county, root, province, (struct campaign_date){950, 1, 1});
    assert(county.count == 1 && county.capacity == 3);
    assert(county.slots[0].type == HOLDING_TEMPLE);
    province_map_free(&map);
}

int main(void)
{
    char root[] = "/tmp/openck2-holdings-XXXXXX";
    assert(mkdtemp(root));
    const char *const directories[] = {
        "map", "history", "history/provinces", "mod", "mod/history", "mod/history/provinces"
    };
    for (size_t i = 0; i < SDL_arraysize(directories); ++i)
        directory(root, directories[i], true);

    char bitmap[256];
    SDL_snprintf(bitmap, sizeof bitmap, "%s/map/provinces.bmp", root);
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_ARGB8888);
    assert(surface && SDL_SaveBMP(surface, bitmap) == 0);
    SDL_FreeSurface(surface);
    fixture(root, "map/definition.csv", "1;0;0;0;Active;x\n");
    fixture(root, "history/provinces/1 - Active.txt",
            "title = c_active\nmax_settlements = 3\nb_town = city\n"
            "800.1.1 = { b_church = temple }\n900.1.1 = { remove_settlement = b_town }\n");
    fixture(root, "history/provinces/1 - Stale.txt", "title = c_stale\nb_ghost = tribal\n");
    check(root, "c_active", "b_town");

    fixture(root, "mod/history/provinces/1 - Renamed.txt",
            "title = c_mod\nmax_settlements = 3\nb_port = city\n"
            "800.1.1 = { b_church = temple }\n900.1.1 = { remove_settlement = b_port }\n");
    char descriptor[256], text[256];
    SDL_snprintf(descriptor, sizeof descriptor, "%s/test.mod", root);
    SDL_snprintf(text, sizeof text, "path = \"%s/mod\"\n", root);
    fixture(root, "test.mod", text);
    struct content_entry mod = {.name = "holdings test", .source = descriptor,
                                .enabled = true, .installed = true};
    const struct content_catalog catalog = {.mods = {&mod, 1}};
    ck2_mount_content(root, &catalog);
    check(root, "c_mod", "b_port");
    ck2_unmount_content();

    fixture(root, "mod/history/provinces/1 - Renamed.txt", NULL);
    fixture(root, "test.mod", NULL);
    fixture(root, "history/provinces/1 - Stale.txt", NULL);
    fixture(root, "history/provinces/1 - Active.txt", NULL);
    fixture(root, "map/definition.csv", NULL);
    assert(unlink(bitmap) == 0);
    for (size_t i = SDL_arraysize(directories); i > 0; --i)
        directory(root, directories[i - 1], false);
    assert(rmdir(root) == 0);
    return 0;
}
