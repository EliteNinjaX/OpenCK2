#include "government_mechanics_core.h"
#include "ck2_lexer_core.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

static bool positive(double v) { return isfinite(v) && v > 0; }
static bool finite_nonnegative(double v) { return isfinite(v) && v >= 0; }
static void copy_key(char *dest, size_t capacity, struct ck2_span tok)
{
    ck2_span_copy(dest, capacity, tok);
}
/* The original CK2 Lua declares NGame = { ... } and NNomad = { ... }.
 * Tokenize line-by-line, tracking section braces; never execute Lua code. */
bool ck2_government_rules_parse(struct ck2_government_rules *rules, const char *lua)
{
    if (!rules || !lua) return false;
    struct ck2_government_rules temp = {0};
    const char *p = lua;
    char section[32] = {0};
    int depth = 0;
    while (*p) {
        const char *end = strchr(p, '\n');
        if (!end) end = p + strlen(p);
        char line[384];
        size_t n = (size_t)(end - p);
        if (n >= sizeof line) n = sizeof line - 1;
        memcpy(line, p, n); line[n] = 0;
        char *comment = strstr(line, "--");
        if (comment) *comment = 0;
        char *start = line;
        while (isspace((unsigned char)*start)) ++start;
        char block[32];
        if (depth == 1 && sscanf(start, "%31[A-Za-z0-9_] = {", block) == 1 &&
            strchr(start, '{')) snprintf(section,sizeof section,"%s",block);
        char key[128]; double number;
        if (depth == 2 && sscanf(start, "%127[A-Za-z0-9_] = %lf", key, &number) == 2 &&
            isfinite(number) && number >= 0) {
            if (!strcmp(section, "NNomad")) {
                if (!strcmp(key, "POPULATION_INCREASE_MULTIPLIER")) temp.nomad_growth=number;
                else if (!strcmp(key, "START_POPULATION")) temp.nomad_start=number;
                else if (!strcmp(key, "MAX_MANPOWER_POPULATION_MULTIPLIER")) temp.nomad_max_manpower=number;
                else if (!strcmp(key, "POPULATION_TAX_MULTIPLIER")) temp.nomad_tax=number;
                else if (!strcmp(key, "MAX_POPULATION_EMPTY_HOLDING_MULTIPLIER")) temp.nomad_max_population_empty_holding=number;
                else if (!strcmp(key, "MAX_CLANS") && number<=UINT_MAX) temp.max_clans=(unsigned)number;
                else if (!strcmp(key, "PROVINCES_PER_CLAN") && number<=UINT_MAX) temp.provinces_per_clan=(unsigned)number;
            } else if (!strcmp(section, "NCharacter") || !strcmp(section, "NEconomy")) {
                if (!strcmp(key, "MERCHANT_REPUBLIC_MAX_PATRICIANS") && number<=UINT_MAX)
                    temp.max_patricians=(unsigned)number;
                else if (!strcmp(key, "MIN_TRADE_POSTS") && number<=UINT_MAX)
                    temp.min_trade_posts=(unsigned)number;
                else if (!strcmp(key, "PATRICIAN_CAMPAIGN_FUND_FACTOR")) temp.respect_fund=number;
                else if (!strcmp(key, "PATRICIAN_PRESTIGE_RESPECT_FACTOR")) temp.respect_prestige=number;
                else if (!strcmp(key, "PATRICIAN_AGE_RESPECT_FACTOR")) temp.respect_age=number;
                else if (!strcmp(key, "DOGE_SUCC_RANDOM_FACTOR")) temp.respect_random=number;
                else if (!strcmp(key, "TRADE_POST_COST_INC_DIST")) temp.trade_post_distance_cost=number;
            }
        }
        for (char *q=start; *q; ++q) {
            if (*q=='{') ++depth;
            else if (*q=='}') --depth;
        }
        if (depth < 0) return false;
        if (depth < 2) section[0]=0;
        p = *end ? end+1 : end;
    }
    if (depth) return false;
    /* Some CK2 distributions use top-level NGame, others NDefines. Do not
     * allow zero-initialized values to masquerade as real original rules. */
    temp.nomad_loaded = positive(temp.nomad_max_population_empty_holding) &&
        positive(temp.nomad_max_manpower) && positive(temp.nomad_start) &&
        positive(temp.nomad_growth) && temp.max_clans>0;
    temp.merchant_loaded = temp.max_patricians>0 && positive(temp.respect_age) &&
        positive(temp.respect_prestige) && positive(temp.respect_fund);
    if (temp.max_clans>CK2_CLAN_LIMIT || temp.max_patricians>CK2_PATRICIAN_LIMIT)
        return false;
    *rules = temp;
    return temp.nomad_loaded || temp.merchant_loaded;
}

static bool trade_parse_building(struct ck2_trade_building *b, struct ck2_lexer *l)
{
    struct ck2_span tok;
    while ((tok=ck2_lex(l)).length && !ck2_span_eq(tok,"}")) {
        if (!ck2_span_eq(ck2_lex(l),"=")) return false;
        struct ck2_span value=ck2_lex(l);
        if (!value.length) return false;
        if (ck2_span_eq(value,"{")) { ck2_skip_scope(l); continue; }
        char num[96], *end;
        copy_key(num,sizeof num,value);
        if (ck2_span_eq(tok,"upgrades_from")) copy_key(b->upgrades_from,sizeof b->upgrades_from,value);
        else if (ck2_span_eq(tok,"gold_cost") || ck2_span_eq(tok,"tradevalue") ||
                 ck2_span_eq(tok,"tax_income") || ck2_span_eq(tok,"build_time")) {
            const double v=strtod(num,&end);
            if (!num[0] || *end || !finite_nonnegative(v)) return false;
            if (ck2_span_eq(tok,"gold_cost")) { b->gold_cost=v; b->has_cost=true; }
            else if (ck2_span_eq(tok,"tradevalue")) b->tradevalue=v;
            else if (ck2_span_eq(tok,"tax_income")) b->tax_income=v;
            else {
                if (v>UINT_MAX || floor(v)!=v) return false;
                b->build_days=(unsigned)v; b->has_days=true;
            }
        }
    }
    return ck2_span_eq(tok,"}");
}
const struct ck2_trade_building *ck2_trade_find(const struct ck2_trade_catalog *c, const char *id)
{
    if (!c || !id) return NULL;
    for (size_t i=0;i<c->count;++i)
        if (!strcmp(c->items[i].key,id)) return c->items+i;
    return NULL;
}
bool ck2_trade_catalog_parse(struct ck2_trade_catalog *c, const char *source)
{
    if (!c || !source) return false;
    struct ck2_lexer l={source}; struct ck2_span tok;
    while ((tok=ck2_lex(&l)).length) {
        if (!ck2_span_eq(ck2_lex(&l),"=") || !ck2_span_eq(ck2_lex(&l),"{")) return false;
        if (!ck2_span_eq(tok,"trade_post")) { ck2_skip_scope(&l); continue; }
        struct ck2_span key;
        while ((key=ck2_lex(&l)).length && !ck2_span_eq(key,"}")) {
            if (!ck2_span_eq(ck2_lex(&l),"=") || !ck2_span_eq(ck2_lex(&l),"{")) return false;
            struct ck2_trade_building b={0};
            if (key.length>=sizeof b.key) return false;
            copy_key(b.key,sizeof b.key,key);
            if (!trade_parse_building(&b,&l)) return false;
            struct ck2_trade_building *prior=(struct ck2_trade_building *)ck2_trade_find(c,b.key);
            if (prior) { *prior=b; continue; }
            if (c->count>=4096) return false;
            if (c->count==c->capacity) {
                size_t next=c->capacity ? c->capacity*2 : 32;
                struct ck2_trade_building *v=realloc(c->items,next*sizeof *v);
                if (!v) return false;
                c->items=v; c->capacity=next;
            }
            c->items[c->count++]=b;
        }
        if (!ck2_span_eq(key,"}")) return false;
    }
    return true;
}
void ck2_trade_catalog_free(struct ck2_trade_catalog *c)
{ if (c) { free(c->items); *c=(struct ck2_trade_catalog){0}; } }

const char *ck2_government_action_text(enum ck2_government_action status)
{
    switch (status) {
    case CK2_GOV_OK: return "Action recorded in OpenCK2 runtime state.";
    case CK2_GOV_MISSING_RULES: return "Required CK2 defines.lua rules are not loaded.";
    case CK2_GOV_INVALID: return "Invalid or unverified action parameters.";
    case CK2_GOV_DUPLICATE: return "A matching character, title, or project already exists.";
    case CK2_GOV_LIMIT: return "The source-defined or runtime limit has been reached.";
    case CK2_GOV_NO_RESOURCE: return "Insufficient population, provinces, or campaign resources.";
    case CK2_GOV_UNSUPPORTED: return "Original CK2 eligibility or scripted approval is unresolved.";
    case CK2_GOV_NO_RECORD: return "No corresponding runtime record exists.";
    } return "Unknown government action";
}

enum ck2_government_action ck2_nomad_add(struct ck2_government_mechanics *s,
    const struct ck2_government_rules *r, unsigned head, unsigned provinces, double cap)
{
    if (!s||!r||!r->nomad_loaded) return CK2_GOV_MISSING_RULES;
    if (!head||!provinces||!positive(cap)||cap>1e12) return CK2_GOV_INVALID;
    if (s->clan_count >= r->max_clans) return CK2_GOV_LIMIT;
    for (size_t i=0;i<s->clan_count;++i) if (s->clans[i].head==head) return CK2_GOV_DUPLICATE;
    struct ck2_nomad_clan c={.head=head,.provinces=provinces,.capacity=cap,
        .population=cap*r->nomad_start};
    if (!positive(c.population) || c.population>cap) return CK2_GOV_INVALID;
    c.manpower=c.population*r->nomad_max_manpower;
    s->clans[s->clan_count++]=c;
    return CK2_GOV_OK;
}
enum ck2_government_action ck2_nomad_split(struct ck2_government_mechanics *s,
    const struct ck2_government_rules *r, unsigned head, unsigned new_head,
    unsigned provinces, double population)
{
    if (!s||!r||!r->nomad_loaded) return CK2_GOV_MISSING_RULES;
    if (!new_head||new_head==head||!provinces||!positive(population)) return CK2_GOV_INVALID;
    if (s->clan_count>=r->max_clans) return CK2_GOV_LIMIT;
    for (size_t i=0;i<s->clan_count;++i) if (s->clans[i].head==new_head) return CK2_GOV_DUPLICATE;
    for (size_t i=0;i<s->clan_count;++i) if (s->clans[i].head==head) {
        struct ck2_nomad_clan *old=s->clans+i;
        if (old->provinces<=provinces || old->population<=population) return CK2_GOV_NO_RESOURCE;
        const double fraction=(double)provinces/old->provinces;
        struct ck2_nomad_clan newer={.head=new_head,.provinces=provinces,
            .capacity=old->capacity*fraction,.population=population};
        if (newer.population>newer.capacity || old->population-population>old->capacity-newer.capacity)
            return CK2_GOV_NO_RESOURCE;
        newer.manpower=old->manpower*(population/old->population);
        old->manpower-=newer.manpower;
        old->population-=population;
        old->capacity-=newer.capacity;
        old->provinces-=provinces;
        s->clans[s->clan_count++]=newer;
        return CK2_GOV_OK;
    }
    return CK2_GOV_NO_RECORD;
}
void ck2_nomad_month(struct ck2_government_mechanics *s, const struct ck2_government_rules *r)
{
    if (!s||!r||!r->nomad_loaded) return;
    for (size_t i=0;i<s->clan_count;++i) {
        struct ck2_nomad_clan *c=s->clans+i;
        if (!positive(c->capacity) || !finite_nonnegative(c->population)) continue;
        c->population+= (c->capacity-c->population)*r->nomad_growth;
        if (c->population>c->capacity) c->population=c->capacity;
        c->manpower=c->population*r->nomad_max_manpower;
    }
}
double ck2_nomad_monthly_tax(const struct ck2_government_mechanics *s,
                            const struct ck2_government_rules *r, unsigned head)
{
    if (!s||!r||!r->nomad_loaded) return 0;
    for (size_t i=0;i<s->clan_count;++i)
        if (s->clans[i].head==head) return s->clans[i].population*r->nomad_tax;
    return 0;
}
enum ck2_government_action ck2_patrician_add(struct ck2_government_mechanics *s,
    const struct ck2_government_rules *r, unsigned head)
{
    if (!s||!r||!r->merchant_loaded) return CK2_GOV_MISSING_RULES;
    if (!head) return CK2_GOV_INVALID;
    for (size_t i=0;i<s->family_count;++i) if (s->families[i].head==head) return CK2_GOV_DUPLICATE;
    if (s->family_count>=r->max_patricians) return CK2_GOV_LIMIT;
    s->families[s->family_count++]=(struct ck2_patrician_family){.head=head};
    return CK2_GOV_OK;
}
enum ck2_government_action ck2_patrician_fund(struct ck2_government_mechanics *s,
    unsigned head, double gold)
{
    if (!s||!positive(gold)||gold>1e12) return CK2_GOV_INVALID;
    for (size_t i=0;i<s->family_count;++i) if (s->families[i].head==head) {
        if (!finite_nonnegative(s->families[i].campaign_fund) ||
            s->families[i].campaign_fund>1e12-gold) return CK2_GOV_LIMIT;
        s->families[i].campaign_fund+=gold;
        return CK2_GOV_OK;
    }
    return CK2_GOV_NO_RECORD;
}
bool ck2_patrician_respect(const struct ck2_government_rules *r, unsigned age,
    double prestige, double fund, double *out)
{
    if (!r||!r->merchant_loaded||!out||age>200||!isfinite(prestige)||
        !finite_nonnegative(fund)) return false;
    const double value=age*(double)age*r->respect_age+prestige*r->respect_prestige+
                       fund*r->respect_fund;
    if (!isfinite(value)) return false;
    *out=value;return true;
}
unsigned ck2_patrician_respect_leader(const struct ck2_government_mechanics *s,
    const struct ck2_government_rules *r, const unsigned *ages,
    const double *prestiges, const bool *known)
{
    if (!s||!r||!ages||!prestiges||!known) return 0;
    double best=-HUGE_VAL; unsigned winner=0; bool tie=false;
    for (size_t i=0;i<s->family_count;++i) {
        double value;
        if (!known[i] || !ck2_patrician_respect(r,ages[i],prestiges[i],
                                                 s->families[i].campaign_fund,&value)) return 0;
        if (value>best) { best=value; winner=s->families[i].head; tie=false; }
        else if (value==best) tie=true;
    }
    return tie?0:winner;
}
static struct ck2_trade_post *find_post(struct ck2_government_mechanics *s, unsigned province)
{
    for (size_t i=0;i<s->post_count;++i) if (s->posts[i].province==province) return s->posts+i;
    return NULL;
}
enum ck2_government_action ck2_trade_build(struct ck2_government_mechanics *s,
    const struct ck2_trade_catalog *cat, unsigned patrician, unsigned province,
    const char *building, bool verified_port, double *gold_cost)
{
    if (!s||!cat||!gold_cost||!patrician||!province||!building) return CK2_GOV_INVALID;
    if (!verified_port) return CK2_GOV_UNSUPPORTED;
    bool registered=false;
    for (size_t i=0;i<s->family_count;++i) if (s->families[i].head==patrician) registered=true;
    if (!registered) return CK2_GOV_NO_RECORD;
    const struct ck2_trade_building *b=ck2_trade_find(cat,building);
    if (!b||!b->has_cost||!b->has_days||!b->build_days||!positive(b->gold_cost))
        return CK2_GOV_UNSUPPORTED;
    struct ck2_trade_post *post=find_post(s,province);
    if (post && (post->patrician!=patrician||post->current_build[0] ||
                 post->completed_count>=CK2_TRADE_BUILDING_LIMIT)) return CK2_GOV_LIMIT;
    if (!post && (s->post_count==CK2_TRADE_POST_LIMIT||b->upgrades_from[0])) return CK2_GOV_LIMIT;
    if (post) {
        /* Upgrading an existing line requires the exact CK2 prerequisite;
         * independent lines can begin without upgrades_from. */
        if (b->upgrades_from[0]) {
            bool found=false;
            for (size_t i=0;i<post->completed_count;++i)
                if (!strcmp(post->completed[i],b->upgrades_from)) found=true;
            if (!found) return CK2_GOV_UNSUPPORTED;
        }
        for (size_t i=0;i<post->completed_count;++i)
            if (!strcmp(post->completed[i],b->key)) return CK2_GOV_DUPLICATE;
    }
    if (!post) { post=s->posts+s->post_count++; *post=(struct ck2_trade_post){.province=province,.patrician=patrician}; }
    memcpy(post->current_build,b->key,strlen(b->key)+1);
    post->days_left=b->build_days;
    *gold_cost=b->gold_cost;
    return CK2_GOV_OK;
}
void ck2_trade_day(struct ck2_government_mechanics *s)
{
    if (!s) return;
    for (size_t i=0;i<s->post_count;++i) {
        struct ck2_trade_post *p=s->posts+i;
        if (p->current_build[0] && p->days_left && !--p->days_left) {
            strcpy(p->completed[p->completed_count++],p->current_build);
            p->current_build[0]=0;
        }
    }
}
static double trade_completed_effect(const struct ck2_trade_post *p,
    const struct ck2_trade_catalog *cat, bool annual_tax)
{
    if (!p||!cat) return 0;
    double total=0;
    for (size_t i=0;i<p->completed_count;++i) {
        const struct ck2_trade_building *b=ck2_trade_find(cat,p->completed[i]);
        if (!b) continue;
        /* A higher level replaces the preceding level within its CK2 chain. */
        bool superseded=false;
        for (size_t j=0;j<p->completed_count;++j) {
            const struct ck2_trade_building *newer=ck2_trade_find(cat,p->completed[j]);
            const struct ck2_trade_building *chain=newer;
            for (size_t depth=0;chain&&depth<p->completed_count;++depth) {
                if (!chain->upgrades_from[0]) break;
                if (!strcmp(chain->upgrades_from,b->key)) superseded=true;
                chain=ck2_trade_find(cat,chain->upgrades_from);
            }
        }
        if (!superseded) total+=annual_tax?b->tax_income:b->tradevalue;
    }
    return total;
}
double ck2_trade_value(const struct ck2_trade_post *p, const struct ck2_trade_catalog *cat)
{ return trade_completed_effect(p,cat,false); }
double ck2_trade_annual_tax(const struct ck2_trade_post *p, const struct ck2_trade_catalog *cat)
{ return trade_completed_effect(p,cat,true); }
enum ck2_government_action ck2_bishop_nominate(struct ck2_government_mechanics *s,
    const char *temple, unsigned liege, unsigned candidate, const char *law)
{
    if (!s||!temple||strncmp(temple,"b_",2)||strlen(temple)>=72||!liege||!candidate||
        liege==candidate) return CK2_GOV_INVALID;
    const bool papal=law&&!strcmp(law,"investiture_law_0");
    if (!papal && (!law||strcmp(law,"investiture_law_1"))) return CK2_GOV_UNSUPPORTED;
    for (size_t i=0;i<s->bishop_count;++i) if (!strcmp(s->bishops[i].temple,temple)) return CK2_GOV_DUPLICATE;
    if (s->bishop_count==CK2_BISHOP_LIMIT) return CK2_GOV_LIMIT;
    struct ck2_bishop_nomination *n=s->bishops+s->bishop_count++;
    *n=(struct ck2_bishop_nomination){.liege=liege,.candidate=candidate,
                                      .papal_approval_required=papal};
    strcpy(n->temple,temple);
    return CK2_GOV_OK;
}
enum ck2_government_action ck2_bishop_approve(struct ck2_government_mechanics *s,
    const char *temple, bool papal_authority)
{
    if (!s||!temple) return CK2_GOV_INVALID;
    for (size_t i=0;i<s->bishop_count;++i) if (!strcmp(s->bishops[i].temple,temple)) {
        if (s->bishops[i].papal_approval_required&&!papal_authority) return CK2_GOV_UNSUPPORTED;
        s->bishops[i].approved=true;
        return CK2_GOV_OK;
    }
    return CK2_GOV_NO_RECORD;
}
bool ck2_government_mechanics_valid(const struct ck2_government_mechanics *s)
{
    if (!s||s->clan_count>CK2_CLAN_LIMIT||s->family_count>CK2_PATRICIAN_LIMIT||
        s->post_count>CK2_TRADE_POST_LIMIT||s->bishop_count>CK2_BISHOP_LIMIT) return false;
    for (size_t i=0;i<s->clan_count;++i) {
        const struct ck2_nomad_clan *c=s->clans+i;
        if (!c->head||!c->provinces||!positive(c->capacity)||
            !finite_nonnegative(c->population)||c->population>c->capacity||
            !finite_nonnegative(c->manpower)) return false;
        for (size_t j=0;j<i;++j) if (s->clans[j].head==c->head) return false;
    }
    for (size_t i=0;i<s->family_count;++i) {
        if (!s->families[i].head||!finite_nonnegative(s->families[i].campaign_fund)) return false;
        for (size_t j=0;j<i;++j) if (s->families[j].head==s->families[i].head) return false;
    }
    for (size_t i=0;i<s->post_count;++i) {
        const struct ck2_trade_post *p=s->posts+i;
        if (!p->province||!p->patrician||p->completed_count>CK2_TRADE_BUILDING_LIMIT||
            (p->current_build[0]?!p->days_left:p->days_left)) return false;
        for (size_t j=0;j<i;++j) if (s->posts[j].province==p->province) return false;
        for (size_t j=0;j<p->completed_count;++j) if (!memchr(p->completed[j],0,sizeof p->completed[j])||
                !p->completed[j][0]) return false;
        if (!memchr(p->current_build,0,sizeof p->current_build)) return false;
        bool owner=false;
        for (size_t j=0;j<s->family_count;++j)
            if (s->families[j].head==p->patrician) owner=true;
        if (!owner) return false;
    }
    for (size_t i=0;i<s->bishop_count;++i) {
        const struct ck2_bishop_nomination *b=s->bishops+i;
        if (!b->liege||!b->candidate||!b->temple[0]||
            !memchr(b->temple,0,sizeof b->temple)) return false;
        for (size_t j=0;j<i;++j) if (!strcmp(s->bishops[j].temple,b->temple)) return false;
    }
    return true;
}
