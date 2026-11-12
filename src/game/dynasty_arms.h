#ifndef OPENCK2_DYNASTY_ARMS_H
#define OPENCK2_DYNASTY_ARMS_H
#include "dynasty_arms_core.h"
#include <SDL.h>

/* Rendered from CK2's own mounted files, without redistributing those assets. */
struct dynasty_arms_runtime;
struct dynasty_arms_runtime *dynasty_arms_open(const char *root);
SDL_Texture *dynasty_arms_render(struct dynasty_arms_runtime *, SDL_Renderer *,
                                 unsigned dynasty_id, const char *religion,
                                 bool *approximate);
void dynasty_arms_close(struct dynasty_arms_runtime *);
#endif
