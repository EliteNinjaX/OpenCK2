#ifndef OPENCK2_LOCALIZATION_CORE_H
#define OPENCK2_LOCALIZATION_CORE_H
#include <stddef.h>
#include <stdbool.h>

enum ck2_language {CK2_ENGLISH, CK2_FRENCH, CK2_GERMAN, CK2_SPANISH, CK2_LANGUAGE_COUNT};
struct loc_entry {char *key, *value;};
struct loc_catalog {struct loc_entry *slots; size_t capacity, count; enum ck2_language language;};
void loc_catalog_init(struct loc_catalog *, enum ck2_language);
void loc_catalog_free(struct loc_catalog *);
bool loc_catalog_add_line(struct loc_catalog *, const char *line, size_t length);
const char *loc_catalog_find(const struct loc_catalog *, const char *key);
const char *loc_language_name(enum ck2_language);
#endif
