#include "law_core.h"
#include "ck2_lexer_core.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

static bool tok(struct ck2_span s, const char *t) { return ck2_span_eq(s,t); }
static void copy(char *s, size_t n, struct ck2_span v) { ck2_span_copy(s,n,v); }
static bool number(struct ck2_span t, double *v)
{
    char buf[72], *end;
    if (!t.length || t.length >= sizeof buf) return false;
    copy(buf,sizeof buf,t);
    errno=0;
    double value=strtod(buf,&end);
    if (errno || *end || !isfinite(value)) return false;
    *v=value;
    return true;
}
static bool next_pair(struct ck2_lexer *lex, struct ck2_span *key, struct ck2_span *value)
{
    *key=ck2_lex(lex);
    if (!key->length || tok(*key,"}")) return false;
    if (!tok(ck2_lex(lex),"=")) return false;
    *value=ck2_lex(lex);
    return value->length != 0;
}
static bool append(struct ck2_law_catalog *catalog, struct ck2_law *law)
{
    if (ck2_law_find(catalog,law->key)) return true;
    if (catalog->count == catalog->capacity) {
        const size_t next=catalog->capacity ? catalog->capacity*2 : 64;
        if (next<catalog->capacity || next>SIZE_MAX/sizeof(*catalog->items)) return false;
        void *p=realloc(catalog->items,next*sizeof(*catalog->items));
        if (!p) return false;
        catalog->items=p; catalog->capacity=next;
    }
    catalog->items[catalog->count++]=*law;
    return true;
}
static void read_field(struct ck2_law *law, struct ck2_span key, struct ck2_span value)
{
    double n;
    if (tok(key,"group")) copy(law->group,sizeof law->group,value);
    else if (tok(key,"default")) law->default_law=tok(value,"yes");
    else if (tok(key,"succession")) copy(law->succession,sizeof law->succession,value);
    else if (tok(key,"gender_succ")) copy(law->gender,sizeof law->gender,value);
    else if (number(value,&n)) {
#define LAW_FIELD(name,field,known) if(tok(key,name)) {law->field=n; law->known=true;return;}
        LAW_FIELD("demesne_size",demesne_size,has_demesne_size)
        LAW_FIELD("vassal_limit",vassal_limit,has_vassal_limit)
        LAW_FIELD("castle_vassal_tax_modifier",castle_tax,has_castle_tax)
        LAW_FIELD("city_vassal_tax_modifier",city_tax,has_city_tax)
        LAW_FIELD("temple_vassal_tax_modifier",temple_tax,has_temple_tax)
        LAW_FIELD("tribal_vassal_tax_modifier",tribal_tax,has_tribal_tax)
        LAW_FIELD("castle_vassal_min_levy",castle_min_levy,has_castle_levy)
        LAW_FIELD("city_vassal_min_levy",city_min_levy,has_city_levy)
        LAW_FIELD("temple_vassal_min_levy",temple_min_levy,has_temple_levy)
        LAW_FIELD("tribal_vassal_min_levy",tribal_min_levy,has_tribal_levy)
#undef LAW_FIELD
    }
}
/* CK2's succession voting rules are in effect={succession={base_type=...
 * voting_rules=...}}, not direct law fields. Read *only* that exact static
 * assignment; do not infer triggers or execute arbitrary effect scopes. */
static void read_succession_voting(struct ck2_law *law,struct ck2_lexer *lex)
{
    struct ck2_span k,v;
    while (next_pair(lex,&k,&v)) {
        if (tok(v,"{")) {ck2_skip_scope(lex);continue;}
        if (tok(k,"base_type")) copy(law->election_base_type,sizeof law->election_base_type,v);
        if (tok(k,"voting_rules")) copy(law->election_voting_rules,sizeof law->election_voting_rules,v);
    }
}
static void read_law_effect(struct ck2_law *law,struct ck2_lexer *lex)
{
    struct ck2_span k,v;
    while (next_pair(lex,&k,&v)) {
        if (tok(k,"succession") && tok(v,"{"))
            read_succession_voting(law,lex);
        else if (tok(v,"{")) ck2_skip_scope(lex);
    }
}
static bool parse_law_table(struct ck2_law_catalog *catalog, struct ck2_lexer *lex,
                            enum ck2_law_class kind)
{
    struct ck2_span key,value;
    while (next_pair(lex,&key,&value)) {
        if (!tok(value,"{")) continue;
        struct ck2_law law={.kind=kind};
        copy(law.key,sizeof law.key,key);
        struct ck2_span k,v;
        while (next_pair(lex,&k,&v)) {
            if (tok(v,"{")) {
                /* Inspect only the exact succession voting metadata. */
                if (kind==CK2_LAW_SUCCESSION && tok(k,"effect"))
                    read_law_effect(&law,lex);
                else ck2_skip_scope(lex);
            } else read_field(&law,k,v);
        }
        if (!law.group[0]) {
            copy(law.group,sizeof law.group,key);
            /* Default law group must be shared by all laws of the same class. */
            if (kind==CK2_LAW_SUCCESSION) strcpy(law.group,"succession");
            if (kind==CK2_LAW_GENDER) strcpy(law.group,"gender_succession");
        }
        if (!append(catalog,&law)) return false;
    }
    return true;
}
bool ck2_law_parse(struct ck2_law_catalog *catalog, const char *source)
{
    if (!catalog || !source) return false;
    struct ck2_lexer lex={source};
    struct ck2_span k,v;
    while (next_pair(&lex,&k,&v)) {
        if (!tok(v,"{")) continue;
        if (tok(k,"laws") || tok(k,"succession_laws") || tok(k,"gender_laws") ||
            tok(k,"de_jure_laws")) {
            const enum ck2_law_class kind=tok(k,"succession_laws") ? CK2_LAW_SUCCESSION :
                tok(k,"gender_laws") ? CK2_LAW_GENDER :
                tok(k,"de_jure_laws") ? CK2_LAW_CROWN : CK2_LAW_REALM;
            if (!parse_law_table(catalog,&lex,kind)) return false;
        } else ck2_skip_scope(&lex);
    }
    return true;
}
const struct ck2_law *ck2_law_find(const struct ck2_law_catalog *c, const char *key)
{
    if (!c || !key) return NULL;
    for (size_t i=0;i<c->count;++i) if (!strcmp(c->items[i].key,key)) return &c->items[i];
    return NULL;
}
void ck2_law_catalog_free(struct ck2_law_catalog *c)
{
    if (!c) return;
    free(c->items); *c=(struct ck2_law_catalog){0};
}
void ck2_law_state_free(struct ck2_law_state *s)
{
    if (!s) return;
    free(s->chosen); *s=(struct ck2_law_state){0};
}
bool ck2_law_select(struct ck2_law_state *s, const struct ck2_law_catalog *c, const char *key)
{
    const struct ck2_law *law=ck2_law_find(c,key);
    if (!s || !law) return false;
    for (size_t i=0;i<s->count;++i)
        if (!strcmp(s->chosen[i].group,law->group)) {
            strcpy(s->chosen[i].key,law->key); return true;
        }
    if (s->count==s->capacity) {
        const size_t next=s->capacity ? s->capacity*2 : 8;
        if (next<s->capacity || next>SIZE_MAX/sizeof(*s->chosen)) return false;
        void *mem=realloc(s->chosen,next*sizeof(*s->chosen));
        if (!mem) return false;
        s->chosen=mem; s->capacity=next;
    }
    struct ck2_active_law *a=&s->chosen[s->count++];
    strcpy(a->group,law->group); strcpy(a->key,law->key);
    return true;
}
const struct ck2_law *ck2_law_selected(const struct ck2_law_state *s,
                                      const struct ck2_law_catalog *c,const char *group)
{
    if (!s || !group) return NULL;
    for (size_t i=0;i<s->count;++i)
        if (!strcmp(s->chosen[i].group,group)) return ck2_law_find(c,s->chosen[i].key);
    return NULL;
}
struct group_date {char group[80]; unsigned date;};
static bool date_key(struct ck2_span t,unsigned *out)
{
    char buf[32],extra=0; unsigned year,month,day;
    if (t.length>=sizeof buf) return false;
    copy(buf,sizeof buf,t);
    if (sscanf(buf,"%u.%u.%u%c",&year,&month,&day,&extra)!=3 ||
        year>9999 || month<1 || month>12 || day<1 || day>31) return false;
    *out=year*10000+month*100+day;
    return true;
}
static bool record_law(struct ck2_law_state *state,const struct ck2_law_catalog *catalog,
                       struct group_date *groups,size_t *n,unsigned date,struct ck2_span key)
{
    char id[80];copy(id,sizeof id,key);
    const struct ck2_law *law=ck2_law_find(catalog,id);
    if (!law || strlen(id)!=key.length) return true; /* Uninstalled DLC law. */
    size_t i=0;
    for (;i<*n;++i) if (!strcmp(groups[i].group,law->group)) break;
    if (i==*n) {
        if (*n==64) return false;
        strcpy(groups[i].group,law->group);
        groups[i].date=0;
        ++*n;
    }
    if (date>=groups[i].date && ck2_law_select(state,catalog,id)) {
        groups[i].date=date;
        return true;
    }
    return date<groups[i].date;
}
bool ck2_law_title_history(struct ck2_law_state *state,const struct ck2_law_catalog *catalog,
                           const char *history,unsigned cutoff)
{
    if (!state || !catalog || !history || !cutoff) return false;
    struct group_date groups[64]={0};size_t groups_count=0;
    struct ck2_lexer lex={history};struct ck2_span k,v;
    while (next_pair(&lex,&k,&v)) {
        if (tok(k,"law") && !tok(v,"{")) {
            if (!record_law(state,catalog,groups,&groups_count,0,v)) return false;
        } else if (tok(v,"{")) {
            unsigned dated=0;
            if (date_key(k,&dated) && dated<=cutoff) {
                struct ck2_span child,value;
                while (next_pair(&lex,&child,&value)) {
                    if (tok(value,"{")) ck2_skip_scope(&lex);
                    else if (tok(child,"law") &&
                        !record_law(state,catalog,groups,&groups_count,dated,value)) return false;
                }
            } else ck2_skip_scope(&lex);
        }
    }
    return true;
}

bool ck2_law_holding_tax(const struct ck2_law_state *s, const struct ck2_law_catalog *c,
                         unsigned type, double *fraction)
{
    if (!fraction) return false;
    const char *group=NULL;
    const struct ck2_law *law;
    switch(type) {
        case 1: group=ck2_law_selected(s,c,"tax_levy_law_feudal") ? "tax_levy_law_feudal":"feudal_tax"; break;
        case 2: group=ck2_law_selected(s,c,"tax_levy_law_republic") ? "tax_levy_law_republic":"city_tax"; break;
        case 3: group=ck2_law_selected(s,c,"tax_levy_law_theocracy") ? "tax_levy_law_theocracy":"temple_tax"; break;
        case 4: group=ck2_law_selected(s,c,"tax_levy_law_tribal") ? "tax_levy_law_tribal":"tribal_tax"; break;
        default: return false;
    }
    law=ck2_law_selected(s,c,group);
    if (!law) return false;
    switch(type) {
        case 1: if (!law->has_castle_tax) return false; *fraction=law->castle_tax; break;
        case 2: if (!law->has_city_tax) return false; *fraction=law->city_tax; break;
        case 3: if (!law->has_temple_tax) return false; *fraction=law->temple_tax; break;
        case 4: if (!law->has_tribal_tax) return false; *fraction=law->tribal_tax; break;
    }
    return *fraction>=0 && *fraction<=1;
}
enum ck2_succession_mode ck2_law_succession_mode(const struct ck2_law *l)
{
    if (!l || l->kind!=CK2_LAW_SUCCESSION) return CK2_SUCC_UNSUPPORTED;
    const char *s=l->succession[0] ? l->succession : l->key;
    if (!strcmp(s,"primogeniture") || !strcmp(s,"succ_primogeniture")) return CK2_SUCC_PRIMOGENITURE;
    if (!strcmp(s,"ultimogeniture") || !strcmp(s,"succ_ultimogeniture")) return CK2_SUCC_ULTIMOGENITURE;
    /* True seniority requires ranking every eligible dynast across the map,
     * not just the historical children and siblings currently indexed. */
    if (!strcmp(s,"seniority") || !strcmp(s,"succ_seniority")) return CK2_SUCC_UNSUPPORTED;
    if (!strcmp(s,"gavelkind") || !strcmp(s,"succ_gavelkind")) return CK2_SUCC_GAVELKIND;
    return CK2_SUCC_UNSUPPORTED;
}
enum ck2_gender_mode ck2_law_gender_mode(const struct ck2_law *l)
{
    if (!l || l->kind!=CK2_LAW_GENDER) return CK2_GENDER_UNSPECIFIED;
    const char *g=l->gender[0] ? l->gender : l->key;
    if (!strcmp(g,"agnatic") || !strcmp(g,"agnatic_succession")) return CK2_GENDER_AGNATIC;
    if (!strcmp(g,"cognatic") || !strcmp(g,"cognatic_succession")) return CK2_GENDER_COGNATIC;
    if (!strcmp(g,"true_cognatic") || !strcmp(g,"true_cognatic_succession")) return CK2_GENDER_EQUAL;
    if (!strcmp(g,"enatic") || !strcmp(g,"enatic_succession")) return CK2_GENDER_ENATIC;
    if (!strcmp(g,"enatic_cognatic") || !strcmp(g,"enatic_cognatic_succession")) return CK2_GENDER_ENATIC_COGNATIC;
    return CK2_GENDER_UNSPECIFIED;
}
static bool eligible(const struct ck2_heir_candidate *c, unsigned dynasty,enum ck2_gender_mode g)
{
    if (!c->id || !c->alive || !c->age_known || (dynasty && c->dynasty_id!=dynasty)) return false;
    return (g!=CK2_GENDER_AGNATIC || !c->female) && (g!=CK2_GENDER_ENATIC || c->female);
}
/* A priority derived from gender and birth order, not a CK2 election. Age
 * granularity is currently years; same-age ties remain deterministic by ID. */
static bool better(const struct ck2_heir_candidate *a, const struct ck2_heir_candidate *b,
                   enum ck2_succession_mode mode, enum ck2_gender_mode gender)
{
    if (!b) return true;
    if (gender==CK2_GENDER_COGNATIC || gender==CK2_GENDER_ENATIC_COGNATIC) {
        const bool pref_female=gender==CK2_GENDER_ENATIC_COGNATIC;
        if (a->female!=b->female) return a->female==pref_female;
    }
    if (a->age!=b->age) return mode==CK2_SUCC_ULTIMOGENITURE ? a->age<b->age : a->age>b->age;
    return a->id<b->id;
}
static size_t select_heirs(enum ck2_succession_mode mode,enum ck2_gender_mode gender,
                           unsigned dead_id,unsigned dynasty,const struct ck2_heir_candidate *c,
                           size_t n,unsigned *result,size_t cap)
{
    if (!c || !result || !cap || !dead_id || mode==CK2_SUCC_UNSUPPORTED ||
        gender==CK2_GENDER_UNSPECIFIED) return 0;
    if (mode==CK2_SUCC_SENIORITY) {
        const struct ck2_heir_candidate *best=NULL;
        for (size_t i=0;i<n;++i)
            if (c[i].id!=dead_id && eligible(&c[i],dynasty,gender) &&
                better(&c[i],best,mode,gender)) best=&c[i];
        if (best) {result[0]=best->id;return 1;}
        return 0;
    }
    size_t count=0;
    /* The strict heir algorithm currently knows direct children only. */
    for (size_t x=0;x<cap;++x) {
        const struct ck2_heir_candidate *best=NULL;
        for (size_t i=0;i<n;++i) {
            if (c[i].father!=dead_id && c[i].mother!=dead_id) continue;
            if (!eligible(&c[i],dynasty,gender)) continue;
            bool seen=false;
            for (size_t k=0;k<count;++k) seen |= result[k]==c[i].id;
            if (!seen && better(&c[i],best,mode,gender)) best=&c[i];
        }
        if (!best) break;
        result[count++]=best->id;
        if (mode!=CK2_SUCC_GAVELKIND) break;
    }
    return count;
}
unsigned ck2_succession_heir(enum ck2_succession_mode mode, enum ck2_gender_mode gender,
                             unsigned dead_id,unsigned dynasty,const struct ck2_heir_candidate *c,size_t n)
{
    unsigned heir=0;
    return select_heirs(mode,gender,dead_id,dynasty,c,n,&heir,1) ? heir : 0;
}
size_t ck2_succession_divide(enum ck2_succession_mode mode,enum ck2_gender_mode gender,
                             unsigned dead_id,unsigned dynasty,const struct ck2_heir_candidate *c,
                             size_t n,unsigned *heirs,size_t capacity)
{
    return select_heirs(mode,gender,dead_id,dynasty,c,n,heirs,capacity);
}
