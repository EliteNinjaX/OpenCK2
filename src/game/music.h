#ifndef CK2_GAME_MUSIC_H
#define CK2_GAME_MUSIC_H

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdbool.h>
#include <stddef.h>

enum menu_cue {
    CUE_MENU, CUE_BOOKMARK, CUE_CHARACTER, CUE_CLICK, CUE_GENERIC, CUE_PLAY, CUE_QUIT,
    CUE_TIME_BACK, CUE_TIME_FORWARD, CUE_TUTORIAL, CUE_RANDOM, CUE_SHATTERED,
    CUE_COUNTRY, CUE_COUNT = CUE_COUNTRY + 10
};

struct menu_music {
    Mix_Music *track;
    char **song_names;
    size_t song_count, next_song_index;
    Mix_Chunk *cues[CUE_COUNT];
    int country_cue;
    bool audio_initialized, mixer_initialized, device_open;
};

bool menu_music_next(struct menu_music *, const char *asset_root);
void menu_music_start(struct menu_music *music, const char *asset_root);
void menu_music_set_volume(const struct menu_music *music, int master, int track, int effects);
void menu_sound_play(struct menu_music *, enum menu_cue);
void menu_sound_stop_country(const struct menu_music *);
void menu_sound_finish(struct menu_music *);
void menu_music_stop(struct menu_music *music);

#endif
