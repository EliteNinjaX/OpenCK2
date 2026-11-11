#define _POSIX_C_SOURCE 200809L
#include "catalog.h"
#include "install.h"

#include <dirent.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>

/* Expansion sprite mapping from dlc_metadata/dlc_info.txt and launcher/main.gfx. */
#define DLC_ICON(id, name) {"dlc" id ".dlc", "gfx/interface/dlc_icon_" name ".dds"}
static const struct {
    const char *descriptor, *path;
} icons[CK2_DLC_ICON_COUNT] = {
    DLC_ICON("007", "sword_of_islam"), DLC_ICON("011", "legacy_of_rome"),
    DLC_ICON("018", "sunset_invasion"), DLC_ICON("022", "the_republic"),
    DLC_ICON("024", "the_old_gods"), DLC_ICON("032", "sons_of_abraham"),
    DLC_ICON("039", "rajas_of_india"), DLC_ICON("045", "charlemagne"),
    DLC_ICON("050", "way_of_life"), DLC_ICON("054", "horse_lords"),
    DLC_ICON("062", "zeus"), DLC_ICON("066", "reapers_due"),
    DLC_ICON("069", "monks_and_mystics"), DLC_ICON("073", "jade_dragon"),
    DLC_ICON("074", "holy_fury"),
};
#undef DLC_ICON

const char *ck2_catalog_icon_path(int icon)
{
    return icons[icon].path;
}

static int icon_for(const char *descriptor)
{
    for (int i = 0; i < CK2_DLC_ICON_COUNT; ++i) {
        if (strcasecmp(icons[i].descriptor, descriptor) == 0)
            return i;
    }
    return -1;
}

/* The installed atlas is Windows-1252. Older descriptors already use that encoding. */
static char *name_for_atlas(char *name)
{
    if (!name)
        return NULL;
    const size_t size = strlen(name);
    if (strcspn(name, "\xc2\xc3\xe2") == size)
        return name;
    iconv_t converter = iconv_open("WINDOWS-1252", "UTF-8");
    if (converter == (iconv_t)-1)
        return name;

    char *result = malloc(size + 1);
    bool converted = false;
    if (result) {
        char *input = name, *output = result;
        size_t remaining = size, space = size;
        converted = iconv(converter, &input, &remaining, &output, &space) != (size_t)-1;
        if (converted)
            *output = '\0';
    }
    iconv_close(converter);
    if (converted) {
        free(name);
        return result;
    }
    free(result);
    return name;
}

static char *descriptor_name(const char *path)
{
    struct stat info;
    if (stat(path, &info) != 0 || !S_ISREG(info.st_mode))
        return NULL;
    FILE *file = fopen(path, "r");
    if (!file)
        return NULL;

    char *line = NULL, *name = NULL;
    size_t capacity = 0;
    while (getline(&line, &capacity, file) >= 0) {
        char *key = line;
        if (strncmp(key, "\xef\xbb\xbf", 3) == 0)
            key += 3;
        key += strspn(key, " \t");
        if (strncmp(key, "name", 4) != 0)
            continue;
        char *value = key + 4;
        value += strspn(value, " \t");
        if (*value != '=')
            continue;
        ++value;
        value += strspn(value, " \t");
        const bool quoted = *value == '"';
        value += quoted;
        size_t length = strcspn(value, quoted ? "\"\r\n" : "#\r\n");
        while (length && (value[length - 1] == ' ' || value[length - 1] == '\t'))
            --length;
        if (length)
            name = strndup(value, length);
        break;
    }
    free(line);
    fclose(file);
    return name_for_atlas(name);
}

static void collect(struct content_list *list, const char *directory, const char *extension)
{
    DIR *folder = directory ? opendir(directory) : NULL;
    if (!folder)
        return;

    const size_t ext_length = strlen(extension);
    struct dirent *entry;
    while ((entry = readdir(folder))) {
        const size_t length = strlen(entry->d_name);
        if (length <= ext_length || strcasecmp(entry->d_name + length - ext_length, extension) != 0)
            continue;

        bool duplicate = false;
        for (size_t i = 0; i < list->count; ++i) {
            if (strcasecmp(list->entries[i].descriptor, entry->d_name) == 0) {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;

        char *path = ck2_join_path(directory, entry->d_name);
        char *name = path ? descriptor_name(path) : NULL;
        free(path);
        if (!name)
            continue;
        char *descriptor = strdup(entry->d_name);
        struct content_entry *entries = descriptor
            ? realloc(list->entries, (list->count + 1) * sizeof *entries) : NULL;
        if (!entries) {
            free(name);
            free(descriptor);
            break;
        }
        list->entries = entries;
        const bool dlc = strcmp(extension, ".dlc") == 0;
        list->entries[list->count++] = (struct content_entry){
            .name = name, .descriptor = descriptor,
            .icon = dlc ? icon_for(descriptor) : -1,
            .category = dlc && icon_for(descriptor) >= 0 ? EXPANSION : OTHER_DLC,
            .enabled = dlc, .installed = true,
        };
    }
    closedir(folder);
}

static int dlc_order(const void *left, const void *right)
{
    const struct content_entry *a = left, *b = right;
    if (a->category != b->category)
        return a->category < b->category ? -1 : 1;
    if (a->installed != b->installed)
        return a->installed ? -1 : 1;
    if (!a->installed && a->steam_id != b->steam_id)
        return a->steam_id > b->steam_id ? -1 : 1;
    return strcasecmp(b->descriptor, a->descriptor);
}

static int mod_order(const void *left, const void *right)
{
    const struct content_entry *a = left, *b = right;
    return strcasecmp(a->name, b->name);
}

static void collect_from(struct content_list *list, const char *root, const char *relative,
                         const char *extension)
{
    char *path = ck2_join_path(root, relative);
    collect(list, path, extension);
    free(path);
}

static const struct {
    const char *key, *label;
} categories[DLC_CATEGORY_COUNT] = {
    [EXPANSION] = {"expansion", "Expansions"},
    [CONTENT_PACK] = {"content", "Content Packs"},
    [UNIT_PACK] = {"unit", "Unit Packs"},
    [PORTRAIT_PACK] = {"portrait", "Portrait Packs"},
    [DYNASTY_SHIELD] = {"dynasty_shield", "Dynasty Shields"},
    [MUSIC_PACK] = {"music", "Music Packs"},
    [OTHER_DLC] = {"other", "Other DLC"},
};

const char *ck2_category_label(enum dlc_category category)
{
    return categories[category].label;
}

static enum dlc_category category_for(const char *key)
{
    for (int i = 0; i < DLC_CATEGORY_COUNT; ++i) {
        if (strcmp(key, categories[i].key) == 0)
            return (enum dlc_category)i;
    }
    return OTHER_DLC;
}

static char *field(char *line, const char *key)
{
    line += strspn(line, " \t");
    const size_t length = strlen(key);
    if (strncmp(line, key, length) != 0)
        return NULL;
    line += length;
    line += strspn(line, " \t");
    if (*line++ != '=')
        return NULL;
    line += strspn(line, " \t");
    const bool quoted = *line == '"';
    line += quoted;
    return strndup(line, strcspn(line, quoted ? "\"\r\n" : " \t\r\n#"));
}

static int brace_delta(const char *line)
{
    bool quoted = false;
    int delta = 0;
    for (; *line; ++line) {
        if (*line == '\\' && quoted && line[1]) {
            ++line;
        } else if (*line == '"') {
            quoted = !quoted;
        } else if (!quoted) {
            if (*line == '#')
                break;
            delta += (*line == '{') - (*line == '}');
        }
    }
    return delta;
}

static struct content_entry *find_entry(struct content_list *list, const char *descriptor)
{
    for (size_t i = 0; i < list->count; ++i) {
        if (strcasecmp(list->entries[i].descriptor, descriptor) == 0)
            return &list->entries[i];
    }
    return NULL;
}

static bool steam_depot_present(const char *manifest, unsigned steam_id)
{
    FILE *file = manifest ? fopen(manifest, "r") : NULL;
    if (!file)
        return false;
    char *line = NULL;
    size_t capacity = 0;
    unsigned id;
    bool found = false;
    while (getline(&line, &capacity, file) >= 0) {
        if (sscanf(line, " \"dlcappid\" \"%u\"", &id) == 1 && id == steam_id) {
            found = true;
            break;
        }
    }
    free(line);
    fclose(file);
    return found;
}

static void apply_metadata(struct content_list *list, struct content_entry *record,
                           const char *hide_if_owned, bool hidden, const char *manifest)
{
    if (!record->descriptor)
        return;
    struct content_entry *installed = find_entry(list, record->descriptor);
    if (installed) {
        installed->category = record->category;
        installed->steam_id = record->steam_id;
        return;
    }
    if (hidden || !record->name || !record->steam_id ||
        (hide_if_owned && find_entry(list, hide_if_owned)) ||
        steam_depot_present(manifest, record->steam_id))
        return;

    struct content_entry *entries = realloc(list->entries, (list->count + 1) * sizeof *entries);
    if (!entries)
        return;
    list->entries = entries;
    record->name = name_for_atlas(record->name);
    record->icon = icon_for(record->descriptor);
    list->entries[list->count++] = *record;
    record->name = record->descriptor = NULL;
}

static void load_dlc_metadata(struct content_list *list, const char *directory)
{
    char *path = ck2_join_path(directory, "dlc_metadata/dlc_info.txt");
    FILE *file = path ? fopen(path, "r") : NULL;
    free(path);
    if (!file)
        return;

    char *manifest = ck2_join_path(directory, "../../appmanifest_203770.acf");
    char *line = NULL, *hide_if_owned = NULL;
    size_t capacity = 0;
    int depth = 0;
    bool in_catalog = false, hidden = false;
    struct content_entry record = {.category = OTHER_DLC};
    while (getline(&line, &capacity, file) >= 0) {
        char *start = line + strspn(line, " \t");
        const int delta = brace_delta(start);
        if (!in_catalog && depth == 0 && strncmp(start, "dlcs", 4) == 0 && delta == 1)
            in_catalog = true;
        else if (in_catalog && depth == 1 && delta == 1) {
            const size_t length = strcspn(start, " \t=");
            if (length > 0 && length < 200) {
                char descriptor[204];
                snprintf(descriptor, sizeof descriptor, "%.*s.dlc", (int)length, start);
                record.descriptor = strdup(descriptor);
            }
        } else if (in_catalog && depth == 2 && record.descriptor) {
            char *value;
            if ((value = field(start, "name"))) {
                free(record.name);
                record.name = value;
            } else if ((value = field(start, "steam_id"))) {
                char *end;
                unsigned long id = strtoul(value, &end, 10);
                if (*value && !*end && id > 0 && id <= 4294967295UL)
                    record.steam_id = (unsigned)id;
                free(value);
            } else if ((value = field(start, "category"))) {
                record.category = category_for(value);
                free(value);
            } else if ((value = field(start, "hide_if_unowned"))) {
                hidden = strcmp(value, "yes") == 0;
                free(value);
            } else if ((value = field(start, "hide_if_dlc_owned"))) {
                free(hide_if_owned);
                const size_t length = strlen(value) + sizeof ".dlc";
                hide_if_owned = malloc(length);
                if (hide_if_owned)
                    snprintf(hide_if_owned, length, "%s.dlc", value);
                free(value);
            }
        }
        depth += delta;
        if (in_catalog && depth == 1 && delta < 0 && record.descriptor) {
            apply_metadata(list, &record, hide_if_owned, hidden, manifest);
            free(record.name);
            free(record.descriptor);
            free(hide_if_owned);
            record = (struct content_entry){.category = OTHER_DLC};
            hide_if_owned = NULL;
            hidden = false;
        }
        if (in_catalog && depth == 0)
            break;
    }
    free(record.name);
    free(record.descriptor);
    free(hide_if_owned);
    free(manifest);
    free(line);
    fclose(file);
}

void ck2_catalog_load(struct content_catalog *catalog, const char *directory)
{
    const char proton[] = "compatdata/203770/pfx/drive_c/users/steamuser/Documents/Paradox Interactive/Crusader Kings II/mod";
    char *steamapps = ck2_join_path(directory, "../..");
    if (steamapps)
        collect_from(&catalog->mods, steamapps, proton, ".mod");
    free(steamapps);

    const char *home = getenv("HOME");
    if (home && *home) {
        collect_from(&catalog->mods, home, ".paradoxinteractive/Crusader Kings II/mod", ".mod");
        char *default_library = ck2_join_path(home, ".steam/steam/steamapps");
        if (default_library)
            collect_from(&catalog->mods, default_library, proton, ".mod");
        free(default_library);
    }
    collect_from(&catalog->mods, directory, "mod", ".mod");
    collect_from(&catalog->dlc, directory, "dlc", ".dlc");
    load_dlc_metadata(&catalog->dlc, directory);

    if (catalog->dlc.count > 1)
        qsort(catalog->dlc.entries, catalog->dlc.count, sizeof *catalog->dlc.entries, dlc_order);
    if (catalog->mods.count > 1)
        qsort(catalog->mods.entries, catalog->mods.count, sizeof *catalog->mods.entries, mod_order);
}

void ck2_catalog_restore(struct content_catalog *catalog, const char *path)
{
    FILE *file = path ? fopen(path, "r") : NULL;
    if (!file)
        return;
    char *line = NULL;
    size_t capacity = 0;
    while (getline(&line, &capacity, file) >= 0) {
        if ((line[0] != 'D' && line[0] != 'M') || line[1] != '\t')
            continue;
        char *descriptor = line + 2;
        descriptor[strcspn(descriptor, "\r\n")] = '\0';
        struct content_list *list = line[0] == 'D' ? &catalog->dlc : &catalog->mods;
        for (size_t i = 0; i < list->count; ++i) {
            if (list->entries[i].installed && strcmp(descriptor, list->entries[i].descriptor) == 0) {
                list->entries[i].enabled = line[0] == 'M';
                break;
            }
        }
    }
    free(line);
    fclose(file);
}

static bool write_changes(FILE *file, const struct content_list *list, bool dlc)
{
    for (size_t i = 0; i < list->count; ++i) {
        const struct content_entry *entry = &list->entries[i];
        if (entry->installed && entry->enabled != dlc && fprintf(file, "%c\t%s\n", dlc ? 'D' : 'M', entry->descriptor) < 0)
            return false;
    }
    return true;
}

bool ck2_catalog_save(const struct content_catalog *catalog, const char *path)
{
    if (!path)
        return false;
    const size_t length = strlen(path) + sizeof ".XXXXXX";
    char *temporary = malloc(length);
    if (!temporary)
        return false;
    snprintf(temporary, length, "%s.XXXXXX", path);
    int descriptor = mkstemp(temporary);
    if (descriptor < 0) {
        free(temporary);
        return false;
    }
    FILE *file = fdopen(descriptor, "w");
    bool saved = false;
    if (file) {
        bool written = write_changes(file, &catalog->dlc, true) &&
                       write_changes(file, &catalog->mods, false);
        saved = fclose(file) == 0 && written && rename(temporary, path) == 0;
    } else {
        close(descriptor);
    }
    if (!saved)
        unlink(temporary);
    free(temporary);
    return saved;
}

void ck2_catalog_set_all(struct content_catalog *catalog, bool enabled)
{
    struct content_list *lists[] = {&catalog->dlc, &catalog->mods};
    for (size_t j = 0; j < 2; ++j) {
        for (size_t i = 0; i < lists[j]->count; ++i)
            if (lists[j]->entries[i].installed)
                lists[j]->entries[i].enabled = enabled;
    }
}

static bool has_state(const struct content_catalog *catalog, bool enabled)
{
    const struct content_list *lists[] = {&catalog->dlc, &catalog->mods};
    for (size_t j = 0; j < 2; ++j) {
        for (size_t i = 0; i < lists[j]->count; ++i) {
            if (lists[j]->entries[i].installed && lists[j]->entries[i].enabled == enabled)
                return true;
        }
    }
    return false;
}

bool ck2_catalog_all_enabled(const struct content_catalog *catalog)
{
    return has_state(catalog, true) && !has_state(catalog, false);
}

bool ck2_catalog_any_enabled(const struct content_catalog *catalog)
{
    return has_state(catalog, true);
}

void ck2_catalog_free(struct content_catalog *catalog)
{
    struct content_list *lists[] = {&catalog->dlc, &catalog->mods};
    for (size_t kind = 0; kind < sizeof lists / sizeof *lists; ++kind) {
        struct content_list *list = lists[kind];
        for (size_t i = 0; i < list->count; ++i) {
            free(list->entries[i].name);
            free(list->entries[i].descriptor);
        }
        free(list->entries);
    }
    *catalog = (struct content_catalog){0};
}
