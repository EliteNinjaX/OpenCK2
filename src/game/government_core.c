#include "government_core.h"
#include "ck2_lexer_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uint32_t government_holding_mask(const char *key)
{
    static const struct { const char *name; uint32_t bit; } names[] = {
        {"CASTLE", GOVERNMENT_CASTLE}, {"CITY", GOVERNMENT_CITY},
        {"TEMPLE", GOVERNMENT_TEMPLE}, {"TRIBAL", GOVERNMENT_TRIBAL},
        {"NOMAD", GOVERNMENT_NOMAD}, {"FORT", GOVERNMENT_FORT},
        {"HOSPITAL", GOVERNMENT_HOSPITAL}, {"TRADE_POST", GOVERNMENT_TRADE_POST},
        {"FAMILY_PALACE", GOVERNMENT_FAMILY_PALACE}
    };
    if (!key) return 0;
    for (size_t i = 0; i < sizeof names / sizeof names[0]; ++i)
        if (!strcmp(key, names[i].name)) return names[i].bit;
    return 0;
}

static char *copy_source(const char *start, const char *end)
{
    if (!start || !end || end < start) return NULL;
    const size_t n = (size_t)(end - start);
    char *result = malloc(n + 1);
    if (result) { memcpy(result, start, n); result[n] = 0; }
    return result;
}

static uint32_t holdings(struct ck2_lexer *lex)
{
    uint32_t bits = 0;
    struct ck2_span t;
    while ((t = ck2_lex(lex)).length && !ck2_span_eq(t, "}")) {
        char name[64];
        ck2_span_copy(name, sizeof name, t);
        bits |= government_holding_mask(name);
    }
    return bits;
}

bool government_key_set_has(const struct government_key_set *set, const char *id)
{
    if (!set || !id || !*id) return false;
    for (size_t i = 0; i < set->count; ++i)
        if (!strcmp(set->ids[i], id)) return true;
    return false;
}

static bool government_read_ids(struct ck2_lexer *lex, struct government_key_set *set)
{
    struct ck2_span t;
    while ((t = ck2_lex(lex)).length && !ck2_span_eq(t, "}")) {
        char id[80];
        if (t.length >= sizeof id || ck2_span_eq(t, "{") || ck2_span_eq(t, "=")) return false;
        ck2_span_copy(id, sizeof id, t);
        if (government_key_set_has(set, id)) continue;
        if (set->count == sizeof set->ids / sizeof set->ids[0]) return false;
        strcpy(set->ids[set->count++], id);
    }
    return ck2_span_eq(t, "}");
}

static uint32_t read_color(struct ck2_lexer *lex, bool *valid)
{
    int channels[3] = {0};
    *valid = true;
    for (int i = 0; i < 3; ++i) {
        char number[32], *end;
        ck2_span_copy(number, sizeof number, ck2_lex(lex));
        const long val = strtol(number, &end, 10);
        if (!number[0] || *end || val < 0 || val > 255) *valid = false;
        channels[i] = (int)val;
    }
    ck2_skip_scope(lex); /* Remaining color scope, usually just closing brace. */
    return (uint32_t)(channels[0] & 255) << 16 |
           (uint32_t)(channels[1] & 255) << 8 | (uint32_t)(channels[2] & 255);
}

static bool parse_definition(struct government_definition *def, struct ck2_lexer *lex)
{
    struct ck2_span key;
    def->aggression = 1.0;
    while ((key = ck2_lex(lex)).length && !ck2_span_eq(key, "}")) {
        if (!ck2_span_eq(ck2_lex(lex), "=")) return false;
        struct ck2_span value = ck2_lex(lex);
        if (!value.start) return false;
        if (ck2_span_eq(value, "{")) {
            if (ck2_span_eq(key, "preferred_holdings")) def->preferred_holdings = holdings(lex);
            else if (ck2_span_eq(key, "allowed_holdings")) def->allowed_holdings = holdings(lex);
            else if (ck2_span_eq(key, "no_vassal_tax")) {
                if (!government_read_ids(lex, &def->no_vassal_tax)) return false;
            } else if (ck2_span_eq(key, "vassal_call_to_arms")) {
                if (!government_read_ids(lex, &def->vassal_call_to_arms)) return false;
            } else if (ck2_span_eq(key, "forced_contract")) {
                if (!government_read_ids(lex, &def->forced_contract)) return false;
            } else if (ck2_span_eq(key, "color"))
                def->color_rgb = read_color(lex, &def->color_defined);
            else if (ck2_span_eq(key, "potential")) {
                const char *first = value.start;
                ck2_skip_scope(lex);
                char *source = copy_source(first, lex->next);
                if (!source) return false;
                free(def->potential_script);
                def->potential_script = source;
                def->has_potential = true;
            } else ck2_skip_scope(lex);
        } else {
            if (ck2_span_eq(key, "frame_suffix")) ck2_span_copy(def->frame_suffix, sizeof def->frame_suffix, value);
            else if (ck2_span_eq(key, "title_prefix")) ck2_span_copy(def->title_prefix, sizeof def->title_prefix, value);
            else if (ck2_span_eq(key, "merchant_republic")) def->merchant_republic = ck2_span_eq(value, "yes");
            else if (ck2_span_eq(key, "landed_inheritance")) {
                def->landed_inheritance_defined = true;
                def->landed_inheritance = ck2_span_eq(value, "yes");
            } else if (ck2_span_eq(key, "have_gender_laws")) {
                def->have_gender_laws_defined = true;
                def->have_gender_laws = ck2_span_eq(value, "yes");
            } else if (ck2_span_eq(key, "can_create_kingdoms")) {
                def->can_create_kingdoms_defined = true;
                def->can_create_kingdoms = ck2_span_eq(value, "yes");
            } else if (ck2_span_eq(key, "can_build_forts")) {
                def->can_build_forts_defined = true;
                def->can_build_forts = ck2_span_eq(value, "yes");
            } else if (ck2_span_eq(key, "can_build_hospitals")) {
                def->can_build_hospitals_defined = true;
                def->can_build_hospitals = ck2_span_eq(value, "yes");
            } else if (ck2_span_eq(key, "allow_looting")) def->allow_looting = ck2_span_eq(value, "yes");
            else if (ck2_span_eq(key, "can_build_holdings")) {
                def->can_build_holdings_defined = true;
                def->can_build_holdings = ck2_span_eq(value, "yes");
            } else if (ck2_span_eq(key, "max_consorts")) {
                char buf[32]; ck2_span_copy(buf, sizeof buf, value);
                def->max_consorts = (unsigned)strtoul(buf, NULL, 10);
            } else if (ck2_span_eq(key, "aggression")) {
                char buf[32]; ck2_span_copy(buf, sizeof buf, value);
                def->aggression = strtod(buf, NULL);
            }
        }
    }
    return ck2_span_eq(key, "}");
}

const struct government_definition *government_find(const struct government_catalog *cat, const char *id)
{
    if (!cat || !id) return NULL;
    for (size_t i = 0; i < cat->count; ++i)
        if (!strcmp(cat->items[i].id, id)) return &cat->items[i];
    return NULL;
}

bool government_catalog_parse(struct government_catalog *cat, const char *source)
{
    if (!cat || !source) return false;
    struct ck2_lexer lex = {source};
    struct ck2_span group;
    while ((group = ck2_lex(&lex)).length) {
        if (!ck2_span_eq(ck2_lex(&lex), "=") || !ck2_span_eq(ck2_lex(&lex), "{")) return false;
        struct ck2_span id;
        while ((id = ck2_lex(&lex)).length && !ck2_span_eq(id, "}")) {
            if (!ck2_span_eq(ck2_lex(&lex), "=") || !ck2_span_eq(ck2_lex(&lex), "{")) return false;
            struct government_definition next = {0};
            ck2_span_copy(next.id, sizeof next.id, id);
            ck2_span_copy(next.group, sizeof next.group, group);
            if (!parse_definition(&next, &lex)) { free(next.potential_script); return false; }
            const struct government_definition *existing = government_find(cat, next.id);
            if (existing) {
                struct government_definition *dest = &cat->items[existing - cat->items];
                free(dest->potential_script);
                *dest = next;
            } else {
                if (cat->count == 256) { free(next.potential_script); return false; }
                if (cat->count == cat->capacity) {
                    size_t capacity = cat->capacity ? cat->capacity * 2 : 16;
                    struct government_definition *items = realloc(cat->items, capacity * sizeof *items);
                    if (!items) { free(next.potential_script); return false; }
                    cat->items = items;
                    cat->capacity = capacity;
                }
                cat->items[cat->count++] = next;
            }
        }
        if (!ck2_span_eq(id, "}")) return false;
    }
    return true;
}

size_t government_holding_candidates(const struct government_catalog *cat, uint32_t bit,
                                     const struct government_definition **out, size_t capacity)
{
    size_t found = 0;
    if (!cat || !bit) return 0;
    for (size_t i = 0; i < cat->count; ++i)
        if (cat->items[i].preferred_holdings & bit) {
            if (out && found < capacity) out[found] = &cat->items[i];
            ++found;
        }
    return found;
}

void government_catalog_destroy(struct government_catalog *cat)
{
    if (!cat) return;
    for (size_t i = 0; i < cat->count; ++i) free(cat->items[i].potential_script);
    free(cat->items);
    *cat = (struct government_catalog){0};
}

/* CK2's imperial potential is not a holding preference. The following scoped
 * evaluator covers exactly the logical/title/date/law atoms needed to decide
 * the authored Roman imperial trigger; everything else stays unresolved. */
static enum government_truth invert_truth(enum government_truth value)
{
    return value == GOVERNMENT_UNRESOLVED ? value :
        (value == GOVERNMENT_TRUE ? GOVERNMENT_FALSE : GOVERNMENT_TRUE);
}
static enum government_truth combine_truth(enum government_truth acc,
                                           enum government_truth next, bool any)
{
    if (any) {
        if (acc == GOVERNMENT_TRUE || next == GOVERNMENT_TRUE) return GOVERNMENT_TRUE;
        return acc == GOVERNMENT_UNRESOLVED || next == GOVERNMENT_UNRESOLVED ?
            GOVERNMENT_UNRESOLVED : GOVERNMENT_FALSE;
    }
    if (acc == GOVERNMENT_FALSE || next == GOVERNMENT_FALSE) return GOVERNMENT_FALSE;
    return acc == GOVERNMENT_UNRESOLVED || next == GOVERNMENT_UNRESOLVED ?
        GOVERNMENT_UNRESOLVED : GOVERNMENT_TRUE;
}
static enum government_truth eval_scope(struct ck2_lexer *lex,
                                        const struct government_context *ctx,
                                        bool any, unsigned depth)
{
    if (depth > 32) { ck2_skip_scope(lex); return GOVERNMENT_UNRESOLVED; }
    enum government_truth total = any ? GOVERNMENT_FALSE : GOVERNMENT_TRUE;
    struct ck2_span key;
    while ((key = ck2_lex(lex)).length && !ck2_span_eq(key, "}")) {
        if (!ck2_span_eq(ck2_lex(lex), "=")) return GOVERNMENT_UNRESOLVED;
        struct ck2_span value = ck2_lex(lex);
        if (!value.length) return GOVERNMENT_UNRESOLVED;
        enum government_truth result = GOVERNMENT_UNRESOLVED;
        if (ck2_span_eq(value, "{")) {
            if (ck2_span_eq(key, "OR")) result = eval_scope(lex, ctx, true, depth + 1);
            else if (ck2_span_eq(key, "AND") || ck2_span_eq(key, "primary_title"))
                result = eval_scope(lex, ctx, false, depth + 1);
            else if (ck2_span_eq(key, "NOT") || ck2_span_eq(key, "NAND"))
                result = invert_truth(eval_scope(lex, ctx,
                    ck2_span_eq(key, "NOT"), depth + 1));
            else ck2_skip_scope(lex);
        } else if (ck2_span_eq(key, "title")) {
            if (ctx->primary_title)
                result = strlen(ctx->primary_title) == value.length &&
                    !memcmp(ctx->primary_title, value.start, value.length) ?
                    GOVERNMENT_TRUE : GOVERNMENT_FALSE;
        } else if (ck2_span_eq(key, "has_law")) {
            if (ctx->succession_law)
                result = strlen(ctx->succession_law) == value.length &&
                    !memcmp(ctx->succession_law, value.start, value.length) ?
                    GOVERNMENT_TRUE : GOVERNMENT_FALSE;
        } else if (ck2_span_eq(key, "is_save_game") ||
                   ck2_span_eq(key, "has_game_started") ||
                   ck2_span_eq(key, "is_patrician")) {
            const bool actual = ck2_span_eq(key, "is_save_game") ? ctx->save_game :
                ck2_span_eq(key, "has_game_started") ? ctx->game_started : ctx->is_patrician;
            if (ck2_span_eq(value, "yes") || ck2_span_eq(value, "no"))
                result = actual == ck2_span_eq(value, "yes") ? GOVERNMENT_TRUE : GOVERNMENT_FALSE;
        }
        total = combine_truth(total, result, any);
    }
    return ck2_span_eq(key, "}") ? total : GOVERNMENT_UNRESOLVED;
}

enum government_truth government_evaluate_potential(
    const struct government_definition *def, const struct government_context *ctx)
{
    if (!def || !ctx || !def->potential_script) return GOVERNMENT_UNRESOLVED;
    struct ck2_lexer lex = {def->potential_script};
    if (!ck2_span_eq(ck2_lex(&lex), "{")) return GOVERNMENT_UNRESOLVED;
    return eval_scope(&lex, ctx, false, 0);
}

size_t government_potential_titles(const struct government_definition *def,
                                   char (*titles)[80], size_t capacity)
{
    if (!def || !def->potential_script) return 0;
    struct ck2_lexer lex = {def->potential_script};
    struct ck2_span key;
    size_t count = 0;
    while ((key = ck2_lex(&lex)).length) {
        if (!ck2_span_eq(key, "title")) continue;
        if (!ck2_span_eq(ck2_lex(&lex), "=")) continue;
        struct ck2_span value = ck2_lex(&lex);
        if (!value.length || ck2_span_eq(value, "{")) continue;
        char id[80]; ck2_span_copy(id, sizeof id, value);
        bool seen = false;
        for (size_t k = 0; k < count && k < capacity; ++k)
            if (!strcmp(titles[k], id)) { seen = true; break; }
        if (!seen) {
            if (titles && count < capacity) strcpy(titles[count], id);
            ++count;
        }
    }
    return count;
}

const struct government_definition *government_resolve_special(
    const struct government_catalog *catalog, const char *authored_key,
    const struct government_context *ctx, bool *inferred)
{
    if (inferred) *inferred = false;
    if (!catalog) return NULL;
    if (authored_key && *authored_key) return government_find(catalog, authored_key);
    /* The other CK2 governments have complex potential scripts. Do not infer
     * them from castle ownership or from a generic feudal/government color. */
    const struct government_definition *roman = government_find(catalog, "roman_imperial_government");
    if (government_evaluate_potential(roman, ctx) != GOVERNMENT_TRUE) return NULL;
    if (inferred) *inferred = true;
    return roman;
}

const struct government_definition *government_provisional_holding(
    const struct government_catalog *cat, uint32_t verified_holding)
{
    const char *id = NULL;
    if (verified_holding == GOVERNMENT_NOMAD) id = "nomadic_government";
    if (verified_holding == GOVERNMENT_CITY) id = "republic_government";
    if (verified_holding == GOVERNMENT_TEMPLE) id = "theocracy_government";
    const struct government_definition *def = government_find(cat, id);
    /* In particular, do NOT assume every city holder is a merchant republic. */
    return def && (def->preferred_holdings & verified_holding) ? def : NULL;
}

bool government_vassal_tax_exempt(const struct government_definition *liege,
                                 const struct government_definition *vassal)
{
    return liege && vassal && government_key_set_has(&liege->no_vassal_tax, vassal->id);
}

bool government_uses_hereditary_succession(const struct government_definition *def)
{
    if (!def) return true; /* Cannot invent missing restrictions. */
    if (def->landed_inheritance_defined && !def->landed_inheritance) return false;
    /* CK2 temple incumbency is not a hereditary title succession path. */
    if (!strcmp(def->id, "theocracy_government")) return false;
    return true;
}

bool government_can_found_holding(const struct government_definition *def)
{
    return !def || !def->can_build_holdings_defined || def->can_build_holdings;
}

uint32_t government_verified_holding(const struct government_holding_evidence *evidence,
    size_t count, unsigned owner, const char *primary_title, unsigned primary_capital)
{
    if (!evidence || !owner) return 0;
    uint32_t unanimous = 0, capital = 0;
    bool conflict = false, capital_conflict = false;
    for (size_t i = 0; i < count; ++i) {
        const struct government_holding_evidence *fact = evidence + i;
        if (fact->holder_id != owner || !fact->holding_bit) continue;
        if (primary_title && !strcmp(primary_title, fact->title_id))
            return fact->holding_bit;
        if (primary_capital && fact->province_id == primary_capital) {
            if (capital && capital != fact->holding_bit) capital_conflict = true;
            else capital = fact->holding_bit;
        }
        if (!unanimous) unanimous = fact->holding_bit;
        else if (unanimous != fact->holding_bit) conflict = true;
    }
    return capital_conflict ? 0 : capital ? capital : conflict ? 0 : unanimous;
}

const struct government_religion *government_religion_find(
    const struct government_religions *index, const char *id)
{
    if (!index || !id || !*id) return NULL;
    for (size_t i=0;i<index->count;++i)
        if (!strcmp(index->entries[i].id,id)) return index->entries+i;
    return NULL;
}
void government_religions_free(struct government_religions *index)
{
    if (!index) return;
    free(index->entries);
    *index=(struct government_religions){0};
}
static bool government_religion_add(struct government_religions *index,
                                    struct government_religion member)
{
    const struct government_religion *previous=government_religion_find(index,member.id);
    if (previous) {
        index->entries[previous-index->entries]=member;
        return true;
    }
    if (index->count==index->capacity) {
        const size_t n=index->capacity ? index->capacity*2 : 64;
        if (n>2048 || n<index->capacity) return false;
        void *p=realloc(index->entries,n*sizeof *index->entries);
        if (!p) return false;
        index->entries=p; index->capacity=n;
    }
    index->entries[index->count++]=member;
    return true;
}
/* Parse a child religion body only far enough to read its temple rules. */
static bool government_group_metadata(struct ck2_span key)
{
    static const char *const meta[]={
        "color", "male_names", "female_names", "god_names", "evil_god_names",
        "graphical_cultures", "interface_skin", "religious_clothing",
        "ai_will_do", "trigger", "modifier", "character_modifier",
        "province_modifier", "reform", "portraits"
    };
    for (size_t i=0;i<sizeof meta/sizeof meta[0];++i)
        if (ck2_span_eq(key,meta[i])) return true;
    return false;
}
static bool government_religion_body(struct ck2_lexer *lex,
                                     struct government_religion *r)
{
    struct ck2_span field;
    while ((field=ck2_lex(lex)).length && !ck2_span_eq(field,"}")) {
        if (!ck2_span_eq(ck2_lex(lex),"=")) return false;
        const struct ck2_span value=ck2_lex(lex);
        if (!value.length) return false;
        if (ck2_span_eq(value,"{")) ck2_skip_scope(lex);
        else if (ck2_span_eq(field,"female_temple_holders"))
            r->female_temple=ck2_span_eq(value,"yes");
        else if (ck2_span_eq(field,"male_temple_holders"))
            r->male_temple=ck2_span_eq(value,"yes");
    }
    return ck2_span_eq(field,"}");
}
bool government_religions_parse(struct government_religions *index, const char *source)
{
    if (!index || !source) return false;
    struct ck2_lexer lex={source};
    struct ck2_span group;
    while ((group=ck2_lex(&lex)).length) {
        if (!ck2_span_eq(ck2_lex(&lex),"=")) return false;
        const struct ck2_span value=ck2_lex(&lex);
        if (!value.length) return false;
        if (!ck2_span_eq(value,"{")) continue;
        /* Default temple eligibility from original CK2: males normally yes,
         * females normally no unless religion/group overrides. */
        struct government_religion defaults={.male_temple=true};
        ck2_span_copy(defaults.group,sizeof defaults.group,group);
        struct ck2_span key;
        while ((key=ck2_lex(&lex)).length && !ck2_span_eq(key,"}")) {
            if (!ck2_span_eq(ck2_lex(&lex),"=")) return false;
            const struct ck2_span val=ck2_lex(&lex);
            if (!val.length) return false;
            if (ck2_span_eq(val,"{") && government_group_metadata(key)) {
                ck2_skip_scope(&lex);
            } else if (ck2_span_eq(val,"{")) {
                /* Only nested blocks under a group are religion candidates.
                 * Group-level color/name lists aren't character faiths and
                 * aren't queried by those IDs in practice. */
                struct government_religion member=defaults;
                ck2_span_copy(member.id,sizeof member.id,key);
                if (!government_religion_body(&lex,&member) ||
                    !government_religion_add(index,member)) return false;
            } else if (ck2_span_eq(key,"female_temple_holders"))
                defaults.female_temple=ck2_span_eq(val,"yes");
            else if (ck2_span_eq(key,"male_temple_holders"))
                defaults.male_temple=ck2_span_eq(val,"yes");
        }
        if (!ck2_span_eq(key,"}")) return false;
    }
    return true;
}

bool government_allows_law_group(const struct government_definition *def, const char *group)
{
    if (!def || !group) return true;
    if (!strcmp(group,"gender_succession") &&
        def->have_gender_laws_defined && !def->have_gender_laws) return false;
    /* Do not let a developer law override turn a merchant election or a
     * temple appointment into unsupported hereditary title succession. */
    if (!strcmp(group,"succession") &&
        (def->merchant_republic || !strcmp(def->id,"theocracy_government"))) return false;
    return true;
}
