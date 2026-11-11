#ifndef CK2_GAME_MUSIC_H
#define CK2_GAME_MUSIC_H

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdbool.h>

struct menu_music {
    Mix_Music *track;
    bool audio_initialized, mixer_initialized, device_open, muted;
};

void menu_music_start(struct menu_music *music, const char *asset_root);
void menu_music_toggle(struct menu_music *music);
void menu_music_stop(struct menu_music *music);

#endif
