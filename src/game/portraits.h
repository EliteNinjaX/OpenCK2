#ifndef OPENCK2_PORTRAITS_H
#define OPENCK2_PORTRAITS_H

#include "era.h"

struct portrait_definition;
struct portrait_source {
    char **documents;
    struct portrait_definition *definitions;
    size_t document_count, culture_first, definition_count, definition_capacity;
    int early_before, late_after;
};

void portraits_open(struct portrait_source *, const char *);
void portraits_close(struct portrait_source *);
SDL_Texture *portrait_render(const struct portrait_source *, SDL_Renderer *, const char *,
                             const struct era_ruler *);

#endif
