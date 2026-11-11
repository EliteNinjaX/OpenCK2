#ifndef OPEN_CK2_CATALOG_H
#define OPEN_CK2_CATALOG_H

#include <stddef.h>
#include <stdbool.h>

enum { CK2_DLC_ICON_COUNT = 15 };

enum dlc_category { EXPANSION, CONTENT_PACK, UNIT_PACK, PORTRAIT_PACK,
                    DYNASTY_SHIELD, MUSIC_PACK, OTHER_DLC, DLC_CATEGORY_COUNT };

struct content_entry {
    char *name, *descriptor, *source;
    int icon;
    enum dlc_category category;
    unsigned steam_id;
    bool enabled, installed;
};
struct content_list {
    struct content_entry *entries;
    size_t count;
};
struct content_catalog {
    struct content_list dlc, mods;
};

void ck2_catalog_load(struct content_catalog *catalog, const char *directory);
const char *ck2_catalog_icon_path(int icon);
const char *ck2_category_label(enum dlc_category category);
void ck2_catalog_restore(struct content_catalog *catalog, const char *path);
bool ck2_catalog_save(const struct content_catalog *catalog, const char *path);
void ck2_content_set_all(struct content_list *list, bool enabled);
bool ck2_content_all_enabled(const struct content_list *list);
bool ck2_content_any_enabled(const struct content_list *list);
void ck2_catalog_free(struct content_catalog *catalog);

#endif
