#include "../open_launcher/asset_paths.h"
#include "../open_launcher/install.h"
#include "portraits.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include "../open_launcher/install.h"

#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct portrait_definition {
    char name[128];
    const char *block;
    size_t order, priority;
    bool portrait;
};

static int compare(const void *a, const void *b)
{
    const struct portrait_definition *x = a, *y = b;
    if (x->portrait != y->portrait)
        return x->portrait ? 1 : -1;
    return strcmp(x->name, y->name);
}

static int compare_order(const void *a, const void *b)
{
    const int name = compare(a, b);
    if (name)
        return name;
    const struct portrait_definition *x = a, *y = b;
    if (x->priority != y->priority)
        return (x->priority > y->priority) - (x->priority < y->priority);
    return (x->order > y->order) - (x->order < y->order);
}

static bool remember(struct portrait_source *source, char *data)
{
    char **documents = realloc(source->documents, (source->document_count + 1) * sizeof *documents);
    if (!documents)
        return false;
    source->documents = documents;
    source->documents[source->document_count++] = data;
    return true;
}

static void index_definitions(struct portrait_source *source, const char *p, size_t priority)
{
    for (struct token t; (t = token(&p)).size || *t.start;) {
        const bool portrait = is(t, "portraitType");
        if (!portrait && !is(t, "spriteType") && !is(t, "textSpriteType"))
            continue;
        if (!is(token(&p), "=") || !is(token(&p), "{"))
            continue;
        const char *block = p;
        skip_block(&p);
        const struct token name = field(block, "name");
        if (!name.size)
            continue;
        if (source->definition_count == source->definition_capacity) {
            const size_t capacity = source->definition_capacity ? source->definition_capacity * 2 : 1024;
            struct portrait_definition *next = realloc(source->definitions, capacity * sizeof *next);
            if (!next)
                return;
            source->definitions = next;
            source->definition_capacity = capacity;
        }
        const size_t order = source->definition_count++;
        struct portrait_definition *record = &source->definitions[order];
        *record = (struct portrait_definition){.block = block, .order = order, .priority = priority, .portrait = portrait};
        copy_token(record->name, sizeof record->name, name);
    }
}

static void index_directory(struct portrait_source *source, const char *root, const char *dir,
                            bool cultures)
{
    struct ck2_asset_files files = ck2_list_assets(root, dir);
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t length = strlen(name);
        if (length < 4 || strcmp(name + length - 4, cultures ? ".txt" : ".gfx"))
            continue;
        char *relative = ck2_join_path(dir, name);
        size_t priority = 0;
        char *path = relative ? ck2_asset_path_with_priority(root, relative, &priority) : NULL;
        char *data = path ? SDL_LoadFile(path, NULL) : NULL;
        free(path);
        free(relative);
        if (!data)
            continue;
        if (!remember(source, data)) {
            SDL_free(data);
            break;
        }
        if (!cultures)
            index_definitions(source, data, priority);
    }
    ck2_free_asset_files(&files);
}

static int year_setting(const char *data, const char *key, int fallback)
{
    const char *p = data ? strstr(data, key) : NULL;
    if (p) {
        p += strlen(key);
        if (is(token(&p), "=")) {
            const unsigned value = number(token(&p));
            if (value && value <= 9999)
                return (int)value;
        }
    }
    return fallback;
}

void portraits_open(struct portrait_source *source, const char *root)
{
    index_directory(source, root, "interface/portraits", false);
    index_directory(source, root, "interface", false);
    if (source->definition_count > 1)
        qsort(source->definitions, source->definition_count, sizeof *source->definitions, compare_order);
    size_t kept = 0;
    for (size_t i = 0; i < source->definition_count; ++i)
        if (i + 1 == source->definition_count || compare(&source->definitions[i], &source->definitions[i + 1]))
            source->definitions[kept++] = source->definitions[i];
    source->definition_count = kept;
    source->culture_first = source->document_count;
    index_directory(source, root, "common/cultures", true);
    char *defines = load_data(root, "common/defines.lua");
    source->early_before = year_setting(defines, "EARLY_PORTRAIT_AND_UNIT_BEFORE_YEAR", 950);
    source->late_after = year_setting(defines, "LATE_PORTRAIT_AND_UNIT_AFTER_YEAR", 1200);
    source->adult_male_age = year_setting(defines, "PORTRAIT_ADULT_MALE_AGE_THRESHOLD", 16);
    source->adult_female_age = year_setting(defines, "PORTRAIT_ADULT_FEMALE_AGE_THRESHOLD", 16);
    source->mid_age = year_setting(defines, "PORTRAIT_MID_AGE_THRESHOLD", 30);
    source->old_age = year_setting(defines, "PORTRAIT_OLD_AGE_THRESHOLD", 50);
    SDL_free(defines);
}

void portraits_close(struct portrait_source *source)
{
    for (size_t i = 0; i < source->document_count; ++i)
        SDL_free(source->documents[i]);
    free(source->documents);
    free(source->definitions);
    *source = (struct portrait_source){0};
}

static const char *definition(const struct portrait_source *source, const char *name, bool portrait)
{
    struct portrait_definition key = {.portrait = portrait};
    SDL_strlcpy(key.name, name, sizeof key.name);
    const struct portrait_definition *result = source->definition_count
        ? bsearch(&key, source->definitions, source->definition_count, sizeof key, compare) : NULL;
    return result ? result->block : NULL;
}

static size_t culture_groups(const struct portrait_source *source, const char *culture,
                             char groups[][64], size_t capacity)
{
    size_t count = 0;
    for (size_t i = source->culture_first; i < source->document_count; ++i) {
        const char *p = source->documents[i];
        for (struct token key; (key = token(&p)).size || *key.start;) {
            if (!is(key, culture))
                continue;
            const char *block = p;
            if (!is(token(&block), "=") || !is(token(&block), "{"))
                continue;
            const char *values = block;
            if (!is(find(&values, "graphical_cultures"), "{"))
                continue;
            count = 0;
            for (struct token name; count < capacity && (name = token(&values)).size && !is(name, "}");)
                copy_token(groups[count++], sizeof groups[0], name);
        }
    }
    return count;
}

static int gene_index(const char *sequence, unsigned slot)
{
    if (slot >= strlen(sequence))
        return 0;
    const char c = sequence[slot];
    if (c >= 'a' && c <= 'z')
        return c - 'a';
    if (c >= '0' && c <= '9')
        return c - '0';
    return 0;
}

static void read_rgb(const char **p, int rgb[3])
{
    if (!is(token(p), "{"))
        return;
    for (int i = 0; i < 3; ++i) {
        const unsigned channel = number(token(p));
        rgb[i] = (int)SDL_min(255u, channel);
    }
    token(p);
}

static void tint_palette(const char *definition_text, const char *field_name, int index,
                         int palette[][3], int colors)
{
    const char *p = definition_text;
    if (!is(find(&p, field_name), "{"))
        return;
    for (int i = 0; i <= index; ++i) {
        const char *next = p;
        if (!is(token(&next), "{"))
            break;
        for (int j = 0; j < colors; ++j)
            read_rgb(&p, palette[j]);
    }
}

static void tint_layer(SDL_Surface *surface, SDL_Rect part, const char *tint,
                       int hair[3][3], int eye[3])
{
    for (int y = 0; y < part.h; ++y) {
        Uint8 *row = (Uint8 *)surface->pixels + y * surface->pitch + part.x * 4;
        for (int x = 0; x < part.w; ++x) {
            Uint8 *p = row + 4 * x;
            if (!p[3])
                continue;
            if (tint[0] == 'h') {
                const int light = p[1] * 2;
                const int lower = light < 255 ? 0 : 1;
                const int blend = light - lower * 255;
                for (int c = 0; c < 3; ++c)
                    p[c] = (Uint8)((hair[lower][2 - c] * (255 - blend) +
                                     hair[lower + 1][2 - c] * blend) / 255);
            } else {
                const int light = p[2];
                for (int c = 0; c < 3; ++c)
                    p[c] = (Uint8)(eye[2 - c] * light / 255);
            }
        }
    }
}

static SDL_Surface *load_layer(const char *root, char *relative)
{
    const size_t length = strlen(relative);
    if (length >= 4 && !SDL_strcasecmp(relative + length - 4, ".dds"))
        return ck2_load_dds_surface(root, relative);
    char *path = ck2_asset_path(root, relative);
    SDL_Surface *surface = path ? IMG_Load(path) : NULL;
    free(path);
    if (!surface && length >= 4 && !SDL_strcasecmp(relative + length - 4, ".tga")) {
        memcpy(relative + length - 4, ".dds", 5);
        surface = ck2_load_dds_surface(root, relative);
    }
    if (surface && surface->format->format != SDL_PIXELFORMAT_BGRA32) {
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_BGRA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
    }
    return surface;
}

static bool sprite_path(const char *sprite, char *path, size_t capacity)
{
    struct token asset = field(sprite, "texturefile");
    if (!asset.size)
        asset = field(sprite, "textureFile");
    size_t len = 0;
    for (size_t i = 0; i < asset.size; ++i) {
        if (len + 1 >= capacity)
            return false;
        if (asset.start[i] == '\\' || asset.start[i] == '/') {
            if (!len || path[len - 1] != '/')
                path[len++] = '/';
        } else {
            path[len++] = asset.start[i];
        }
    }
    path[len] = 0;
    if (!len || path[0] == '/' || strstr(path, ".."))
        return false;
    return true;
}

static bool draw_layer(const struct portrait_source *source, SDL_Surface *portrait,
                       const char *root, const struct era_ruler *ruler,
                       char *entry, int hair[3][3], int eye[3])
{
    char *gene = strchr(entry, ':');
    if (!gene || gene == entry)
        return false;
    *gene++ = '\0';
    const char *sprite = definition(source, entry, false);
    if (!sprite)
        return false;
    char path[200];
    if (!sprite_path(sprite, path, sizeof path))
        return false;
    const unsigned slot = (unsigned)strtoul(gene + 1, NULL, 10);
    if (gene[0] == 'p' && slot > 5 && slot >= strlen(ruler->properties))
        return false;
    SDL_Surface *layer = load_layer(root, path);
    if (!layer)
        return false;
    const int frames = (int)SDL_clamp(number(field(sprite, "noOfFrames")), 1u, (unsigned)layer->w);
    const int index = gene_index(gene[0] == 'd' ? ruler->dna : ruler->properties, slot) % frames;
    const int width = layer->w / frames;
    const SDL_Rect part = {index * width, 0, width, layer->h};
    SDL_Rect target = {0, 0, width, layer->h};
    const char *offset = strstr(gene, ":o");
    if (offset)
        sscanf(offset + 2, "%dx%d", &target.x, &target.y);
    if (strstr(gene, ":h"))
        tint_layer(layer, part, "h", hair, eye);
    else if (strstr(gene, ":e"))
        tint_layer(layer, part, "e", hair, eye);
    SDL_SetSurfaceBlendMode(layer, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(layer, &part, portrait, &target);
    SDL_FreeSurface(layer);
    return strstr(entry, ruler->female ? "_female_base" : "_male_base") != NULL;
}

/* Stable fallback for historical records without authored DNA.
   Vanilla's initialization algorithm has not been recovered. */
static void fallback_dna(struct era_ruler *ruler)
{
    Uint32 value = ruler->id ^ 0x9e3779b9u;
    for (size_t i = 0; i < 11; ++i) {
        value = value * 1664525u + 1013904223u;
        ruler->dna[i] = (char)('a' + (value >> 16) % 26);
    }
    ruler->dna[11] = 0;
}

static SDL_Texture *child_portrait(const struct portrait_source *source, SDL_Renderer *renderer,
                                  const char *root, char groups[][64], size_t count, const char *sex)
{
    for (size_t i = 0; i < count; ++i) {
        char name[112], path[200];
        SDL_snprintf(name, sizeof name, "PORTRAIT_%s_child_%s", groups[i], sex);
        const char *sprite = definition(source, name, false);
        if (!sprite || !sprite_path(sprite, path, sizeof path))
            continue;
        SDL_Surface *art = load_layer(root, path);
        if (!art)
            continue;
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, art);
        SDL_FreeSurface(art);
        if (texture)
            return texture;
    }
    return NULL;
}

SDL_Texture *portrait_render(const struct portrait_source *source, SDL_Renderer *renderer,
                             const char *root, const struct era_ruler *ruler)
{
    if (!ruler->dna[0] && !ruler->id)
        return NULL;
    struct era_ruler generated;
    if (!ruler->dna[0]) {
        generated = *ruler;
        fallback_dna(&generated);
        ruler = &generated;
    }
    char groups[16][64];
    size_t count = culture_groups(source, ruler->culture, groups, SDL_arraysize(groups) - 2);
    if (!strcmp(ruler->religion, "sunni") || !strcmp(ruler->religion, "shiite"))
        SDL_strlcpy(groups[count++], "muslimgfx", sizeof groups[0]);
    SDL_strlcpy(groups[count++], "westerngfx", sizeof groups[0]);
    const char *period = ruler->portrait_year > 0 && ruler->portrait_year < source->early_before ? "_early"
        : ruler->portrait_year > source->late_after ? "_late" : "";
    const char *sex = ruler->female ? "female" : "male";
    const unsigned adult_age = (unsigned)(ruler->female ? source->adult_female_age : source->adult_male_age);
    if ((ruler->age_known || ruler->age) && ruler->age < adult_age) {
        SDL_Texture *child = child_portrait(source, renderer, root, groups, count, sex);
        if (child)
            return child;
    }
    const char *age = ruler->age < (unsigned)source->mid_age ? ""
        : ruler->age < (unsigned)source->old_age ? "1" : "2";
    SDL_Surface *portrait = SDL_CreateRGBSurfaceWithFormat(0, 152, 152, 32, SDL_PIXELFORMAT_BGRA32);
    if (!portrait)
        return NULL;
    bool base = false;
    for (size_t attempt = 0; attempt < count && !base; ++attempt) {
        SDL_FillRect(portrait, NULL, 0);
        char name[112];
        SDL_snprintf(name, sizeof name, "PORTRAIT_%s_%s%s%s", groups[attempt], sex, period, age);
        const char *def = definition(source, name, true);
        if (!def && *period) {
            SDL_snprintf(name, sizeof name, "PORTRAIT_%s_%s%s", groups[attempt], sex, age);
            def = definition(source, name, true);
        }
        const char *layer = def;
        if (def && is(find(&layer, "layer"), "{")) {
            int hair[3][3] = {{35, 25, 20}, {112, 78, 53}, {255, 255, 255}};
            int eye[1][3] = {{94, 120, 140}};
            tint_palette(def, "hair_color", gene_index(ruler->dna, number(field(def, "hair_color_index"))),
                         hair, 3);
            tint_palette(def, "eye_color", gene_index(ruler->dna, number(field(def, "eye_color_index"))),
                         eye, 1);
            for (struct token t; (t = token(&layer)).size && !is(t, "}");) {
                char entry[180];
                copy_token(entry, sizeof entry, t);
                if (draw_layer(source, portrait, root, ruler, entry, hair, eye[0]))
                    base = true;
            }
        }
    }
    for (int y = 0; y < portrait->h; ++y) {
        Uint8 *row = (Uint8 *)portrait->pixels + y * portrait->pitch;
        for (int x = 0; x < portrait->w; ++x) {
            const int dx = x - 76, dy = y - 76;
            if (dx * dx + dy * dy >= 75 * 75)
                row[4 * x + 3] = 0;
        }
    }
    SDL_Texture *texture = base ? SDL_CreateTextureFromSurface(renderer, portrait) : NULL;
    SDL_FreeSurface(portrait);
    return texture;
}
