#ifndef OPENCK2_ZIP_ASSETS_H
#define OPENCK2_ZIP_ASSETS_H

#include <stddef.h>
struct ck2_zip;
struct ck2_zip *ck2_zip_open(const char *path);
void ck2_zip_close(struct ck2_zip *);
size_t ck2_zip_count(const struct ck2_zip *);
const char *ck2_zip_name(const struct ck2_zip *, size_t index);
char *ck2_zip_asset(struct ck2_zip *, const char *name);

#endif
