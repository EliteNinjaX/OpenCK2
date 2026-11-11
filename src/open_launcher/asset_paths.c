#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 700
#include "asset_paths.h"
#include "install.h"
#include "zip_assets.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct mount {
    char *directory;
    char **replace;
    size_t count;
    struct ck2_zip *archive;
};

static struct mount *mounts;
static size_t mount_count;

static bool under(const char *path, const char *prefix)
{
    const size_t n = strlen(prefix);
    return !strncmp(path, prefix, n) && (!path[n] || path[n] == '/');
}

static bool blocked(const struct mount *mount, const char *path)
{
    for (size_t i = 0; i < mount->count; ++i)
        if (under(path, mount->replace[i]))
            return true;
    return false;
}

static void free_mount(struct mount *mount)
{
    ck2_zip_close(mount->archive);
    free(mount->directory);
    for (size_t i = 0; i < mount->count; ++i)
        free(mount->replace[i]);
    free(mount->replace);
}

void ck2_unmount_content(void)
{
    for (size_t i = 0; i < mount_count; ++i)
        free_mount(&mounts[i]);
    free(mounts);
    mounts = NULL;
    mount_count = 0;
}

static char *value_for(char *line, const char *key)
{
    if (!strncmp(line, "\xef\xbb\xbf", 3))
        line += 3;
    line += strspn(line, " \t");
    const size_t len = strlen(key);
    if (strncmp(line, key, len))
        return NULL;
    line += len;
    line += strspn(line, " \t");
    if (*line++ != '=')
        return NULL;
    line += strspn(line, " \t");
    const bool quoted = *line == '"';
    line += quoted;
    size_t n = strcspn(line, quoted ? "\"\r\n" : "#\r\n \t");
    while (n && line[n - 1] == '/')
        --n;
    return n ? strndup(line, n) : NULL;
}

static char *locate_payload(const char *root, const char *source, const char *value, bool archive)
{
    if (!value)
        return NULL;
    char *descriptor_dir = strdup(source);
    if (!descriptor_dir)
        return NULL;
    char *slash = strrchr(descriptor_dir, '/');
    if (slash)
        *slash = 0;
    const char *local = !strncmp(value, "mod/", 4) ? value + 4 : value;
    char *candidates[] = {
        value[0] == '/' ? strdup(value) : ck2_join_path(descriptor_dir, local),
        value[0] == '/' ? NULL : ck2_join_path(root, value),
    };
    char *result = NULL;
    for (size_t i = 0; i < sizeof candidates / sizeof *candidates; ++i) {
        struct stat info;
        if (!result && candidates[i] && stat(candidates[i], &info) == 0 &&
            (archive ? S_ISREG(info.st_mode) : S_ISDIR(info.st_mode)))
            result = realpath(candidates[i], NULL);
        free(candidates[i]);
    }
    free(descriptor_dir);
    return result;
}

static void mount_entry(const char *root, const struct content_entry *entry, const char *kind)
{
    if (!entry->installed || !entry->enabled || !entry->source)
        return;
    struct mount mount = {0};
    char *path = NULL, *archive = NULL, *line = NULL;
    size_t capacity = 0;
    FILE *file = fopen(entry->source, "r");
    if (file) {
        while (getline(&line, &capacity, file) >= 0) {
            if (!path)
                path = value_for(line, "path");
            if (!archive)
                archive = value_for(line, "archive");
            char *replace = value_for(line, "replace_path");
            if (replace) {
                char **next = realloc(mount.replace, (mount.count + 1) * sizeof *next);
                if (next) {
                    mount.replace = next;
                    mount.replace[mount.count++] = replace;
                } else {
                    free(replace);
                }
            }
        }
        fclose(file);
    }
    free(line);
    mount.directory = locate_payload(root, entry->source, path, false);
    if (!mount.directory && archive) {
        char *payload = locate_payload(root, entry->source, archive, true);
        if (payload) {
            mount.archive = ck2_zip_open(payload);
            free(payload);
        }
    }
    free(path);
    free(archive);
    if (mount.directory || mount.archive) {
        mounts[mount_count++] = mount;
        fprintf(stderr, "Mounted %s: %s\n", kind, entry->name);
    } else {
        fprintf(stderr, "Could not mount %s: %s\n", kind, entry->source);
        free_mount(&mount);
    }
}

static int dlc_order(const void *a, const void *b)
{
    const struct content_entry *const *left = a, *const *right = b;
    return strcmp((*left)->descriptor, (*right)->descriptor);
}

void ck2_mount_content(const char *root, const struct content_catalog *catalog)
{
    ck2_unmount_content();
    const size_t count = catalog->dlc.count + catalog->mods.count;
    if (!count)
        return;
    mounts = calloc(count, sizeof *mounts);
    if (!mounts)
        return;
    const struct content_entry **dlc = malloc(catalog->dlc.count * sizeof *dlc);
    if (dlc) {
        for (size_t i = 0; i < catalog->dlc.count; ++i)
            dlc[i] = &catalog->dlc.entries[i];
        qsort(dlc, catalog->dlc.count, sizeof *dlc, dlc_order);
        for (size_t i = 0; i < catalog->dlc.count; ++i)
            mount_entry(root, dlc[i], "DLC");
        free(dlc);
    }
    for (size_t i = 0; i < catalog->mods.count; ++i)
        mount_entry(root, &catalog->mods.entries[i], "mod");
}

static char *within_mount(const struct mount *mount, const char *relative, bool directory)
{
    if (mount->archive)
        return directory ? NULL : ck2_zip_asset(mount->archive, relative);
    char *path = ck2_join_path(mount->directory, relative);
    char *resolved = path ? realpath(path, NULL) : NULL;
    free(path);
    struct stat info;
    if (resolved && under(resolved, mount->directory) && stat(resolved, &info) == 0 &&
        (directory ? S_ISDIR(info.st_mode) : S_ISREG(info.st_mode)))
        return resolved;
    free(resolved);
    return NULL;
}

char *ck2_asset_path_with_priority(const char *root, const char *relative, size_t *priority)
{
    if (priority)
        *priority = 0;
    for (size_t i = mount_count; i > 0; --i) {
        char *path = within_mount(&mounts[i - 1], relative, false);
        if (path) {
            if (priority)
                *priority = i;
            return path;
        }
        if (blocked(&mounts[i - 1], relative))
            return NULL;
    }
    return ck2_join_path(root, relative);
}

char *ck2_asset_path(const char *root, const char *relative)
{
    return ck2_asset_path_with_priority(root, relative, NULL);
}

static void list_add(struct ck2_asset_files *files, const char *entry)
{
    if (entry[0] == '.')
        return;
    size_t i = 0;
    while (i < files->count && strcmp(files->names[i], entry))
        ++i;
    if (i < files->count) {
        char *name = files->names[i];
        memmove(files->names + i, files->names + i + 1,
                (files->count - i - 1) * sizeof *files->names);
        files->names[files->count - 1] = name;
    } else {
        char *name = strdup(entry);
        char **next = name ? realloc(files->names, (files->count + 1) * sizeof *next) : NULL;
        if (next) {
            files->names = next;
            files->names[files->count++] = name;
        } else {
            free(name);
        }
    }
}

static void list_directory(struct ck2_asset_files *files, const char *path)
{
    struct dirent **entries = NULL;
    const int count = path ? scandir(path, &entries, NULL, alphasort) : -1;
    for (int index = 0; index < count; ++index) {
        const char *entry = entries[index]->d_name;
        list_add(files, entry);
        free(entries[index]);
    }
    free(entries);
}

struct ck2_asset_files ck2_list_assets(const char *root, const char *relative)
{
    struct ck2_asset_files files = {0};
    size_t first = 0;
    for (size_t i = mount_count; i > 0; --i) {
        if (blocked(&mounts[i - 1], relative)) {
            first = i;
            break;
        }
    }
    if (!first) {
        char *directory = ck2_join_path(root, relative);
        list_directory(&files, directory);
        free(directory);
    }
    for (size_t i = first ? first - 1 : 0; i < mount_count; ++i) {
        if (mounts[i].archive) {
            const struct ck2_zip *zip = mounts[i].archive;
            const size_t length = strlen(relative);
            for (size_t j = 0; j < ck2_zip_count(zip); ++j) {
                const char *name = ck2_zip_name(zip, j);
                if (!strncmp(name, relative, length) && name[length] == '/' &&
                    !strchr(name + length + 1, '/'))
                    list_add(&files, name + length + 1);
            }
        } else {
            char *directory = within_mount(&mounts[i], relative, true);
            list_directory(&files, directory);
            free(directory);
        }
    }
    return files;
}

void ck2_free_asset_files(struct ck2_asset_files *files)
{
    for (size_t i = 0; i < files->count; ++i)
        free(files->names[i]);
    free(files->names);
    *files = (struct ck2_asset_files){0};
}
