#ifndef OPEN_CK2_INSTALL_H
#define OPEN_CK2_INSTALL_H

#include <stdbool.h>

bool ck2_install_valid(const char *directory);
char *ck2_find_steam_install(void);
char *ck2_read_install(const char *settings);
bool ck2_write_install(const char *settings, const char *directory);
char *ck2_join_path(const char *directory, const char *name);

#endif
