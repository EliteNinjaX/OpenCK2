#define _XOPEN_SOURCE 700
#include "setup_toolbar.h"
#include "asset_paths.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *const widgets[] = {
    "mapmode_terrain", "mapmode_realms", "mapmode_defacto_counties",
    "mapmode_defacto_duchies", "mapmode_defacto_kingdoms", "mapmode_religion",
    "mapmode_culture", "mapmode_governments", "alternate_start_shattered_button",
    "alternate_start_random_button", "shattered_ornament", "random_ornament", "frontend_banner1"
};

static void gui(const char *path, int x, const char *sprite, const char *orientation)
{
    FILE *file = fopen(path, "w");
    assert(file);
    fputs("guiTypes = { windowType = { name = top_window position = { x = 2 y = 3 }\n", file);
    for (size_t i = 0; i < SDL_arraysize(widgets); ++i) {
        fprintf(file, "guiButtonType = { name = %s position = { x = %d y = 10 } "
                "Orientation = %s quadTextureSprite = %s %s }\n",
                widgets[i], x + (int)i * 20, orientation, i ? "Art" : sprite,
                i == 0 ? "shortcut = q" : i == 1 ? "shortcut = w" : "");
    }
    assert(fputs("} }\n", file) >= 0 && fclose(file) == 0);
}

static void word(Uint8 *buffer, size_t offset, Uint32 value)
{
    value = SDL_SwapLE32(value);
    memcpy(buffer + offset, &value, sizeof value);
}

static void dds(const char *path)
{
    Uint8 data[128 + 32 * 4] = {0};
    memcpy(data, "DDS ", 4);
    word(data, 4, 124);
    word(data, 12, 1);
    word(data, 16, 32);
    word(data, 76, 32);
    word(data, 80, 0x41);
    word(data, 88, 32);
    word(data, 92, 0x00ff0000);
    word(data, 96, 0x0000ff00);
    word(data, 100, 0x000000ff);
    word(data, 104, 0xff000000);
    FILE *file = fopen(path, "wb");
    assert(file && fwrite(data, sizeof data, 1, file) == 1 && fclose(file) == 0);
}

int main(int argc, char **argv)
{
    SDL_Surface *screen = SDL_CreateRGBSurfaceWithFormat(0, 640, 480, 32, SDL_PIXELFORMAT_ARGB8888);
    assert(screen);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(screen);
    assert(renderer);
    struct setup_toolbar toolbar = {0};
    if (argc == 2) {
        if (setup_lobby_load(&toolbar, renderer, argv[1], (SDL_Point){1600, 900}) < 0) {
            fprintf(stderr, "%s\n", SDL_GetError());
            return EXIT_FAILURE;
        }
        assert(toolbar.widgets[SETUP_TERRAIN].area.w > 0);
        assert(toolbar.frontend.count > 1000);
        assert(frontend_gui_find(&toolbar.frontend, -1, "IronmanSaveSelectDialog") >= 0);
        assert(frontend_gui_find(&toolbar.frontend, -1, "customize_window") >= 0);
        assert(toolbar.lobby[LOBBY_DESCRIPTION].area.w > 0);
        assert(toolbar.lobby[LOBBY_EXPANSIONS].area.y < toolbar.lobby[LOBBY_BOTTOM].area.y);
        assert(toolbar.lobby[LOBBY_RIGHT].area.x > 800);
        assert(toolbar.lobby[LOBBY_PLAY].area.y < toolbar.lobby[LOBBY_BACK].area.y);
        const SDL_Rect date = toolbar.lobby[LOBBY_DAY_UP].area;
        assert(setup_lobby_deferred(&toolbar, (SDL_Point){date.x, date.y}));
        assert(setup_toolbar_shortcut(&toolbar, SDLK_q) == SETUP_TERRAIN);
        goto done;
    }

    char root[] = "/tmp/openck2-toolbar-XXXXXX";
    assert(mkdtemp(root));
    char directory[256], layout[256], art[256], image[256], tga[256], mod[256], mod_interface[256];
    char mod_gui[256], mod_gfx[256], descriptor[256];
    SDL_snprintf(directory, sizeof directory, "%s/interface", root);
    SDL_snprintf(layout, sizeof layout, "%s/interface/frontend.gui", root);
    SDL_snprintf(art, sizeof art, "%s/interface/main.gfx", root);
    SDL_snprintf(image, sizeof image, "%s/sheet.dds", root);
    SDL_snprintf(tga, sizeof tga, "%s/sheet.tga", root);
    SDL_snprintf(mod, sizeof mod, "%s/mod", root);
    SDL_snprintf(mod_interface, sizeof mod_interface, "%s/mod/interface", root);
    SDL_snprintf(mod_gui, sizeof mod_gui, "%s/mod/interface/frontend.gui", root);
    SDL_snprintf(mod_gfx, sizeof mod_gfx, "%s/mod/interface/frontend.gfx", root);
    SDL_snprintf(descriptor, sizeof descriptor, "%s/test.mod", root);
    assert(mkdir(directory, 0700) == 0 && mkdir(mod, 0700) == 0 && mkdir(mod_interface, 0700) == 0);
    FILE *file = fopen(art, "w");
    assert(file);
    assert(fputs("spriteTypes = { spriteType = { name = Art texturefile = sheet.tga noOfFrames = 2 } }",
                 file) >= 0 && fclose(file) == 0);
    dds(image);
    gui(layout, -100, "Art", "CENTER_UP");
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) == 0);
    assert(toolbar.widgets[SETUP_TERRAIN].area.w == 16);
    assert(setup_toolbar_hit(&toolbar, (SDL_Point){222, 13}) == SETUP_TERRAIN);
    assert(setup_toolbar_shortcut(&toolbar, SDLK_w) == SETUP_REALMS);
    assert(setup_toolbar_shortcut(&toolbar, SDLK_UNKNOWN) == SETUP_NO_CONTROL);
    gui(layout, -150, "Art", "CENTER_UP");
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) == 0);
    assert(setup_toolbar_hit(&toolbar, (SDL_Point){172, 13}) == SETUP_TERRAIN);
    assert(setup_toolbar_load(&toolbar, renderer, root, 800) == 0);
    assert(setup_toolbar_hit(&toolbar, (SDL_Point){252, 13}) == SETUP_TERRAIN);

    gui(mod_gui, 8, "Art", "UPPER_LEFT");
    file = fopen(mod_gfx, "w");
    assert(file);
    assert(fputs("spriteTypes = { spriteType = { name = Art textureFile = sheet.dds noOfFrames = 4 } }",
                 file) >= 0 && fclose(file) == 0);
    file = fopen(descriptor, "w");
    assert(file && fprintf(file, "path = \"%s\"\n", mod) > 0 && fclose(file) == 0);
    struct content_entry entry = {.name = "toolbar test", .source = descriptor,
                                  .enabled = true, .installed = true};
    const struct content_catalog catalog = {.mods = {&entry, 1}};
    ck2_mount_content(root, &catalog);
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) == 0);
    assert(setup_toolbar_hit(&toolbar, (SDL_Point){10, 13}) == SETUP_TERRAIN);
    assert(toolbar.widgets[SETUP_TERRAIN].area.w == 8);
    ck2_unmount_content();

    gui(layout, -100, "Unknown", "CENTER_UP");
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) < 0);
    assert(strstr(SDL_GetError(), "missing sprite") != NULL);
    assert(setup_toolbar_hit(&toolbar, (SDL_Point){10, 13}) == SETUP_TERRAIN);
    gui(layout, -100, "Art", "OTHER");
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) < 0);
    assert(strstr(SDL_GetError(), "position/orientation") != NULL);
    gui(layout, -100, "Art", "CENTER_UP");
    file = fopen(tga, "w");
    assert(file && fclose(file) == 0);
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) < 0);
    assert(unlink(tga) == 0 && unlink(image) == 0);
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) < 0);
    assert(strstr(SDL_GetError(), "could not load") != NULL);
    file = fopen(layout, "w");
    assert(file && fputs("guiTypes = { windowType = { name = top_window position = { x = invalid", file)
           >= 0 && fclose(file) == 0);
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) < 0);
    assert(strstr(SDL_GetError(), "unbalanced") != NULL);
    assert(unlink(layout) == 0);
    assert(setup_toolbar_load(&toolbar, renderer, root, 640) < 0);
    assert(strstr(SDL_GetError(), "Could not read interface/frontend.gui") != NULL);
    assert(unlink(art) == 0 && unlink(mod_gui) == 0 && unlink(mod_gfx) == 0 && unlink(descriptor) == 0);
    assert(rmdir(mod_interface) == 0 && rmdir(mod) == 0 && rmdir(directory) == 0 && rmdir(root) == 0);

done:
    setup_toolbar_free(&toolbar);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(screen);
    return EXIT_SUCCESS;
}
