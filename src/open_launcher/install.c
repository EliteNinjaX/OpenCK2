#define _XOPEN_SOURCE 700
#include "install.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *ck2_join_path(const char *directory, const char *name)
{
    size_t length = strlen(directory) + strlen(name) + 2;
    char *path = malloc(length);
    if (path)
        snprintf(path, length, "%s/%s", directory, name);
    return path;
}

bool ck2_install_valid(const char *directory)
{
    static const char *const required[] = {
        "launcher/gfx/interface/tab_button.tga",
        "launcher/gfx/interface/launcher_dlc_entry_bg.dds",
        "launcher/gfx/interface/launcher_dlc_entry_category_bg.dds",
        "launcher/gfx/interface/launcher_dlc_entry_sub_bg.dds",
        "launcher/gfx/interface/launcher_dlc_entry_sub_unowned_bg.dds",
        "launcher/gfx/interface/dlc_category_button.dds",
        "launcher/gfx/interface/store_button.dds",
        "launcher/gfx/interface/launcher_dlc_mod_bg.dds",
        "map/terrain/colormap.dds",
        "gfx/interface/big_button_300.dds",
        "gfx/interface/checkbox_default.dds", "gfx/interface/scroll.dds",
        "gfx/interface/scroll_bg.dds", "gfx/interface/scroll_up.dds",
        "gfx/interface/scroll_down.dds",
        "gfx/fonts/vic_22.fnt", "gfx/fonts/vic_22.tga",
        "gfx/interface/frontend_title.dds", "gfx/interface/pdx_dev_logo.dds",
        "gfx/interface/frontend_center_panel_top.dds",
        "gfx/interface/frontend_center_panel_bottom1.dds",
        "gfx/interface/big_button_232.dds", "gfx/interface/creditscreen_bg.dds",
        "interface/credits.txt"
    };
    if (!directory || directory[0] != '/' || strpbrk(directory, "\r\n"))
        return false;
    for (size_t i = 0; i < sizeof required / sizeof *required; ++i) {
        char *file = ck2_join_path(directory, required[i]);
        struct stat info;
        bool valid = file && stat(file, &info) == 0 && S_ISREG(info.st_mode)
            && access(file, R_OK) == 0;
        free(file);
        if (!valid)
            return false;
    }
    return true;
}

/* Valve writes these string fields as quoted key/value pairs on one line. */
static char *quoted_value(const char *line, const char *key)
{
    line += strspn(line, " \t");
    const size_t length = strlen(key);
    if (*line++ != '"' || strncmp(line, key, length) != 0 || line[length] != '"')
        return NULL;
    line += length + 1;
    line += strspn(line, " \t");
    if (*line++ != '"')
        return NULL;

    char *value = malloc(strlen(line) + 1);
    if (!value)
        return NULL;
    char *out = value;
    while (*line && *line != '"') {
        if (*line == '\\' && (line[1] == '\\' || line[1] == '"'))
            ++line;
        *out++ = *line++;
    }
    *out = '\0';
    if (*line == '"')
        return value;
    free(value);
    return NULL;
}

static char *steam_library_install(const char *library)
{
    char *manifest = ck2_join_path(library, "steamapps/appmanifest_203770.acf");
    FILE *file = manifest ? fopen(manifest, "r") : NULL;
    free(manifest);
    char *name = NULL, *line = NULL;
    size_t capacity = 0;
    if (file) {
        while (!name && getline(&line, &capacity, file) >= 0)
            name = quoted_value(line, "installdir");
        fclose(file);
    }
    free(line);
    if (name && (!*name || strpbrk(name, "/\\") || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)) {
        free(name);
        name = NULL;
    }
    char *common = ck2_join_path(library, "steamapps/common");
    char *directory = common ? ck2_join_path(common, name ? name : "Crusader Kings II") : NULL;
    free(name);
    free(common);
    if (ck2_install_valid(directory)) {
        char *resolved = realpath(directory, NULL);
        if (!resolved)
            return directory;
        free(directory);
        return resolved;
    }
    free(directory);
    return NULL;
}

static char *find_in_steam_root(const char *steam_root)
{
    char *directory = steam_library_install(steam_root);
    if (directory)
        return directory;

    char *folders = ck2_join_path(steam_root, "steamapps/libraryfolders.vdf");
    FILE *file = folders ? fopen(folders, "r") : NULL;
    free(folders);
    if (!file)
        return NULL;

    char *line = NULL;
    size_t capacity = 0;
    while (!directory && getline(&line, &capacity, file) >= 0) {
        char *library = quoted_value(line, "path");
        if (library)
            directory = steam_library_install(library);
        free(library);
    }
    free(line);
    fclose(file);
    return directory;
}

char *ck2_find_steam_install(void)
{
    const char *data_home = getenv("XDG_DATA_HOME");
    if (data_home && *data_home) {
        char *steam = ck2_join_path(data_home, "Steam");
        char *directory = steam ? find_in_steam_root(steam) : NULL;
        free(steam);
        if (directory)
            return directory;
    }

    const char *home = getenv("HOME");
    if (!home || !*home)
        return NULL;
    static const char *const roots[] = {
        ".steam/steam", ".steam/root", ".local/share/Steam",
        ".var/app/com.valvesoftware.Steam/.local/share/Steam"
    };
    for (size_t i = 0; i < sizeof roots / sizeof *roots; ++i) {
        char *steam = ck2_join_path(home, roots[i]);
        char *directory = steam ? find_in_steam_root(steam) : NULL;
        free(steam);
        if (directory)
            return directory;
    }
    return NULL;
}

char *ck2_read_install(const char *settings)
{
    FILE *file = settings ? fopen(settings, "r") : NULL;
    if (!file)
        return NULL;
    char *directory = NULL;
    size_t capacity = 0;
    ssize_t size = getline(&directory, &capacity, file);
    fclose(file);
    if (size >= 0) {
        directory[strcspn(directory, "\r\n")] = '\0';
        if (ck2_install_valid(directory))
            return directory;
    }
    free(directory);
    return NULL;
}

bool ck2_write_install(const char *settings, const char *directory)
{
    if (!settings)
        return false;
    const size_t length = strlen(settings) + sizeof ".XXXXXX";
    char *temporary = malloc(length);
    if (!temporary)
        return false;
    snprintf(temporary, length, "%s.XXXXXX", settings);
    int descriptor = mkstemp(temporary);
    if (descriptor < 0) {
        free(temporary);
        return false;
    }
    FILE *file = fdopen(descriptor, "w");
    bool saved = false;
    if (file) {
        const bool written = fprintf(file, "%s\n", directory) >= 0;
        saved = fclose(file) == 0 && written && rename(temporary, settings) == 0;
    } else {
        close(descriptor);
    }
    if (!saved)
        unlink(temporary);
    free(temporary);
    return saved;
}
