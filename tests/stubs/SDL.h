#ifndef SDL_STUB_TEST_ONLY
#define SDL_STUB_TEST_ONLY
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
typedef uint32_t Uint32;
typedef uint8_t Uint8;
typedef uint64_t Uint64;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_RWops SDL_RWops;
typedef struct SDL_Surface {int w,h,pitch;void *pixels;} SDL_Surface;
typedef struct SDL_Point {int x,y;} SDL_Point;
typedef struct SDL_Rect {int x,y,w,h;} SDL_Rect;
#define SDL_arraysize(a) (sizeof(a)/sizeof((a)[0]))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_MUSTLOCK(a) (0)
#define SDL_LOG_CATEGORY_APPLICATION 0
#define SDL_free free
#define SDL_snprintf snprintf
static inline size_t SDL_strlcpy(char *dst,const char *src,size_t n){size_t slen=strlen(src);if(n){size_t k=slen<n-1?slen:n-1;memcpy(dst,src,k);dst[k]=0;}return slen;}
static inline int SDL_SetError(const char *format, ...){(void)format;return -1;}
static inline int SDL_OutOfMemory(void){return -1;}
static inline const char *SDL_GetError(void){return "stub";}
static inline void SDL_LogWarn(int cat,const char *fmt,...){(void)cat;(void)fmt;}
static inline int SDL_LockSurface(SDL_Surface *s){(void)s;return 0;}
static inline void SDL_UnlockSurface(SDL_Surface *s){(void)s;}
#define SDL_MAX_UINT32 UINT32_MAX
#define SDL_PIXELFORMAT_ARGB8888 0u
#define SDL_BLENDMODE_BLEND 1
static inline void *SDL_LoadFile(const char *path, size_t *length) {(void)path;(void)length;return NULL;}
#endif
