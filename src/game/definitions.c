#include "definitions.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <stdlib.h>
#include <string.h>

static char *slice(const char *p, size_t n)
{
    char *s = malloc(n + 1);
    if (s) { memcpy(s, p, n); s[n] = 0; }
    return s;
}
void ck2_definitions_free(struct ck2_definitions *d)
{
    for (size_t i = 0; i < d->count; ++i) {
        free(d->items[i].key); free(d->items[i].group); free(d->items[i].body);
    }
    free(d->items); *d = (struct ck2_definitions){0};
}
const struct ck2_definition *ck2_definition_get(const struct ck2_definitions *d, const char *key)
{
    for (size_t i = 0; i < d->count; ++i)
        if (!strcmp(key, d->items[i].key)) return &d->items[i];
    return NULL;
}
static bool add(struct ck2_definitions *d, const char *key, const char *group, const char *body)
{
    struct ck2_definition v = {slice(key, strlen(key)), slice(group, strlen(group)), slice(body, strlen(body))};
    if (!v.key || !v.group || !v.body) { free(v.key); free(v.group); free(v.body); return false; }
    size_t i = 0;
    while (i < d->count && strcmp(d->items[i].key, key)) ++i;
    if (i == d->count) {
        void *p = realloc(d->items, (d->count + 1) * sizeof *d->items);
        if (!p) { free(v.key); free(v.group); free(v.body); return false; }
        d->items = p; ++d->count;
    } else { free(d->items[i].key); free(d->items[i].group); free(d->items[i].body); }
    d->items[i] = v; return true;
}
static bool scope(struct ck2_definitions *d, const char **p, bool grouped, const char *group, bool nested)
{
    for (;;) {
        struct token k = token(p);
        if (!k.size) return !nested;
        if (is(k, "}")) return nested;
        if (is(k, "{") || is(k, "=") || !is(token(p), "=")) return false;
        struct token v = token(p);
        if (!is(v, "{")) return false;
        const char *begin = *p;
        unsigned depth = 1;
        struct token t;
        while (depth) {
            t = token(p);
            if (!t.size) return false;
            if (is(t, "{")) ++depth;
            if (is(t, "}")) --depth;
        }
        char *key = slice(k.start, k.size), *body = slice(begin, (size_t)(t.start - begin));
        if (!key || !body) { free(key); free(body); return false; }
        bool ok;
        if (grouped) {
            const char *q = body;
            ok = scope(d, &q, false, key, false);
        } else ok = add(d, key, group, body);
        free(key); free(body);
        if (!ok) return false;
    }
}
bool ck2_definitions_parse(struct ck2_definitions *d, const char *text, bool grouped)
{
    struct ck2_definitions next = {0};
    for (size_t i = 0; i < d->count; ++i)
        if (!add(&next, d->items[i].key, d->items[i].group, d->items[i].body)) goto fail;
    if (!scope(&next, &text, grouped, "", false)) goto fail;
    ck2_definitions_free(d); *d = next; return true;
fail:
    ck2_definitions_free(&next); return false;
}
bool ck2_definitions_load(struct ck2_definitions *d, const char *root, bool governments)
{
    const char *directory = governments ? "common/governments" : "common/bloodlines";
    struct ck2_asset_files files = ck2_list_assets(root, directory);
    bool ok = true;
    for (size_t i = 0; i < files.count; ++i) {
        size_t n = strlen(files.names[i]);
        if (n < 4 || strcmp(files.names[i] + n - 4, ".txt")) continue;
        char *text = load_from_directory(root, directory, files.names[i]);
        if (!text || !ck2_definitions_parse(d, text, governments)) ok = false;
        SDL_free(text);
    }
    ck2_free_asset_files(&files); return ok;
}

enum ck2_bloodline_inheritance ck2_bloodline_inheritance(const struct ck2_definition *d)
{
    if (!d) return CK2_BLOODLINE_UNKNOWN;
    struct token t = field(d->body, "inheritance");
    if (is(t, "all_descendants")) return CK2_BLOODLINE_ALL_DESCENDANTS;
    if (is(t, "matrilineal")) return CK2_BLOODLINE_MATRILINEAL;
    if (is(t, "patrilineal")) return CK2_BLOODLINE_PATRILINEAL;
    if (is(t, "no_inheritance")) return CK2_BLOODLINE_NO_INHERITANCE;
    return CK2_BLOODLINE_UNKNOWN;
}
