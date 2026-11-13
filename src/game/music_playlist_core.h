#ifndef OPENCK2_MUSIC_PLAYLIST_CORE_H
#define OPENCK2_MUSIC_PLAYLIST_CORE_H
#include <stdbool.h>
#include <stddef.h>

/* Only local, supported music assets may enter the in-game playlist. */
bool music_playlist_is_track(const char *file);
/* Select next index, including the first entry when no track has been chosen. */
size_t music_playlist_next(size_t count, size_t current);
#endif
