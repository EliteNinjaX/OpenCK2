#include "music_playlist_core.h"
#include <string.h>

static bool ends_with(const char *text, const char *suffix)
{
    const size_t n = strlen(text), m = strlen(suffix);
    if (n < m) return false;
    for (size_t i = 0; i < m; ++i) {
        char a = text[n - m + i], b = suffix[i];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}

bool music_playlist_is_track(const char *name)
{
    if (!name || !*name || name[0] == '.' || strstr(name, "..") ||
        strchr(name, '/') || strchr(name, '\\')) return false;
    return ends_with(name, ".ogg") || ends_with(name, ".mp3") || ends_with(name, ".flac");
}

size_t music_playlist_next(size_t count, size_t current)
{
    if (!count) return 0;
    return current >= count ? 0 : (current + 1) % count;
}
