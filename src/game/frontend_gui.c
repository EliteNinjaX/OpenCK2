#include "frontend_gui.h"

#include <stdlib.h>

void frontend_gui_free(struct frontend_gui *gui)
{
    SDL_free(gui->source);
    free(gui->blocks);
    *gui = (struct frontend_gui){0};
}

int frontend_gui_load_file(struct frontend_gui *out, const char *root, const char *file)
{
    struct frontend_gui gui = {.source = load_data(root, file)};
    if (!gui.source)
        return SDL_SetError("Could not read %s", file);
    int parent = -1;
    size_t capacity = 0;
    const char *p = gui.source;
    struct token previous = {0}, key = {0};
    while (*p) {
        const struct token t = token(&p);
        const bool quoted = t.start > gui.source && t.start[-1] == '"';
        if (!quoted && is(t, "{")) {
            if (gui.count == capacity) {
                capacity = capacity ? capacity * 2 : 512;
                struct frontend_block *blocks = realloc(gui.blocks, capacity * sizeof *blocks);
                if (!blocks) {
                    frontend_gui_free(&gui);
                    return SDL_OutOfMemory();
                }
                gui.blocks = blocks;
            }
            gui.blocks[gui.count] = (struct frontend_block){
                .type = is(previous, "=") ? key : (struct token){0},
                .name = field(p, "name"), .body = p, .parent = parent
            };
            parent = (int)gui.count++;
        } else if (!quoted && is(t, "}")) {
            if (parent < 0)
                goto invalid;
            parent = gui.blocks[parent].parent;
        }
        key = previous;
        previous = t;
    }
    if (parent >= 0 || !gui.count)
        goto invalid;
    frontend_gui_free(out);
    *out = gui;
    return 0;

invalid:
    frontend_gui_free(&gui);
    return SDL_SetError("%s: unbalanced or empty widget tree", file);
}

int frontend_gui_load(struct frontend_gui *out, const char *root)
{
    return frontend_gui_load_file(out, root, "interface/frontend.gui");
}

int frontend_gui_find(const struct frontend_gui *gui, int parent, const char *name)
{
    for (size_t i = 0; i < gui->count; ++i)
        if ((parent < 0 || gui->blocks[i].parent == parent) && is(gui->blocks[i].name, name))
            return (int)i;
    return -1;
}

static bool coordinate(struct token t, int *value)
{
    char buffer[24], *end;
    if (!t.size || t.size >= sizeof buffer)
        return false;
    copy_token(buffer, sizeof buffer, t);
    const long number = strtol(buffer, &end, 10);
    if (*end || number < -32768 || number > 32767)
        return false;
    *value = (int)number;
    return true;
}

bool frontend_gui_rect(const struct frontend_gui *gui, int node, SDL_Point viewport, SDL_Rect *rect)
{
    if (node < 0 || (size_t)node >= gui->count)
        return false;
    *rect = (SDL_Rect){0};
    const char *body = gui->blocks[node].body;
    const char *size = body;
    if (is(find(&size, "size"), "{")) {
        if (!coordinate(field(size, "x"), &rect->w) || !coordinate(field(size, "y"), &rect->h))
            return false;
    } else {
        coordinate(field(body, "maxWidth"), &rect->w);
        coordinate(field(body, "maxHeight"), &rect->h);
    }

    for (int i = node; i >= 0; i = gui->blocks[i].parent) {
        body = gui->blocks[i].body;
        struct token anchor = field(body, "Orientation");
        if (!anchor.size)
            anchor = field(body, "orientation");
        const char *position = body;
        SDL_Point offset = {0};
        if (is(find(&position, "position"), "{") &&
            (!coordinate(field(position, "x"), &offset.x) || !coordinate(field(position, "y"), &offset.y)))
            return false;
        if (is(anchor, "CENTER")) {
            offset.x += viewport.x / 2;
            offset.y += viewport.y / 2;
        } else if (is(anchor, "CENTER_UP"))
            offset.x += viewport.x / 2;
        else if (is(anchor, "UPPER_RIGHT"))
            offset.x += viewport.x;
        else if (is(anchor, "LOWER_LEFT"))
            offset.y += viewport.y;
        else if (is(anchor, "LOWER_RIGHT")) {
            offset.x += viewport.x;
            offset.y += viewport.y;
        }
        else if (anchor.size && !is(anchor, "UPPER_LEFT"))
            return false;
        rect->x += offset.x;
        rect->y += offset.y;
    }
    return true;
}
