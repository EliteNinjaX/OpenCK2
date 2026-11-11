#include "music.h"

#include <stdio.h>
#include <string.h>

void menu_music_start(struct menu_music *music, const char *asset_root)
{
    const char suffix[] = "/music/crusaderkings2_maintitle.ogg";
    const size_t length = strlen(asset_root) + sizeof suffix;
    char *path = SDL_malloc(length);
    if (!path) {
        fputs("Menu music unavailable: could not allocate the track path.\n", stderr);
        return;
    }
    SDL_snprintf(path, length, "%s%s", asset_root, suffix);

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        goto unavailable;
    music->audio_initialized = true;
    music->mixer_initialized = true;
    if (!(Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG))
        goto unavailable;
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) < 0)
        goto unavailable;
    music->device_open = true;
    music->track = Mix_LoadMUS(path);
    if (!music->track)
        goto unavailable;

    /* The original track's volume in music/songs.txt is 0.48. */
    Mix_VolumeMusic((MIX_MAX_VOLUME * 48 + 50) / 100);
    if (Mix_PlayMusic(music->track, -1) < 0)
        goto unavailable;
    SDL_free(path);
    return;

unavailable:
    fprintf(stderr, "Menu music unavailable (%s): %s\n", path, Mix_GetError());
    SDL_free(path);
    menu_music_stop(music);
}

void menu_music_toggle(struct menu_music *music)
{
    if (!music->track)
        return;
    music->muted = !music->muted;
    if (music->muted)
        Mix_PauseMusic();
    else
        Mix_ResumeMusic();
}

void menu_music_stop(struct menu_music *music)
{
    if (music->track) {
        Mix_HaltMusic();
        Mix_FreeMusic(music->track);
    }
    if (music->device_open)
        Mix_CloseAudio();
    if (music->mixer_initialized)
        Mix_Quit();
    if (music->audio_initialized)
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    *music = (struct menu_music){0};
}
