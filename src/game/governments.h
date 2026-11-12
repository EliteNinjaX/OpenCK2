#ifndef OPENCK2_GOVERNMENTS_H
#define OPENCK2_GOVERNMENTS_H
#include "government_core.h"
/* Loads the original base/DLC/mod government definitions from the mounted
 * CK2 content tree. Does not execute scripted potential or assign rulers. */
int governments_load(struct government_catalog *catalog, const char *root);
#endif
