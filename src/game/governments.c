#include "governments.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <SDL.h>
#include <string.h>

int governments_load(struct government_catalog *catalog, const char *root)
{
    if (!catalog || !root) return SDL_SetError("Government loader: invalid arguments");
    struct ck2_asset_files files = ck2_list_assets(root, "common/governments");
    size_t loaded = 0;
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t n = strlen(name);
        if (n < 4 || strcmp(name + n - 4, ".txt")) continue;
        char *source = load_from_directory(root, "common/governments", name);
        if (!source) continue;
        const bool success = government_catalog_parse(catalog, source);
        SDL_free(source);
        if (!success) {
            ck2_free_asset_files(&files);
            return SDL_SetError("Malformed or oversized government definitions: %s", name);
        }
        ++loaded;
    }
    ck2_free_asset_files(&files);
    return loaded ? 0 : SDL_SetError("No CK2 government definition .txt files found");
}
