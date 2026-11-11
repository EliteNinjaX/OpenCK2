#include "music.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

static void load_effects(struct menu_music *music, const char *root)
{
    static const char *const names[] = {
        "menu_click", "bookmark_click", "character_view_click", "click", "generic_click_04",
        "play", "quit", "time_back", "time_forward", "tutorial_click",
        "click_random_world", "click_shattered_world"
    };
    char *data = load_data(root, "interface/sound.sfx");
    if (!data)
        return;
    for (int i = 0; i < CUE_COUNT; ++i) {
        char name[40], file[128], relative[144];
        if (i < CUE_COUNTRY)
            SDL_strlcpy(name, names[i], sizeof name);
        else
            SDL_snprintf(name, sizeof name, "choose_country_%d", i - CUE_COUNTRY + 1);
        const char *block = data;
        if (!is(find(&block, name), "{"))
            continue;
        copy_token(file, sizeof file, field(block, "file"));
        if (!file[0] || strpbrk(file, "/\\"))
            continue;
        SDL_snprintf(relative, sizeof relative, "sound/%s", file);
        char *path = ck2_asset_path(root, relative);
        music->cues[i] = path ? Mix_LoadWAV(path) : NULL;
        free(path);
        if (music->cues[i]) {
            const struct token gain = field(block, "volume");
            const unsigned percent = gain.size ? number(gain) : 100;
            Mix_VolumeChunk(music->cues[i], (int)(MIX_MAX_VOLUME * SDL_min(percent, 100u) / 100));
        }
    }
    SDL_free(data);
}

void menu_music_start(struct menu_music *music, const char *asset_root)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        return;
    music->audio_initialized = true;
    music->mixer_initialized = true;
    Mix_Init(MIX_INIT_OGG);
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) < 0)
        return;
    music->device_open = true;
    load_effects(music, asset_root);
    char *path = ck2_asset_path(asset_root, "music/crusaderkings2_maintitle.ogg");
    music->track = path ? Mix_LoadMUS(path) : NULL;
    free(path);
    menu_music_set_volume(music, 100, 100, 100);
    if (music->track)
        Mix_PlayMusic(music->track, -1);
}

void menu_music_set_volume(const struct menu_music *music, int master, int track, int effects)
{
    if (!music->device_open)
        return;
    Mix_VolumeMusic((MIX_MAX_VOLUME * 48 * master * track + 500000) / 1000000);
    Mix_Volume(-1, (MIX_MAX_VOLUME * master * effects + 5000) / 10000);
}

void menu_sound_play(struct menu_music *music, enum menu_cue cue)
{
    if (cue == CUE_COUNTRY) {
        cue = (enum menu_cue)(CUE_COUNTRY + music->country_cue);
        music->country_cue = (music->country_cue + 1) % 10;
        if (!music->cues[cue])
            cue = CUE_CHARACTER;
    }
    if (music->cues[cue])
        Mix_PlayChannel(-1, music->cues[cue], 0);
}

void menu_sound_finish(struct menu_music *music)
{
    if (!music->device_open)
        return;
    const Uint64 deadline = SDL_GetTicks64() + 450;
    while (Mix_Playing(-1) && SDL_GetTicks64() < deadline)
        SDL_Delay(10);
}

void menu_music_stop(struct menu_music *music)
{
    if (music->track) {
        Mix_HaltMusic();
        Mix_FreeMusic(music->track);
    }
    if (music->device_open) {
        Mix_HaltChannel(-1);
        for (int i = 0; i < CUE_COUNT; ++i)
            if (music->cues[i])
                Mix_FreeChunk(music->cues[i]);
        Mix_CloseAudio();
    }
    if (music->mixer_initialized)
        Mix_Quit();
    if (music->audio_initialized)
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    *music = (struct menu_music){0};
}
