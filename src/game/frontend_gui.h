#ifndef OPENCK2_FRONTEND_GUI_H
#define OPENCK2_FRONTEND_GUI_H

#include "ck2_script.h"

struct frontend_block {
    struct token type, name;
    const char *body;
    int parent;
};

struct frontend_gui {
    char *source;
    struct frontend_block *blocks;
    size_t count;
};

int frontend_gui_load_file(struct frontend_gui *, const char *root, const char *file);
int frontend_gui_load(struct frontend_gui *, const char *root);
void frontend_gui_free(struct frontend_gui *);
int frontend_gui_find(const struct frontend_gui *, int parent, const char *name);
bool frontend_gui_rect(const struct frontend_gui *, int block, SDL_Point viewport, SDL_Rect *);

#endif
