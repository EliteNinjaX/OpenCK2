#include "checkpoint_core.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Explicit little-endian primitives and length-prefixed UTF-8 are used rather
 * than raw compiler structs, pointers, locale-dependent text or CK2 saves.
 * Version 2 extends the bounded prototype with government mechanics; the
 * reader can still decode version 1 checkpoints. This is not a CK2 save. */
enum { CKPT_VERSION = 2, CKPT_MAX_PERSONS = 10000, CKPT_MAX_CONVERSIONS = 200000,
       CKPT_MAX_LAWS = 1024, CKPT_MAX_CONSTRUCTION = 100000,
       CKPT_MAX_TITLES = 50000, CKPT_MAX_BYTES = 64 * 1024 * 1024 };
static const char ckpt_magic[8] = {'O', 'C', 'K', '2', 'S', 'A', 'V', 'E'};
static char failure[160];

const char *checkpoint_last_error(void) { return failure[0] ? failure : "Checkpoint unavailable"; }

struct ck_io {
    FILE *stream;
    bool writing, ok;
    uint64_t hash;
    size_t size;
};
static bool io_bytes(struct ck_io *io, void *data, size_t length)
{
    if (!io->ok) return false;
    if (length > CKPT_MAX_BYTES - io->size) { io->ok = false; return false; }
    size_t done = io->writing ? fwrite(data, 1, length, io->stream)
                              : fread(data, 1, length, io->stream);
    if (done != length) { io->ok = false; return false; }
    for (const unsigned char *p = data; p < (const unsigned char *)data + length; ++p) {
        io->hash ^= *p;
        io->hash *= UINT64_C(1099511628211);
    }
    io->size += length;
    return true;
}
static void io_u32(struct ck_io *io, unsigned *v)
{
    unsigned char bytes[4];
    if (io->writing) for (unsigned i = 0; i < 4; ++i) bytes[i] = (unsigned char)(*v >> (8 * i));
    if (!io_bytes(io, bytes, 4)) return;
    if (!io->writing) {
        *v = 0;
        for (unsigned i = 0; i < 4; ++i) *v |= (unsigned)bytes[i] << (8 * i);
    }
}
static void io_s32(struct ck_io *io, int *v)
{
    unsigned n = io->writing ? (unsigned)(uint32_t)*v : 0;
    io_u32(io, &n);
    if (!io->writing && io->ok) *v = (int)(int32_t)n;
}
static void io_u64(struct ck_io *io, uint64_t *v)
{
    unsigned char bytes[8];
    if (io->writing) for (unsigned i = 0; i < 8; ++i) bytes[i] = (unsigned char)(*v >> (8 * i));
    if (!io_bytes(io, bytes, 8)) return;
    if (!io->writing) {
        *v = 0;
        for (unsigned i = 0; i < 8; ++i) *v |= (uint64_t)bytes[i] << (8 * i);
    }
}
static void io_size(struct ck_io *io, size_t *count, size_t max)
{
    if (*count > max) { io->ok = false; return; }
    unsigned n = io->writing ? (unsigned)*count : 0;
    io_u32(io, &n);
    if (!io->writing) {
        if (n > max) io->ok = false;
        else *count = n;
    }
}
static void io_bool(struct ck_io *io, bool *v)
{
    unsigned n = io->writing ? (unsigned)*v : 0;
    io_u32(io, &n);
    if (n > 1) io->ok = false;
    if (!io->writing) *v = (n == 1);
}
static void io_double(struct ck_io *io, double *v)
{
    uint64_t bits = 0;
    if (sizeof *v != sizeof bits || (io->writing && !isfinite(*v))) { io->ok = false; return; }
    if (io->writing) memcpy(&bits, v, sizeof bits);
    io_u64(io, &bits);
    if (!io->writing && io->ok) {
        memcpy(v, &bits, sizeof bits);
        if (!isfinite(*v)) io->ok = false;
    }
}
static void io_string(struct ck_io *io, char *text, size_t capacity)
{
    if (!io->ok || !capacity) { io->ok = false; return; }
    size_t length = 0;
    if (io->writing) {
        while (length < capacity && text[length]) ++length;
        if (length == capacity) { io->ok = false; return; }
    }
    unsigned n = (unsigned)length;
    io_u32(io, &n);
    if (!io->ok || n >= capacity) { io->ok = false; return; }
    io_bytes(io, text, n);
    if (!io->writing && io->ok) {
        if (memchr(text, 0, n)) { io->ok = false; return; }
        text[n] = '\0';
    }
}
#define STR(io, obj, field) io_string((io), (obj)->field, sizeof (obj)->field)
#define U32(io, obj, field) io_u32((io), &(obj)->field)
#define BOOL(io, obj, field) io_bool((io), &(obj)->field)
#define DBL(io, obj, field) io_double((io), &(obj)->field)
#define DATE(io, obj, field) io_date((io), &(obj)->field)

static void io_date(struct ck_io *io, struct campaign_date *v)
{
    io_s32(io, &v->year);
    io_s32(io, &v->month);
    io_s32(io, &v->day);
    if (!io->ok) return;
    if ((v->year == 0 && v->month == 0 && v->day == 0)) return;
    if (v->year <= 0 || v->year > 10000 || v->month < 1 || v->month > 12 ||
        v->day < 1 || v->day > 31) io->ok = false;
}
static void io_profile(struct ck_io *io, struct ruler_profile *p)
{
    io_size(io, &p->trait_count, RULER_TRAIT_LIMIT);
    if (!io->ok) return;
    for (size_t i = 0; i < p->trait_count && io->ok; ++i)
        io_string(io, p->traits[i], sizeof p->traits[i]);
    io_size(io, &p->claim_count, RULER_CLAIM_LIMIT);
    if (!io->ok) return;
    for (size_t i = 0; i < p->claim_count && io->ok; ++i) {
        STR(io, &p->claims[i], title);
        BOOL(io, &p->claims[i], weak);
    }
    for (size_t i = 0; i < RULER_RESOURCE_COUNT; ++i) io_double(io, &p->resources[i]);
    U32(io, p, known_resources);
    for (size_t i = 0; i < RULER_STAT_COUNT; ++i) io_s32(io, &p->stats[i]);
    U32(io, p, known_stats);
    U32(io, p, father); U32(io, p, mother); U32(io, p, spouse);
    BOOL(io, p, deceased);
    U32(io, p, id); U32(io, p, dynasty_id); U32(io, p, age); U32(io, p, religion_icon);
    STR(io, p, name); STR(io, p, culture_key); STR(io, p, religion_key);
    STR(io, p, dynasty); STR(io, p, government_key);
    STR(io, p, culture); STR(io, p, religion); STR(io, p, dna); STR(io, p, properties);
    BOOL(io, p, dynasty_titles); BOOL(io, p, female); BOOL(io, p, age_known);
    BOOL(io, p, government_inferred);
    if (!p->id || p->known_resources >> RULER_RESOURCE_COUNT ||
        p->known_stats >> RULER_STAT_COUNT) io->ok = false;
}
static void io_campaign(struct ck_io *io, struct campaign *p)
{
    DATE(io, p, date);
    U32(io, p, treasury); U32(io, p, improvements);
    DBL(io, p, gold_remainder); DBL(io, p, monthly_tax);
    DBL(io, p, prestige); DBL(io, p, piety);
    DBL(io, p, monthly_prestige); DBL(io, p, monthly_piety);
    io_size(io, &p->demesne_count, CKPT_MAX_TITLES);
    U32(io, p, demesne_limit); BOOL(io, p, demesne_limit_known);
    BOOL(io, p, monthly_tax_known);
    if (p->date.year < 1 || !isfinite(p->monthly_tax)) io->ok = false;
}
static void io_family(struct ck_io *io, struct family_gameplay *f)
{
    io_size(io, &f->count, CKPT_MAX_PERSONS);
    U32(io, f, next_id); U32(io, f, births);
    U32(io, f, marriage_min_male); U32(io, f, marriage_min_female);
    U32(io, f, max_birth_age); U32(io, f, post_birth_wait_months);
    if (!io->ok) return;
    if (!io->writing && f->count) {
        f->people = calloc(f->count, sizeof *f->people);
        if (!f->people) { io->ok = false; return; }
        f->capacity = f->count;
    }
    for (size_t i = 0; i < f->count && io->ok; ++i) {
        struct family_runtime_person *p = f->people + i;
        io_profile(io, &p->profile);
        DATE(io, p, born); U32(io, p, pregnancy_father);
        DATE(io, p, due); BOOL(io, p, has_due_date);
        BOOL(io, p, matrilineal_marriage);
        DATE(io, p, last_birth); DATE(io, p, death_date);
        BOOL(io, p, has_death_date);
        if (p->has_due_date && !p->due.year) io->ok = false;
        if (p->has_death_date && !p->death_date.year) io->ok = false;
        if (!io->writing) for (size_t j = 0; j < i; ++j)
            if (p->profile.id == f->people[j].profile.id) io->ok = false;
    }
}
static int compare_ckpt_id(const void *left, const void *right)
{
    const unsigned a = *(const unsigned *)left, b = *(const unsigned *)right;
    return (a > b) - (a < b);
}
static void io_check_unique_ids(struct ck_io *io, const unsigned *ids, size_t count)
{
    if (!io->ok || !count) return;
    unsigned *copy = malloc(count * sizeof *copy);
    if (!copy) { io->ok = false; return; }
    memcpy(copy, ids, count * sizeof *copy);
    qsort(copy, count, sizeof *copy, compare_ckpt_id);
    for (size_t i = 1; i < count; ++i)
        if (copy[i] == copy[i-1]) { io->ok = false; break; }
    free(copy);
}
static void io_conversion(struct ck_io *io, struct conversion_state *s)
{
    io_size(io, &s->province_count, CKPT_MAX_CONVERSIONS);
    io_size(io, &s->person_count, CKPT_MAX_CONVERSIONS);
    U32(io, s, revision);
    if (!io->ok) return;
    if (!io->writing) {
        if (s->province_count) s->provinces = calloc(s->province_count, sizeof *s->provinces);
        if (s->person_count) s->people = calloc(s->person_count, sizeof *s->people);
        if ((s->province_count && !s->provinces) || (s->person_count && !s->people)) {
            io->ok = false; return;
        }
        s->province_capacity = s->province_count;
        s->person_capacity = s->person_count;
    }
    for (size_t i = 0; i < s->province_count && io->ok; ++i) {
        struct conversion_province *p = s->provinces + i;
        U32(io, p, id); STR(io, p, religion); STR(io, p, culture);
        BOOL(io, p, has_religion); BOOL(io, p, has_culture);
        if (!p->id || (p->has_religion && !p->religion[0]) ||
            (p->has_culture && !p->culture[0])) io->ok = false;
    }
    for (size_t i = 0; i < s->person_count && io->ok; ++i) {
        struct conversion_person *p = s->people + i;
        U32(io, p, id); STR(io, p, religion); STR(io, p, culture);
        BOOL(io, p, has_religion); BOOL(io, p, has_culture);
        if (!p->id || (p->has_religion && !p->religion[0]) ||
            (p->has_culture && !p->culture[0])) io->ok = false;
    }
    /* Validate duplicates in O(n log n), not a potentially quadratic scan
     * of an untrusted checkpoint's maximum allowed conversion count. */
    const size_t max_count = s->province_count > s->person_count ? s->province_count : s->person_count;
    unsigned *ids = max_count && io->ok ? calloc(max_count, sizeof *ids) : NULL;
    if (max_count && io->ok && !ids) io->ok = false;
    if (ids) {
        for (size_t i = 0; i < s->province_count; ++i) ids[i] = s->provinces[i].id;
        io_check_unique_ids(io, ids, s->province_count);
        for (size_t i = 0; i < s->person_count; ++i) ids[i] = s->people[i].id;
        io_check_unique_ids(io, ids, s->person_count);
        free(ids);
    }
    for (size_t i = 0; i < 2 && io->ok; ++i) {
        struct conversion_mission *m = &s->missions[i];
        unsigned kind = io->writing ? (unsigned)m->kind : 0;
        io_u32(io, &kind);
        if (!io->writing) m->kind = (enum conversion_kind)kind;
        U32(io, m, province_id); U32(io, m, actor_id);
        STR(io, m, target); BOOL(io, m, active);
        if (kind > 1 || (m->active && (!m->province_id || !m->target[0]))) io->ok = false;
    }
}
static void io_warfare(struct ck_io *io, struct warfare_state *s)
{
    io_size(io, &s->war_count, WARFARE_MAX_WARS);
    io_size(io, &s->army_count, WARFARE_MAX_ARMIES);
    io_size(io, &s->occupation_count, WARFARE_MAX_OCCUPATIONS);
    io_size(io, &s->levy_source_count, WARFARE_MAX_OCCUPATIONS);
    U32(io, s, next_army_id);
    if (!io->ok) return;
    for (size_t i = 0; i < s->war_count && io->ok; ++i) {
        U32(io, &s->wars[i], attacker); U32(io, &s->wars[i], defender);
        BOOL(io, &s->wars[i], active);
        if (!s->wars[i].attacker || !s->wars[i].defender ||
            s->wars[i].attacker == s->wars[i].defender) io->ok = false;
        for (size_t j = 0; j < i; ++j)
            if ((s->wars[i].attacker == s->wars[j].attacker &&
                 s->wars[i].defender == s->wars[j].defender) ||
                (s->wars[i].attacker == s->wars[j].defender &&
                 s->wars[i].defender == s->wars[j].attacker)) io->ok = false;
    }
    for (size_t i = 0; i < s->army_count && io->ok; ++i) {
        struct warfare_army *a = s->armies + i;
        U32(io, a, id); U32(io, a, ruler_id); U32(io, a, province_id);
        U32(io, a, troops); U32(io, a, origin); U32(io, a, target);
        io_size(io, &a->route_count, CKPT_MAX_TITLES);
        io_size(io, &a->route_step, CKPT_MAX_TITLES);
        U32(io, a, travel_days); U32(io, a, siege_days);
        if (!a->id || !a->ruler_id || !a->province_id ||
            a->route_step > a->route_count) io->ok = false;
        if (!io->ok) return;
        if (!io->writing && a->route_count) {
            a->route = calloc(a->route_count, sizeof *a->route);
            if (!a->route) { io->ok = false; return; }
        }
        if (a->route_count && !a->route) { io->ok = false; return; }
        for (size_t j = 0; j < a->route_count && io->ok; ++j) {
            io_u32(io, &a->route[j]);
            if (!a->route[j]) io->ok = false;
        }
        if (a->route_count && (a->route_step >= a->route_count ||
            a->route[a->route_step] != a->province_id ||
            a->route[a->route_count - 1] != a->target || !a->travel_days)) io->ok = false;
        if (!io->writing) for (size_t j = 0; j < i; ++j)
            if (a->id == s->armies[j].id) io->ok = false;
    }
    for (size_t i = 0; i < s->occupation_count && io->ok; ++i) {
        struct warfare_occupation *p = s->occupations + i;
        U32(io, p, province_id); U32(io, p, occupier_id); U32(io, p, defender_id);
        if (!p->province_id || !p->occupier_id) io->ok = false;
    }
    for (size_t i = 0; i < s->levy_source_count && io->ok; ++i) {
        U32(io, &s->levy_sources[i], ruler_id);
        U32(io, &s->levy_sources[i], province_id);
        if (!s->levy_sources[i].ruler_id || !s->levy_sources[i].province_id) io->ok = false;
    }
}
static void io_laws(struct ck_io *io, struct ck2_law_state *s)
{
    io_size(io, &s->count, CKPT_MAX_LAWS);
    if (!io->ok) return;
    if (!io->writing && s->count) {
        s->chosen = calloc(s->count, sizeof *s->chosen);
        if (!s->chosen) { io->ok = false; return; }
        s->capacity = s->count;
    }
    for (size_t i = 0; i < s->count && io->ok; ++i) {
        STR(io, &s->chosen[i], group); STR(io, &s->chosen[i], key);
        if (!s->chosen[i].group[0] || !s->chosen[i].key[0]) io->ok = false;
        for (size_t j = 0; j < i; ++j)
            if (!strcmp(s->chosen[i].group, s->chosen[j].group)) io->ok = false;
    }
}
static void io_holding(struct ck_io *io, struct holding *h)
{
    STR(io, h, id); STR(io, h, name);
    unsigned type = io->writing ? (unsigned)h->type : 0;
    io_u32(io, &type);
    if (!io->writing) h->type = (enum holding_type)type;
    io_s32(io, &h->effective);
    io_size(io, &h->building_count, HOLDING_BUILDING_LIMIT);
    if (!io->ok || type <= HOLDING_EMPTY || type >= HOLDING_TYPE_COUNT) { io->ok = false; return; }
    for (size_t i = 0; i < h->building_count && io->ok; ++i)
        io_string(io, h->buildings[i], sizeof h->buildings[i]);
}
static void io_construction(struct ck_io *io, struct construction_checkpoint *s)
{
    io_size(io, &s->building_count, CKPT_MAX_CONSTRUCTION);
    io_size(io, &s->settlement_count, CKPT_MAX_CONSTRUCTION);
    if (!io->ok) return;
    if (!io->writing) {
        if (s->building_count) s->buildings = calloc(s->building_count, sizeof *s->buildings);
        if (s->settlement_count) s->settlements = calloc(s->settlement_count, sizeof *s->settlements);
        if ((s->building_count && !s->buildings) ||
            (s->settlement_count && !s->settlements)) { io->ok = false; return; }
    }
    for (size_t i = 0; i < s->building_count && io->ok; ++i) {
        STR(io, &s->buildings[i], holding); STR(io, &s->buildings[i], building);
        U32(io, &s->buildings[i], days);
    }
    for (size_t i = 0; i < s->settlement_count && io->ok; ++i) {
        U32(io, &s->settlements[i], province);
        U32(io, &s->settlements[i], days);
        io_holding(io, &s->settlements[i].holding);
    }
}
static void io_titles(struct ck_io *io, struct openck2_checkpoint *c)
{
    io_size(io, &c->title_count, CKPT_MAX_TITLES);
    if (!io->ok) return;
    if (!io->writing && c->title_count) {
        c->titles = calloc(c->title_count, sizeof *c->titles);
        if (!c->titles) { io->ok = false; return; }
    }
    for (size_t i = 0; i < c->title_count && io->ok; ++i) {
        STR(io, &c->titles[i], id); U32(io, &c->titles[i], owner);
        if (!c->titles[i].id[0] ||
            (i && strcmp(c->titles[i-1].id, c->titles[i].id) >= 0)) io->ok = false;
    }
}
/* Version 2 appends bounded runtime government mechanics. Earlier v1
 * checkpoints remain readable and initialize these records to empty. */
static void io_government_mechanics(struct ck_io *io, struct ck2_government_mechanics *s)
{
    io_size(io,&s->clan_count,CK2_CLAN_LIMIT);
    io_size(io,&s->family_count,CK2_PATRICIAN_LIMIT);
    io_size(io,&s->post_count,CK2_TRADE_POST_LIMIT);
    io_size(io,&s->bishop_count,CK2_BISHOP_LIMIT);
    if (!io->ok) return;
    for (size_t i=0;i<s->clan_count && io->ok;++i) {
        struct ck2_nomad_clan *c=s->clans+i;
        U32(io,c,head); U32(io,c,provinces);
        DBL(io,c,population); DBL(io,c,capacity); DBL(io,c,manpower);
    }
    for (size_t i=0;i<s->family_count && io->ok;++i) {
        U32(io,&s->families[i],head);
        DBL(io,&s->families[i],campaign_fund);
    }
    for (size_t i=0;i<s->post_count && io->ok;++i) {
        struct ck2_trade_post *p=s->posts+i;
        U32(io,p,province); U32(io,p,patrician);
        io_size(io,&p->completed_count,CK2_TRADE_BUILDING_LIMIT);
        if (!io->ok) return;
        for (size_t k=0;k<p->completed_count && io->ok;++k)
            io_string(io,p->completed[k],sizeof p->completed[k]);
        STR(io,p,current_build); U32(io,p,days_left);
    }
    for (size_t i=0;i<s->bishop_count && io->ok;++i) {
        struct ck2_bishop_nomination *b=s->bishops+i;
        STR(io,b,temple); U32(io,b,liege); U32(io,b,candidate);
        BOOL(io,b,papal_approval_required); BOOL(io,b,approved);
    }
    if (io->ok && !ck2_government_mechanics_valid(s)) io->ok=false;
}

static void io_checkpoint(struct ck_io *io, struct openck2_checkpoint *c, unsigned version)
{
    DATE(io, c, bookmark);
    U32(io, c, bookmark_ruler); U32(io, c, player); U32(io, c, home_province);
    io_campaign(io, &c->campaign);
    io_family(io, &c->family);
    io_conversion(io, &c->conversion);
    io_warfare(io, &c->warfare);
    io_laws(io, &c->laws);
    io_construction(io, &c->construction);
    io_titles(io, c);
    if (version>=2) io_government_mechanics(io,&c->governments);
    if (!c->bookmark.year || !c->bookmark_ruler || !c->player) io->ok = false;
}
void checkpoint_free(struct openck2_checkpoint *c)
{
    if (!c) return;
    family_gameplay_free(&c->family);
    conversion_state_free(&c->conversion);
    warfare_state_free(&c->warfare);
    ck2_law_state_free(&c->laws);
    construction_checkpoint_free(&c->construction);
    free(c->titles);
    *c = (struct openck2_checkpoint){0};
}
static bool checkpoint_stream(struct ck_io *io, struct openck2_checkpoint *c)
{
    char magic[8];
    if (io->writing) memcpy(magic, ckpt_magic, sizeof magic);
    if (!io_bytes(io, magic, sizeof magic) || memcmp(magic, ckpt_magic, sizeof magic))
        return io->ok = false;
    unsigned version = CKPT_VERSION;
    io_u32(io, &version);
    if (version != 1 && version != CKPT_VERSION) return io->ok = false;
    io_checkpoint(io, c, version);
    if (!io->ok) return false;
    const uint64_t computed = io->hash;
    uint64_t stored = computed;
    /* The final checksum is not part of the checksummed data. */
    unsigned char b[8];
    if (io->writing) for (unsigned i = 0; i < 8; ++i) b[i] = (unsigned char)(stored >> (8 * i));
    if ((io->writing ? fwrite(b, 1, 8, io->stream) : fread(b, 1, 8, io->stream)) != 8)
        return io->ok = false;
    if (!io->writing) {
        stored = 0;
        for (unsigned i = 0; i < 8; ++i) stored |= (uint64_t)b[i] << (8 * i);
        if (stored != computed || fgetc(io->stream) != EOF) return io->ok = false;
    }
    return io->ok;
}

bool checkpoint_write(const char *path, const struct openck2_checkpoint *source)
{
    if (!path || !*path || !source) { snprintf(failure, sizeof failure, "Invalid checkpoint path/state"); return false; }
    const size_t length = strlen(path);
    if (length > 1024) { snprintf(failure, sizeof failure, "Checkpoint path too long"); return false; }
    char temp[1032];
    snprintf(temp, sizeof temp, "%s.tmp", path);
    FILE *file = fopen(temp, "wb");
    if (!file) { snprintf(failure, sizeof failure, "Cannot create checkpoint: %s", strerror(errno)); return false; }
    struct ck_io io = {.stream = file, .writing = true, .ok = true,
                       .hash = UINT64_C(14695981039346656037)};
    /* Writer never mutates the caller's data, but the encoder operates on a
     * local value-copy of pointer-bearing state to share the read code. */
    struct openck2_checkpoint view = *source;
    bool success = checkpoint_stream(&io, &view);
    if (fflush(file) || ferror(file)) success = false;
    if (fclose(file)) success = false;
    if (success && rename(temp, path)) success = false;
    if (!success) {
        remove(temp);
        snprintf(failure, sizeof failure, "Cannot write checkpoint (invalid data, I/O or rename)");
    } else failure[0] = 0;
    return success;
}

bool checkpoint_read(const char *path, struct openck2_checkpoint *out)
{
    if (!path || !*path || !out) { snprintf(failure, sizeof failure, "Invalid checkpoint path/target"); return false; }
    FILE *file = fopen(path, "rb");
    if (!file) { snprintf(failure, sizeof failure, "Cannot open checkpoint: %s", strerror(errno)); return false; }
    struct openck2_checkpoint result = {0};
    struct ck_io io = {.stream = file, .writing = false, .ok = true,
                       .hash = UINT64_C(14695981039346656037)};
    const bool success = checkpoint_stream(&io, &result);
    fclose(file);
    if (!success) {
        checkpoint_free(&result);
        snprintf(failure, sizeof failure, "Invalid, truncated, corrupt or unsupported checkpoint");
        return false;
    }
    *out = result;
    failure[0] = 0;
    return true;
}
