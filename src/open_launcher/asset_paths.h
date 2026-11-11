#ifndef OPENCK2_ASSET_PATHS_H
#define OPENCK2_ASSET_PATHS_H

#include "catalog.h"

void ck2_mount_content(const char *root, const struct content_catalog *catalog);
void ck2_unmount_content(void);
char *ck2_asset_path(const char *root, const char *relative);
char *ck2_asset_path_with_priority(const char *root, const char *relative, size_t *priority);

struct ck2_asset_files {
    char **names;
    size_t count;
};
struct ck2_asset_files ck2_list_assets(const char *root, const char *relative);
void ck2_free_asset_files(struct ck2_asset_files *);

#endif
