#include "../src/game/music_playlist_core.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    assert(music_playlist_is_track("the_king.ogg"));
    assert(music_playlist_is_track("music.MP3"));
    assert(music_playlist_is_track("a.flac"));
    assert(!music_playlist_is_track("../evil.ogg"));
    assert(!music_playlist_is_track("folder/a.ogg"));
    assert(!music_playlist_is_track("a.ogg.bak"));
    assert(!music_playlist_is_track(""));
    assert(music_playlist_next(0, 0) == 0);
    assert(music_playlist_next(4, 4) == 0);
    assert(music_playlist_next(4, 3) == 0);
    assert(music_playlist_next(4, 2) == 3);
    puts("music playlist: all assertions passed");
    return 0;
}
