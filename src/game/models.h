#ifndef OPENCK2_MODELS_H
#define OPENCK2_MODELS_H

#include <SDL.h>
#include <stdbool.h>

struct ck2_model_vertex {
    float x, y, z, nx, ny, nz, u, v;
};

struct ck2_model {
    struct ck2_model_vertex *vertices;
    Uint32 *indices;
    size_t vertex_count, index_count;
    float scale;
    char diffuse[256];
};

bool ck2_model_load(struct ck2_model *, const char *root, const char *actor);
void ck2_model_free(struct ck2_model *);

#endif
