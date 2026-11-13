#include "election_core.h"
#include "ck2_lexer_core.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool pair(struct ck2_lexer *lex, struct ck2_span *key, struct ck2_span *value)
{
    *key=ck2_lex(lex);
    if (!key->length || ck2_span_eq(*key,"}")) return false;
    if (!ck2_span_eq(ck2_lex(lex),"=")) return false;
    *value=ck2_lex(lex);
    return value->length>0;
}
static bool parse_int(struct ck2_span v, int *out)
{
    char buffer[32],*end;
    if (!v.length || v.length>=sizeof buffer) return false;
    ck2_span_copy(buffer,sizeof buffer,v);
    errno=0;long n=strtol(buffer,&end,10);
    if (errno || *end || n<INT_MIN || n>INT_MAX) return false;
    *out=(int)n;
    return true;
}
static int section_id(struct ck2_span key)
{
    static const char *const keys[]={
        "elector_selection","elector_vote_strength","candidate_vote_score","candidate_trigger"
    };
    for (int i=0;i<CK2_ELECTION_SECTION_COUNT;++i)
        if (ck2_span_eq(key,keys[i])) return i;
    return -1;
}
static char *capture_scope(struct ck2_lexer *lex)
{
    /* The '{' has already been consumed. Keep original rule text and nested
     * scopes byte-for-byte, for eventual interpreted trigger evaluation. */
    const char *begin=lex->next;
    ck2_skip_scope(lex);
    const char *end=lex->next;
    if (!begin || !end || end<=begin || end[-1]!='}') return NULL;
    size_t n=(size_t)(end-begin-1);
    if (n>4*1024*1024) return NULL;
    char *script=malloc(n+1);
    if (!script) return NULL;
    memcpy(script,begin,n);script[n]=0;
    return script;
}
static void free_rule(struct ck2_election_rule *r)
{
    for (int i=0;i<CK2_ELECTION_SECTION_COUNT;++i) free(r->script[i]);
    *r=(struct ck2_election_rule){0};
}
void ck2_elections_free(struct ck2_election_catalog *c)
{
    if (!c) return;
    for (size_t i=0;i<c->count;++i) free_rule(&c->rules[i]);
    free(c->rules);*c=(struct ck2_election_catalog){0};
}
const struct ck2_election_rule *ck2_election_find(const struct ck2_election_catalog *c,
                                                   const char *id)
{
    if (!c || !id) return NULL;
    for (size_t i=0;i<c->count;++i)
        if (!strcmp(c->rules[i].id,id)) return &c->rules[i];
    return NULL;
}
static bool read_section(struct ck2_election_rule *rule,int section,char *script)
{
    if (!script) return false;
    free(rule->script[section]);rule->script[section]=script;
    rule->has_base_days[section]=false;
    rule->base_days[section]=0;
    rule->modifier_count[section]=0;
    if (section==CK2_ELECTOR_SELECTION) rule->has_max_electors=false;
    struct ck2_lexer lex={script};struct ck2_span k,v;
    while (pair(&lex,&k,&v)) {
        if (ck2_span_eq(v,"{")) {
            if (ck2_span_eq(k,"additive_modifier") || ck2_span_eq(k,"modifier"))
                ++rule->modifier_count[section];
            ck2_skip_scope(&lex);
        } else if (ck2_span_eq(k,"days")) {
            int n;if (parse_int(v,&n)) {
                rule->has_base_days[section]=true;
                rule->base_days[section]=n;
            }
        } else if (section==CK2_ELECTOR_SELECTION && ck2_span_eq(k,"max_amount")) {
            int n;if (parse_int(v,&n) && n>0) {
                rule->has_max_electors=true;
                rule->max_electors=n;
            }
        }
    }
    return true;
}
static bool read_definition(struct ck2_election_rule *rule,struct ck2_lexer *lex)
{
    struct ck2_span k,v;
    while (pair(lex,&k,&v)) {
        if (!ck2_span_eq(v,"{")) continue;
        const int section=section_id(k);
        if (section<0) ck2_skip_scope(lex);
        else if (!read_section(rule,section,capture_scope(lex))) return false;
    }
    return true;
}
bool ck2_elections_parse(struct ck2_election_catalog *c,const char *source)
{
    if (!c || !source) return false;
    struct ck2_lexer lex={source};struct ck2_span key,value;
    while (pair(&lex,&key,&value)) {
        if (!ck2_span_eq(value,"{")) continue;
        char name[80];
        if (key.length>=sizeof name) {ck2_skip_scope(&lex);continue;}
        ck2_span_copy(name,sizeof name,key);
        struct ck2_election_rule *r=(struct ck2_election_rule *)ck2_election_find(c,name);
        if (!r) {
            if (c->count==c->capacity) {
                const size_t next=c->capacity ? c->capacity*2 : 16;
                if (next>256 || next>SIZE_MAX/sizeof(*c->rules)) return false;
                void *new_rules=realloc(c->rules,next*sizeof *c->rules);
                if (!new_rules) return false;
                c->rules=new_rules;c->capacity=next;
            }
            r=&c->rules[c->count++];*r=(struct ck2_election_rule){0};
            strcpy(r->id,name);
        } else {
            /* Later base/DLC/mod definitions override the entire rule. */
            free_rule(r);strcpy(r->id,name);
        }
        if (!read_definition(r,&lex)) return false;
    }
    return true;
}
bool ck2_election_excludes_incumbent(const struct ck2_election_rule *r)
{
    if (!r || !r->script[CK2_CANDIDATE_TRIGGER]) return false;
    struct ck2_lexer lex={r->script[CK2_CANDIDATE_TRIGGER]};
    struct ck2_span key,value;
    while (pair(&lex,&key,&value)) {
        if (!ck2_span_eq(value,"{")) continue;
        if (!ck2_span_eq(key,"NOT")) {ck2_skip_scope(&lex);continue;}
        char *scope=capture_scope(&lex);
        if (!scope) return false;
        struct ck2_lexer clause={scope};
        const struct ck2_span actor=ck2_lex(&clause);
        const struct ck2_span equal=ck2_lex(&clause);
        const struct ck2_span from=ck2_lex(&clause);
        const struct ck2_span end=ck2_lex(&clause);
        const bool excludes=ck2_span_eq(actor,"character") &&
            ck2_span_eq(equal,"=") && ck2_span_eq(from,"FROM") && !end.length;
        free(scope);
        if (excludes) return true;
    }
    return false;
}

static enum ck2_election_truth flip(enum ck2_election_truth t)
{
    return t==CK2_ELECTION_TRUE ? CK2_ELECTION_FALSE :
        t==CK2_ELECTION_FALSE ? CK2_ELECTION_TRUE : CK2_ELECTION_UNKNOWN;
}
static enum ck2_election_truth combine(enum ck2_election_truth a,
                                       enum ck2_election_truth b,bool any)
{
    if (any) {
        if (a==CK2_ELECTION_TRUE || b==CK2_ELECTION_TRUE) return CK2_ELECTION_TRUE;
        if (a==CK2_ELECTION_UNKNOWN || b==CK2_ELECTION_UNKNOWN) return CK2_ELECTION_UNKNOWN;
        return CK2_ELECTION_FALSE;
    }
    if (a==CK2_ELECTION_FALSE || b==CK2_ELECTION_FALSE) return CK2_ELECTION_FALSE;
    if (a==CK2_ELECTION_UNKNOWN || b==CK2_ELECTION_UNKNOWN) return CK2_ELECTION_UNKNOWN;
    return CK2_ELECTION_TRUE;
}
static enum ck2_election_truth predicate(struct ck2_span k,struct ck2_span v,
    const struct ck2_election_candidate *ctx,bool on_candidate)
{
    if (ck2_span_eq(k,"character") && (ck2_span_eq(v,"ROOT") || ck2_span_eq(v,"FROM"))) {
        if (!ctx->ruler_id || !ctx->candidate_id) return CK2_ELECTION_UNKNOWN;
        const unsigned current=on_candidate ? ctx->candidate_id : ctx->ruler_id;
        const unsigned target=ck2_span_eq(v,"FROM") ? ctx->candidate_id : ctx->ruler_id;
        return current==target ? CK2_ELECTION_TRUE : CK2_ELECTION_FALSE;
    }
    if (ck2_span_eq(k,"trait")) {
        const char *const *traits=on_candidate ? ctx->candidate_traits : ctx->ruler_traits;
        const size_t count=on_candidate ? ctx->candidate_trait_count : ctx->ruler_trait_count;
        for (size_t i=0;i<count;++i)
            if (traits && traits[i] && ck2_span_eq(v,traits[i])) return CK2_ELECTION_TRUE;
        /* CK2 scripts can add traits after the historical character snapshot. */
        return CK2_ELECTION_UNKNOWN;
    }
    return CK2_ELECTION_UNKNOWN;
}
static enum ck2_election_truth eval_scope(struct ck2_lexer *lex,
    const struct ck2_election_candidate *ctx,bool on_candidate,
    bool any,bool negate,unsigned depth)
{
    enum ck2_election_truth total=any ? CK2_ELECTION_FALSE : CK2_ELECTION_TRUE;
    if (depth>48) {ck2_skip_scope(lex);return CK2_ELECTION_UNKNOWN;}
    struct ck2_span key,value;
    while (pair(lex,&key,&value)) {
        enum ck2_election_truth part=CK2_ELECTION_UNKNOWN;
        if (ck2_span_eq(value,"{")) {
            if (ck2_span_eq(key,"NOT") || ck2_span_eq(key,"NOR") ||
                ck2_span_eq(key,"NAND") || ck2_span_eq(key,"AND") || ck2_span_eq(key,"OR")) {
                const bool new_any=ck2_span_eq(key,"OR") || ck2_span_eq(key,"NOR");
                const bool new_negate=ck2_span_eq(key,"NOT") || ck2_span_eq(key,"NOR") ||
                                       ck2_span_eq(key,"NAND");
                part=eval_scope(lex,ctx,on_candidate,new_any,new_negate,depth+1);
            } else if (ck2_span_eq(key,"FROM") || ck2_span_eq(key,"ROOT")) {
                part=eval_scope(lex,ctx,ck2_span_eq(key,"FROM"),false,false,depth+1);
            } else {
                /* Unimplemented ROOT_FROMFROM, any_claim, any_liege, job scope,
                 * religion scope, etc. Never inspect nested tokens out of context. */
                ck2_skip_scope(lex);
            }
        } else part=predicate(key,value,ctx,on_candidate);
        total=combine(total,part,any);
    }
    return negate ? flip(total) : total;
}
enum ck2_election_truth ck2_election_candidate_trigger(
    const struct ck2_election_rule *rule,const struct ck2_election_candidate *ctx)
{
    if (!rule || !ctx || !rule->script[CK2_CANDIDATE_TRIGGER]) return CK2_ELECTION_UNKNOWN;
    struct ck2_lexer lex={rule->script[CK2_CANDIDATE_TRIGGER]};
    return eval_scope(&lex,ctx,false,false,false,0);
}

size_t ck2_election_office_keys(const struct ck2_election_rule *rule,
                                char (*keys)[64],size_t capacity)
{
    if (!rule || !rule->script[CK2_ELECTOR_SELECTION]) return 0;
    size_t count=0;
    struct ck2_lexer lex={rule->script[CK2_ELECTOR_SELECTION]};
    struct ck2_span tok;
    while ((tok=ck2_lex(&lex)).length) {
        if (!ck2_span_eq(tok,"has_job_title") && !ck2_span_eq(tok,"has_minor_title")) continue;
        if (!ck2_span_eq(ck2_lex(&lex),"=")) continue;
        struct ck2_span key=ck2_lex(&lex);
        if (key.length==0 || key.length>=64 || ck2_span_eq(key,"{")) continue;
        char identifier[64];ck2_span_copy(identifier,sizeof identifier,key);
        size_t i=0;
        for (;i<count && i<capacity;++i)
            if (keys && !strcmp(keys[i],identifier)) break;
        if (i<count && i<capacity) continue;
        /* Even when the output is truncated, count the remaining unique
         * identifiers (the consumer should pass sufficient capacity). */
        if (keys && count<capacity) strcpy(keys[count],identifier);
        ++count;
    }
    return count;
}
