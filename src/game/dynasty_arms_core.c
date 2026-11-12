#include "dynasty_arms_core.h"
#include "ck2_lexer_core.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

static struct ck2_span next(struct ck2_lexer *l) { return ck2_lex(l); }
static bool eq(struct ck2_span s,const char *p){return ck2_span_eq(s,p);}
static void str(char *s,size_t n,struct ck2_span t){ck2_span_copy(s,n,t);}
static int integer(struct ck2_span s){char b[40]={0},*end; str(b,sizeof b,s); long n=strtol(b,&end,10); return b[0] && !*end && n>=INT_MIN && n<=INT_MAX ? (int)n : 0;}
static double real(struct ck2_span s){char b[48]={0};str(b,sizeof b,s);return strtod(b,NULL);}
static bool entry(struct ck2_lexer *l,struct ck2_span *key,struct ck2_span *value){
    *key=next(l); if(!key->length || eq(*key,"}"))return false;
    if(!eq(next(l),"="))return false;
    *value=next(l);return value->start!=NULL;
}
static void skip(struct ck2_lexer *l,struct ck2_span val){if(eq(val,"{"))ck2_skip_scope(l);}
static void path(char *out,size_t size,struct ck2_span in){
    char raw[256];str(raw,sizeof raw,in);size_t n=0;
    for(size_t i=0;raw[i] && n+1<size;i++){
        const char c=raw[i]=='\\' ? '/' : raw[i];
        if(c=='/' && n && out[n-1]=='/')continue;
        out[n++]=c;
    }
    out[n]=0;
}
static void parse_layer(struct coa_layer *layer, struct ck2_lexer *l){
    struct ck2_span k,v;size_t colors=0;
    while(entry(l,&k,&v)){
        if(eq(k,"texture"))layer->texture=integer(v);
        else if(eq(k,"texture_internal"))layer->frame=integer(v);
        else if(eq(k,"emblem"))layer->emblem=integer(v);
        else if(eq(k,"color") && colors<3)layer->colors[colors++]=integer(v);
        skip(l,v);
    }
}
static void parse_coa(struct coa_definition *d,struct ck2_lexer *l){
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"religion"))str(d->religion,sizeof d->religion,v);
        else if(eq(k,"template"))d->template_index=integer(v);
        else if(eq(k,"layer") && eq(v,"{")){
            if(d->layer_count<COA_MAX_LAYERS)parse_layer(&d->layers[d->layer_count++],l);
            else ck2_skip_scope(l);
            continue;
        }else if(eq(k,"data") && eq(v,"{")){
            int nums[1+COA_MAX_LAYERS*6]; size_t count=0;struct ck2_span t;
            while((t=next(l)).length && !eq(t,"}")){
                if(count<sizeof nums/sizeof nums[0])nums[count++]=integer(t);
            }
            if(count>=7 && (count-1)%6==0 && (count-1)/6<=COA_MAX_LAYERS){
                d->template_index=nums[0];d->layer_count=(count-1)/6;
                for(size_t i=0;i<d->layer_count;i++){
                    const int *p=nums+1+i*6;
                    d->layers[i]=(struct coa_layer){.texture=p[0],.emblem=p[1],.frame=p[2],
                        .colors={p[3],p[4],p[5]}};
                }
            }
            continue;
        }
        skip(l,v);
    }
}
static void parse_dynasty(struct coa_definition *d,struct ck2_lexer *l){
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"name"))str(d->name,sizeof d->name,v);
        else if(eq(k,"religion"))str(d->religion,sizeof d->religion,v);
        else if(eq(k,"culture"))str(d->culture,sizeof d->culture,v);
        else if(eq(k,"coat_of_arms") && eq(v,"{")){
            d->authored=true;parse_coa(d,l);continue;
        }
        skip(l,v);
    }
}
static int compare_dynasty(const void *a,const void *b){
    unsigned x=((const struct coa_definition*)a)->dynasty_id,y=((const struct coa_definition*)b)->dynasty_id;
    return (x>y)-(x<y);
}
const struct coa_definition *coa_find_dynasty(const struct coa_catalog *cat,unsigned id){
    if(!cat || !id)return NULL;
    const struct coa_definition key={.dynasty_id=id};
    return bsearch(&key,cat->dynasties,cat->dynasty_count,sizeof key,compare_dynasty);
}
bool coa_catalog_add_dynasties(struct coa_catalog *cat,const char *source){
    if(!cat || !source)return false;
    struct ck2_lexer l={source};struct ck2_span k,v;
    while(entry(&l,&k,&v)){
        if(!eq(v,"{")) {skip(&l,v);continue;}
        struct coa_definition d={.dynasty_id=(unsigned)integer(k)};
        parse_dynasty(&d,&l);
        if(!d.dynasty_id)continue;
        /* Sort after appending; successive files overwrite matching IDs. */
        struct coa_definition *old=NULL;
        for(size_t i=0;i<cat->dynasty_count;i++)
            if(cat->dynasties[i].dynasty_id==d.dynasty_id){old=&cat->dynasties[i];break;}
        if(old){*old=d;continue;}
        if(cat->dynasty_count==cat->dynasty_capacity){
            size_t n=cat->dynasty_capacity?cat->dynasty_capacity*2:512;
            struct coa_definition *p=realloc(cat->dynasties,n*sizeof *p);
            if(!p)return false;
            cat->dynasties=p;cat->dynasty_capacity=n;
        }
        cat->dynasties[cat->dynasty_count++]=d;
    }
    qsort(cat->dynasties,cat->dynasty_count,sizeof *cat->dynasties,compare_dynasty);
    return true;
}
static void parse_rgb_colors(struct coa_style *s,struct ck2_lexer *l){
    struct ck2_span t;
    while((t=next(l)).length && !eq(t,"}")){
        if(!eq(t,"{")){continue;}
        int v[3]={0};for(int i=0;i<3;i++)v[i]=integer(next(l));
        ck2_skip_scope(l);
        if(s->color_count<COA_MAX_COLORS){
            struct coa_rgb *c=&s->colors[s->color_count++];
            c->r=(uint8_t)(v[0]<0?0:v[0]>255?255:v[0]);
            c->g=(uint8_t)(v[1]<0?0:v[1]>255?255:v[1]);
            c->b=(uint8_t)(v[2]<0?0:v[2]>255?255:v[2]);
        }
    }
}
static void parse_templates_names(struct coa_style *s,struct ck2_lexer *l){
    struct ck2_span t;
    while((t=next(l)).length && !eq(t,"}")){
        if(s->template_count>=COA_MAX_TEMPLATES)continue;
        char name[100];str(name,sizeof name,t);
        char *colon=strrchr(name,':');
        int layers=1;if(colon){*colon++=0;layers=atoi(colon);}
        size_t idx=s->template_count++;
        str(s->template_names[idx],sizeof s->template_names[idx],(struct ck2_span){name,strlen(name)});
        s->template_layers[idx]=layers;
    }
}
static void parse_file_entry(struct coa_texture_definition *d,struct ck2_lexer *l){
    *d=(struct coa_texture_definition){.columns=1,.rows=1,.random_allowed=true};
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"file"))path(d->path,sizeof d->path,v);
        else if(eq(k,"noOfFrames"))d->frames=integer(v);
        else if(eq(k,"color"))d->colors=integer(v);
        else if(eq(k,"random"))d->random_allowed=!eq(v,"no");
        else if(eq(k,"size") && eq(v,"{")){
            struct ck2_span a,b;
            while(entry(l,&a,&b)){
                if(eq(a,"x"))d->columns=integer(b);
                if(eq(a,"y"))d->rows=integer(b);
                skip(l,b);
            }
            continue;
        }
        skip(l,v);
    }
}
static void parse_textures(struct coa_style *s,struct ck2_lexer *l){
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"texture") && eq(v,"{")){
            if(s->texture_count<COA_MAX_TEXTURES)
                parse_file_entry(&s->textures[s->texture_count++],l);
            else ck2_skip_scope(l);
        }else skip(l,v);
    }
}
static void parse_emblem(struct coa_style *s,struct ck2_lexer *l){
    char name[192]={0};int frames=0;
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"file"))path(name,sizeof name,v);
        else if(eq(k,"noOfFrames"))frames=integer(v);
        skip(l,v);
    }
    if(s->emblem_count<COA_MAX_EMBLEMS){
        size_t i=s->emblem_count++;
        snprintf(s->emblems[i],sizeof s->emblems[i],"%s",name);
        s->emblem_frames[i]=frames;
    }
}
static void parse_religions(struct coa_style *s,struct ck2_lexer *l){
    struct ck2_span v;
    while((v=next(l)).length && !eq(v,"}")){
        if(s->religion_count<sizeof s->religions/sizeof s->religions[0])
            str(s->religions[s->religion_count++],sizeof s->religions[0],v);
    }
}
static void parse_style(struct coa_style *s,struct ck2_lexer *l){
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"religion_group"))str(s->religion_group,sizeof s->religion_group,v);
        else if(eq(k,"religion") && eq(v,"{")){parse_religions(s,l);continue;}
        else if(eq(k,"templates") && eq(v,"{")){parse_templates_names(s,l);continue;}
        else if(eq(k,"color") && eq(v,"{")){parse_rgb_colors(s,l);continue;}
        else if(eq(k,"textures") && eq(v,"{")){parse_textures(s,l);continue;}
        else if(eq(k,"emblem") && eq(v,"{")){parse_emblem(s,l);continue;}
        skip(l,v);
    }
}
bool coa_catalog_add_styles(struct coa_catalog *cat,const char *source){
    if(!cat || !source)return false;
    struct ck2_lexer l={source};struct ck2_span k,v;
    while(entry(&l,&k,&v)){
        if(eq(k,"culture") && eq(v,"{")){
            if(cat->style_count>=COA_MAX_STYLES)return false;
            parse_style(&cat->styles[cat->style_count++],&l);
        }else skip(&l,v);
    }
    return cat->style_count>0;
}
static void parse_template_layer(struct coa_template_layer *layer,struct ck2_lexer *l){
    *layer=(struct coa_template_layer){.scale=1};
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"mask"))path(layer->mask_path,sizeof layer->mask_path,v);
        else if(eq(k,"scale"))layer->scale=real(v);
        else if(eq(k,"center") && eq(v,"{")){
            struct ck2_span a,b;
            while(entry(l,&a,&b)){
                if(eq(a,"x"))layer->x=real(b);
                if(eq(a,"y"))layer->y=real(b);
                skip(l,b);
            }
            continue;
        }
        skip(l,v);
    }
}
static void parse_template(struct coa_template *t,struct ck2_lexer *l){
    struct ck2_span k,v;
    while(entry(l,&k,&v)){
        if(eq(k,"name"))str(t->name,sizeof t->name,v);
        else if(eq(k,"frame"))path(t->frame_path,sizeof t->frame_path,v);
        else if(eq(k,"mask"))path(t->mask_path,sizeof t->mask_path,v);
        else if(eq(k,"seal_overlay"))path(t->seal_path,sizeof t->seal_path,v);
        else if(eq(k,"layer") && eq(v,"{")){
            if(t->layer_count<COA_MAX_LAYERS)
                parse_template_layer(&t->layers[t->layer_count++],l);
            else ck2_skip_scope(l);
            continue;
        }
        skip(l,v);
    }
}
bool coa_catalog_add_templates(struct coa_catalog *cat,const char *source){
    if(!cat || !source)return false;
    struct ck2_lexer l={source};struct ck2_span k,v;
    while(entry(&l,&k,&v)){
        if(eq(k,"spriteTypes") && eq(v,"{")){
            struct ck2_span a,b;
            while(entry(&l,&a,&b)){
                if(eq(a,"CoatOfArmsType") && eq(b,"{")){
                    if(cat->template_count>=sizeof cat->templates/sizeof cat->templates[0])return false;
                    parse_template(&cat->templates[cat->template_count++],&l);
                }else skip(&l,b);
            }
        }else skip(&l,v);
    }
    return cat->template_count>0;
}
const struct coa_template *coa_find_template(const struct coa_catalog *c,const char *name){
    if(!c || !name)return NULL;
    for(size_t i=0;i<c->template_count;i++)if(!strcmp(c->templates[i].name,name))return &c->templates[i];
    return NULL;
}
const struct coa_style *coa_style_for(const struct coa_catalog *c,const char *religion,const char *group){
    if(!c)return NULL;
    if(religion && *religion)
        for(size_t i=0;i<c->style_count;i++)for(size_t j=0;j<c->styles[i].religion_count;j++)
            if(!strcmp(religion,c->styles[i].religions[j]))return &c->styles[i];
    if(group && *group)
        for(size_t i=0;i<c->style_count;i++)
            if(!strcmp(group,c->styles[i].religion_group))return &c->styles[i];
    return NULL;
}
static uint32_t next_rand(uint32_t *s){*s^=*s<<13;*s^=*s>>17;*s^=*s<<5;return *s;}
bool coa_generate_approximate(const struct coa_style *s,unsigned id,struct coa_definition *out){
    if(!s||!id||!out||!s->texture_count||!s->color_count||!s->template_count)return false;
    uint32_t state=id*2654435761u+0x9e3779b9u;
    size_t template_index=next_rand(&state)%s->template_count;
    size_t layers=(size_t)s->template_layers[template_index];
    if(!layers||layers>COA_MAX_LAYERS)return false;
    size_t choices[COA_MAX_TEXTURES],count=0;
    for(size_t i=0;i<s->texture_count;i++)if(s->textures[i].random_allowed && s->textures[i].frames>0)choices[count++]=i;
    if(!count)return false;
    *out=(struct coa_definition){.dynasty_id=id,.template_index=(int)template_index,.layer_count=layers};
    for(size_t j=0;j<layers;j++){
        size_t t=choices[next_rand(&state)%count];
        struct coa_layer *l=&out->layers[j];
        l->texture=(int)t;l->frame=(int)(next_rand(&state)%(unsigned)s->textures[t].frames);
        for(size_t k=0;k<3;k++)l->colors[k]=(int)(next_rand(&state)%s->color_count);
    }
    return true;
}
bool coa_catalog_add_religions(struct coa_catalog *cat,const char *source){
    if(!cat || !source)return false;
    struct ck2_lexer l={source};struct ck2_span group,value;
    while(entry(&l,&group,&value)){
        if(!eq(value,"{"))continue;
        char group_name[64];str(group_name,sizeof group_name,group);
        struct ck2_span k,v;
        while(entry(&l,&k,&v)){
            if(eq(v,"{") && cat->religion_count<2048){
                if(cat->religion_count==cat->religion_capacity){
                    size_t n=cat->religion_capacity?cat->religion_capacity*2:128;
                    struct coa_religion *p=realloc(cat->religions,n*sizeof *p);
                    if(!p)return false;
                    cat->religions=p;cat->religion_capacity=n;
                }
                struct coa_religion *r=&cat->religions[cat->religion_count++];
                str(r->name,sizeof r->name,k);
                snprintf(r->group,sizeof r->group,"%s",group_name);
            }
            skip(&l,v);
        }
    }
    return true;
}
const char *coa_religion_group(const struct coa_catalog *cat,const char *religion){
    if(!cat || !religion)return NULL;
    for(size_t i=cat->religion_count;i>0;i--)
        if(!strcmp(cat->religions[i-1].name,religion))return cat->religions[i-1].group;
    return NULL;
}
void coa_catalog_free(struct coa_catalog *cat){
    if(!cat)return;
    free(cat->dynasties);free(cat->religions);*cat=(struct coa_catalog){0};
}
