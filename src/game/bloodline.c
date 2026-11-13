#include "bloodline.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static char *duplicate_span(const char *start, size_t len)
{
    char *s = malloc(len + 1);
    if (!s) return NULL;
    memcpy(s, start, len);
    s[len] = '\0';
    return s;
}
static char *duplicate_token(struct token t)
{
    return duplicate_span(t.start, t.size);
}
static bool is_txt(const char *name)
{
    size_t n = strlen(name);
    return n >= 4 && strcmp(name + n - 4, ".txt") == 0;
}
static bool parse_inheritance(struct token t, ck2_blood_inheritance *out)
{
    if (is(t, "all_descendants")) *out = CK2_BLOOD_ALL_DESCENDANTS;
    else if (is(t, "matrilineal")) *out = CK2_BLOOD_MATRILINEAL;
    else if (is(t, "patrilineal")) *out = CK2_BLOOD_PATRILINEAL;
    else if (is(t, "no_inheritance")) *out = CK2_BLOOD_NO_INHERITANCE;
    else return false;
    return true;
}
static void free_type(ck2_bloodline_type *type)
{
    free(type->key);
    free(type->picture);
    free(type->script);
    memset(type, 0, sizeof *type);
}
static bool parse_type(ck2_bloodline_type *type, struct token key, const char *body)
{
    memset(type, 0, sizeof *type);
    type->key = duplicate_token(key);
    type->script = duplicate_span(body, strlen(body));
    type->inheritance = CK2_BLOOD_ALL_DESCENDANTS;
    type->matrilineal_override = true;
    if (!type->key || !type->script) goto fail;
    struct token t = field(body, "inheritance");
    if (t.size && !parse_inheritance(t, &type->inheritance)) goto fail;
    t = field(body, "matrilineal_override");
    if (t.size) type->matrilineal_override = is(t, "yes");
    t = field(body, "allow_bastards");
    if (t.size) type->allow_bastards = is(t, "yes");
    t = field(body, "picture");
    type->picture = t.size ? duplicate_token(t) : duplicate_span("GFX_bloodlines_placeholder_symbol", strlen("GFX_bloodlines_placeholder_symbol"));
    if (!type->picture) goto fail;
    return true;
fail:
    free_type(type);
    return false;
}
const ck2_bloodline_type *ck2_bloodline_find(const ck2_bloodline_registry *r, const char *key)
{
    if (!r || !key) return NULL;
    for (size_t i = 0; i < r->count; ++i)
        if (!strcmp(r->types[i].key, key)) return &r->types[i];
    return NULL;
}
static bool insert_type(ck2_bloodline_registry *r, ck2_bloodline_type *item)
{
    for (size_t i = 0; i < r->count; ++i) {
        if (!strcmp(r->types[i].key, item->key)) {
            free_type(&r->types[i]);
            r->types[i] = *item;
            return true;
        }
    }
    if (r->count == r->capacity) {
        size_t cap = r->capacity ? r->capacity * 2 : 32;
        if (cap < r->capacity || cap > SIZE_MAX / sizeof *r->types) return false;
        void *new_types = realloc(r->types, cap * sizeof *r->types);
        if (!new_types) return false;
        r->types = new_types;
        r->capacity = cap;
    }
    r->types[r->count++] = *item;
    return true;
}
static bool load_text(ck2_bloodline_registry *r, const char *text)
{
    const char *p = text;
    for (;;) {
        struct token key = token(&p);
        if (!key.size) return true;
        if (!is(token(&p), "=")) return false;
        struct token opening = token(&p);
        if (!is(opening, "{")) return false;
        const char *body_start = p;
        skip_block(&p);
        if (p <= body_start || p[-1] != '}') return false;
        char *body = duplicate_span(body_start, (size_t)(p - body_start - 1));
        if (!body) return false;
        ck2_bloodline_type item;
        bool ok = parse_type(&item, key, body);
        free(body);
        if (!ok) return false;
        if (!insert_type(r, &item)) { free_type(&item); return false; }
    }
}
bool ck2_bloodlines_parse_text(ck2_bloodline_registry *r, const char *text)
{
    return r && text && load_text(r, text);
}

bool ck2_bloodlines_load(ck2_bloodline_registry *r, const char *root)
{
    if (!r || !root) return false;
    struct ck2_asset_files files = ck2_list_assets(root, "common/bloodlines");
    bool ok = true;
    for (size_t i = 0; i < files.count; ++i) {
        if (!is_txt(files.names[i])) continue;
        char *data = load_from_directory(root, "common/bloodlines", files.names[i]);
        if (!data) { ok = false; break; }
        ok = load_text(r, data);
        free(data);
        if (!ok) break;
    }
    ck2_free_asset_files(&files);
    return ok;
}
void ck2_bloodlines_free(ck2_bloodline_registry *r)
{
    if (!r) return;
    for (size_t i = 0; i < r->count; ++i) free_type(&r->types[i]);
    free(r->types);
    memset(r, 0, sizeof *r);
}
bool ck2_bloodline_has_member(const ck2_bloodline *line, unsigned id)
{
    if (!line || !id) return false;
    for (size_t i = 0; i < line->count; ++i) if (line->members[i] == id) return true;
    return false;
}
bool ck2_bloodline_add_member(ck2_bloodline *line, unsigned id)
{
    if (!line || !id) return false;
    if (ck2_bloodline_has_member(line, id)) return true;
    if (line->count == line->capacity) {
        size_t cap = line->capacity ? line->capacity * 2 : 8;
        if (cap < line->capacity || cap > SIZE_MAX / sizeof *line->members) return false;
        void *new_members = realloc(line->members, cap * sizeof *line->members);
        if (!new_members) return false;
        line->members = new_members;
        line->capacity = cap;
    }
    line->members[line->count++] = id;
    return true;
}
void ck2_bloodline_remove_member(ck2_bloodline *line, unsigned id)
{
    if (!line) return;
    for (size_t i = 0; i < line->count; ++i) {
        if (line->members[i] == id) {
            memmove(&line->members[i], &line->members[i + 1],
                    (line->count - i - 1) * sizeof *line->members);
            --line->count;
            return;
        }
    }
}
void ck2_bloodline_free(ck2_bloodline *line)
{
    if (!line) return;
    free(line->members);
    memset(line, 0, sizeof *line);
}
bool ck2_bloodline_child_inherits(const ck2_bloodline *line,
    unsigned father_id, unsigned mother_id, bool father_known,
    bool matrilineal_marriage, bool child_is_bastard)
{
    if (!line || !line->type) return false;
    const ck2_bloodline_type *type = line->type;
    if (child_is_bastard && !type->allow_bastards) return false;
    bool father = father_known && ck2_bloodline_has_member(line, father_id);
    bool mother = ck2_bloodline_has_member(line, mother_id);
    switch (type->inheritance) {
    case CK2_BLOOD_ALL_DESCENDANTS: return father || mother;
    case CK2_BLOOD_PATRILINEAL:
        return father || (type->matrilineal_override && (matrilineal_marriage || !father_known) && mother);
    case CK2_BLOOD_MATRILINEAL:
        return mother || (type->matrilineal_override && (matrilineal_marriage || !father_known) && father);
    case CK2_BLOOD_NO_INHERITANCE: return false;
    }
    return false;
}
