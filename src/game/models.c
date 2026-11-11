#include "models.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include "../open_launcher/install.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct reader {
    const Uint8 *next;
    size_t left;
};

static const Uint8 *take(struct reader *reader, size_t size)
{
    if (size > reader->left)
        return NULL;
    const Uint8 *bytes = reader->next;
    reader->next += size;
    reader->left -= size;
    return bytes;
}

static Uint32 word(const Uint8 *bytes)
{
    Uint32 value;
    memcpy(&value, bytes, sizeof value);
    return SDL_SwapLE32(value);
}

static float real(const Uint8 *bytes)
{
    const Uint32 bits = word(bytes);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static bool actor_in_scope(const char **p, const char *name, char path[256], float *scale)
{
    bool found = false;
    for (struct token key; (key = token(p)).size && !is(key, "}");) {
        if (!is(token(p), "=") || !is(token(p), "{"))
            continue;
        const bool emfx = is(key, "EMFXActorType");
        if ((emfx || is(key, "pdxmesh")) && is(field(*p, "name"), name)) {
            const struct token file = field(*p, emfx ? "actorfile" : "file");
            if (file.size && file.size < 256) {
                char value[32];
                copy_token(path, 256, file);
                copy_token(value, sizeof value, field(*p, "scale"));
                const float factor = strtof(value, NULL);
                *scale = isfinite(factor) && factor > 0 ? factor : 1.0f;
                found = true;
            }
        }
        skip_block(p);
    }
    return found;
}

static bool actor_path(const char *root, const char *name, char path[256], float *scale)
{
    struct ck2_asset_files files = ck2_list_assets(root, "interface");
    bool found = false;
    size_t priority = 0;
    for (size_t i = 0; i < files.count; ++i) {
        const char *file = files.names[i];
        const size_t length = strlen(file);
        if (length < 4 || strcmp(file + length - 4, ".gfx"))
            continue;
        char *relative = ck2_join_path("interface", file);
        size_t layer;
        char *local = relative ? ck2_asset_path_with_priority(root, relative, &layer) : NULL;
        free(relative);
        if (!local)
            continue;
        char *data = !found || layer >= priority ? SDL_LoadFile(local, NULL) : NULL;
        free(local);
        if (!data)
            continue;
        const char *scope = data;
        if (is(find(&scope, "objectTypes"), "{") && actor_in_scope(&scope, name, path, scale)) {
            found = true;
            priority = layer;
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
    if (found) {
        for (char *p = path; *p; ++p)
            if (*p == '\\')
                *p = '/';
    }
    return found;
}

static void diffuse_path(struct ck2_model *model, const char *path, const Uint8 *name,
                         size_t size, const char *extension)
{
    if (model->diffuse[0] || !size || size >= 256)
        return;
    const char *slash = strrchr(path, '/');
    const int prefix = slash ? (int)(slash - path + 1) : 0;
    if (SDL_snprintf(model->diffuse, sizeof model->diffuse, "%.*s%.*s%s",
                     prefix, path, (int)size, (const char *)name, extension) >=
        (int)sizeof model->diffuse)
        model->diffuse[0] = 0;
}

/* Material v2 stores its texture layers after the declared chunk body. */
static bool material(struct ck2_model *model, struct reader chunk, struct reader *file,
                     const char *path)
{
    const Uint8 *header = take(&chunk, 84);
    if (!header)
        return false;
    for (unsigned i = 0; i < header[83]; ++i) {
        const Uint8 *layer = take(file, 32);
        if (!layer)
            return false;
        const Uint32 size = word(layer + 28);
        const Uint8 *name = take(file, size);
        if (!name)
            return false;
        if (layer[26] == 2)
            diffuse_path(model, path, name, size, ".dds");
    }
    return true;
}

static bool append_vertices(struct ck2_model *model, Uint32 vertices, Uint32 indices,
                            const Uint8 *positions, const Uint8 *normals, const Uint8 *uv)
{
    if (!positions || !vertices || !indices || vertices > 1000000 || indices > 6000000 ||
        model->vertex_count > 1000000 - vertices || model->index_count > 6000000 - indices)
        return false;

    struct ck2_model_vertex *out = realloc(model->vertices,
        (model->vertex_count + vertices) * sizeof *out);
    if (!out)
        return false;
    model->vertices = out;
    Uint32 *triangles = realloc(model->indices, (model->index_count + indices) * sizeof *triangles);
    if (!triangles)
        return false;
    model->indices = triangles;

    for (size_t i = 0; i < vertices; ++i) {
        const Uint8 *p = positions + i * 12;
        const Uint8 *n = normals ? normals + i * 12 : NULL;
        const Uint8 *t = uv ? uv + i * 8 : NULL;
        struct ck2_model_vertex vertex = {
            real(p), real(p + 4), real(p + 8),
            n ? real(n) : 0, n ? real(n + 4) : 1, n ? real(n + 8) : 0,
            t ? real(t) : 0, t ? real(t + 4) : 0
        };
        if (!isfinite(vertex.x) || !isfinite(vertex.y) || !isfinite(vertex.z))
            return false;
        /* Some shipped meshes leave UVs undefined on untextured vertices. */
        if (!isfinite(vertex.u) || !isfinite(vertex.v))
            vertex.u = vertex.v = 0;
        if (!isfinite(vertex.nx) || !isfinite(vertex.ny) || !isfinite(vertex.nz))
            vertex.nx = vertex.nz = 0, vertex.ny = 1;
        out[model->vertex_count + i] = vertex;
    }
    return true;
}

static bool mesh(struct ck2_model *model, struct reader chunk)
{
    const Uint8 *header = take(&chunk, 28);
    if (!header)
        return false;
    const Uint32 vertices = word(header + 8), indices = word(header + 12);
    const Uint32 parts = word(header + 16), layers = word(header + 20);
    const Uint8 *positions = NULL, *normals = NULL, *uv = NULL;
    if (!vertices || vertices > 1000000 || indices > 6000000 ||
        model->vertex_count > 1000000 - vertices || model->index_count > 6000000 - indices)
        return false;
    for (Uint32 i = 0; i < layers; ++i) {
        const Uint8 *layer = take(&chunk, 12);
        if (!layer)
            return false;
        const Uint32 type = word(layer), stride = word(layer + 4);
        if (stride > chunk.left / vertices)
            return false;
        const Uint8 *data = take(&chunk, (size_t)stride * vertices);
        if (type == 0 && stride == 12)
            positions = data;
        if (type == 1 && stride == 12)
            normals = data;
        if (type == 3 && stride == 8 && !uv)
            uv = data;
    }
    if (!append_vertices(model, vertices, indices, positions, normals, uv))
        return false;
    Uint32 first = 0, used = 0;
    for (Uint32 i = 0; i < parts; ++i) {
        const Uint8 *part = take(&chunk, 16);
        if (!part)
            return false;
        const Uint32 count = word(part), local = word(part + 4), bones = word(part + 12);
        if (count % 3 || count > indices - used || local > vertices - first)
            return false;
        for (Uint32 j = 0; j < count; ++j) {
            const Uint8 *index = take(&chunk, 4);
            if (!index || word(index) >= local)
                return false;
            model->indices[model->index_count + used++] = (Uint32)model->vertex_count + first + word(index);
        }
        if (bones > chunk.left / 4 || !take(&chunk, (size_t)bones * 4))
            return false;
        first += local;
    }
    if (used != indices || first != vertices)
        return false;
    model->index_count += indices;
    model->vertex_count += vertices;
    return true;
}

struct pdx_geometry {
    const Uint8 *positions, *normals, *uv, *triangles;
    Uint32 positions_count, normals_count, uv_count, index_count;
};

static bool append_pdx_geometry(struct ck2_model *model, const struct pdx_geometry *g)
{
    const Uint32 vertices = g->positions_count / 3;
    if (g->positions_count % 3 || g->index_count % 3 ||
        (g->normals && g->normals_count != vertices * 3) ||
        (g->uv && g->uv_count != vertices * 2) || !g->triangles ||
        !append_vertices(model, vertices, g->index_count, g->positions, g->normals, g->uv))
        return false;
    for (Uint32 i = 0; i < g->index_count; ++i) {
        const Uint32 index = word(g->triangles + (size_t)i * 4);
        if (index >= vertices)
            return false;
        model->indices[model->index_count + i] = (Uint32)model->vertex_count + index;
    }
    model->index_count += g->index_count;
    model->vertex_count += vertices;
    return true;
}

/* Clausewitz binary objects use repeated '[' for depth and typed '!' properties. */
static bool decode_pdx(struct ck2_model *model, struct reader file, const char *path)
{
    take(&file, 4);
    struct pdx_geometry geometry = {0};
    unsigned depth = 0;
    bool objects = false, in_mesh = false, in_material = false, base_lod = true;
    while (file.left) {
        if (*file.next == '[') {
            depth = 0;
            while (file.left && *file.next == '[') {
                take(&file, 1);
                ++depth;
            }
            const Uint8 *end = memchr(file.next, 0, file.left);
            if (!end)
                return false;
            const char *name = (const char *)take(&file, (size_t)(end - file.next) + 1);
            if (depth <= 3) {
                if (in_mesh && !append_pdx_geometry(model, &geometry))
                    return false;
                geometry = (struct pdx_geometry){0};
                in_mesh = false;
            }
            if (depth == 1)
                objects = !strcmp(name, "object");
            if (depth == 2)
                base_lod = true;
            if (depth == 3)
                in_mesh = objects && base_lod && !strcmp(name, "mesh");
            if (depth <= 4)
                in_material = depth == 4 && in_mesh && !strcmp(name, "material");
            continue;
        }

        const Uint8 *property = take(&file, 2);
        if (!property || property[0] != '!')
            return false;
        const char *name = (const char *)take(&file, property[1]);
        const Uint8 *type = take(&file, 5);
        if (!name || !type)
            return false;
        const Uint32 count = word(type + 1);
        if (type[0] == 's' && count == 1) {
            const Uint8 *length = take(&file, 4);
            if (!length)
                return false;
            const Uint32 size = word(length);
            const Uint8 *data = take(&file, size);
            if (!data)
                return false;
            if (in_material && depth == 4 && property[1] == 4 && !memcmp(name, "diff", 4))
                diffuse_path(model, path, data, size, "");
            continue;
        }
        if ((type[0] != 'f' && type[0] != 'i') || count > file.left / 4)
            return false;
        const Uint8 *data = take(&file, (size_t)count * 4);
        const struct token key = {name, property[1]};
        if (depth == 2 && type[0] == 'i' && count == 1 && is(key, "lod"))
            base_lod = word(data) == 0;
        if (!in_mesh || depth != 3)
            continue;
        if (type[0] == 'f' && is(key, "p")) {
            geometry.positions = data;
            geometry.positions_count = count;
        } else if (type[0] == 'f' && is(key, "n")) {
            geometry.normals = data;
            geometry.normals_count = count;
        } else if (type[0] == 'f' && is(key, "u0")) {
            geometry.uv = data;
            geometry.uv_count = count;
        } else if (type[0] == 'i' && is(key, "tri")) {
            geometry.triangles = data;
            geometry.index_count = count;
        }
    }
    return (!in_mesh || append_pdx_geometry(model, &geometry)) && model->index_count != 0;
}

static bool decode(struct ck2_model *model, struct reader file, const char *path)
{
    const Uint8 *magic = take(&file, 8);
    if (!magic || memcmp(magic, "XAC ", 4) || magic[6] != 0)
        return false;
    while (file.left) {
        const Uint8 *header = take(&file, 12);
        if (!header)
            return false;
        const Uint32 id = word(header), size = word(header + 4), version = word(header + 8);
        const Uint8 *body = take(&file, size);
        if (!body)
            return false;
        const struct reader chunk = {body, size};
        if (id == 1 && (version != 1 || !mesh(model, chunk)))
            return false;
        if (id == 3 && (version != 2 || !material(model, chunk, &file, path)))
            return false;
    }
    return model->index_count != 0;
}

bool ck2_model_load(struct ck2_model *model, const char *root, const char *actor)
{
    struct ck2_model next = {0};
    char path[256];
    if (!actor_path(root, actor, path, &next.scale))
        return false;
    char *local = ck2_asset_path(root, path);
    size_t size;
    Uint8 *data = local ? SDL_LoadFile(local, &size) : NULL;
    free(local);
    if (!data)
        return false;
    const struct reader file = {data, size};
    const bool loaded = size >= 4 && !memcmp(data, "@@b@", 4)
        ? decode_pdx(&next, file, path) : decode(&next, file, path);
    SDL_free(data);
    if (loaded) {
        ck2_model_free(model);
        *model = next;
    } else {
        ck2_model_free(&next);
    }
    return loaded;
}

void ck2_model_free(struct ck2_model *model)
{
    free(model->vertices);
    free(model->indices);
    *model = (struct ck2_model){0};
}
