#include "dynasty_arms.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <SDL_image.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* NOTE: Render compositing is an OpenCK2 approximation of CK2's coa_shield.lua
 * pixel shader. Source atlas tiles, authored indices, palette and template
 * data are original; the exact channel/alpha math is not yet verified. */
enum { ARMS_PIXELS=96 };
struct dynasty_arms_runtime {
    struct coa_catalog *catalog;
    char *root;
    struct { char path[192]; SDL_Surface *image; } cache[48];
    size_t cache_count;
};
static SDL_Surface *image(struct dynasty_arms_runtime *state,const char *name){
    if(!name || !*name)return NULL;
    for(size_t i=0;i<state->cache_count;i++)
        if(!strcmp(name,state->cache[i].path))return state->cache[i].image;
    if(state->cache_count==sizeof state->cache/sizeof state->cache[0])return NULL;
    char *path=ck2_asset_path(state->root,name);
    SDL_Surface *loaded=path?IMG_Load(path):NULL;
    free(path);
    SDL_Surface *pixels=loaded?SDL_ConvertSurfaceFormat(loaded,SDL_PIXELFORMAT_RGBA32,0):NULL;
    SDL_FreeSurface(loaded);
    size_t i=state->cache_count++;
    snprintf(state->cache[i].path,sizeof state->cache[i].path,"%s",name);
    state->cache[i].image=pixels;
    return pixels;
}
static struct coa_rgb at(SDL_Surface *s,int x,int y,uint8_t *alpha){
    struct coa_rgb rgb={0};*alpha=0;
    if(!s || x<0 || y<0 || x>=s->w || y>=s->h)return rgb;
    const Uint32 *p=(const Uint32*)((const Uint8*)s->pixels+y*s->pitch)+x;
    Uint8 r,g,b,a;SDL_GetRGBA(*p,s->format,&r,&g,&b,&a);
    rgb=(struct coa_rgb){r,g,b};*alpha=a;return rgb;
}
static void put(SDL_Surface *s,int x,int y,struct coa_rgb rgb,uint8_t a){
    Uint32 *p=(Uint32*)((Uint8*)s->pixels+y*s->pitch)+x;
    Uint8 old_r,old_g,old_b,old_a;
    SDL_GetRGBA(*p,s->format,&old_r,&old_g,&old_b,&old_a);
    const unsigned oa= (unsigned)a+(unsigned)old_a*(255-a)/255;
    if(!oa){*p=0;return;}
    const unsigned fraction_old=(unsigned)old_a*(255-a)/255;
    *p=SDL_MapRGBA(s->format,
        (Uint8)(((unsigned)rgb.r*a+old_r*fraction_old)/oa),
        (Uint8)(((unsigned)rgb.g*a+old_g*fraction_old)/oa),
        (Uint8)(((unsigned)rgb.b*a+old_b*fraction_old)/oa),
        (Uint8)oa);
}
static uint8_t coverage(SDL_Surface *s,int x,int y,int w,int h){
    if(!s)return 255;
    uint8_t a;
    struct coa_rgb c=at(s,x*s->w/w,y*s->h/h,&a);
    /* CK2 mask textures are typically monochrome, with or without alpha. */
    unsigned gray=((unsigned)c.r+c.g+c.b)/3;
    return (uint8_t)(a<255 ? (a*gray)/255 : gray);
}
static struct coa_rgb pigment(struct coa_rgb tex,const struct coa_style *style,
                               const struct coa_layer *l,int color_count){
    if(color_count<=0)return tex;  /* pre-painted dynastic texture frames */
    const int *indices=l->colors;
    const unsigned channels[3]={tex.r,tex.g,tex.b};
    unsigned weight=0,rr=0,gg=0,bb=0;
    for(int i=0;i<color_count && i<3;i++){
        const int key=indices[i];
        if(key<0 || (size_t)key>=style->color_count)continue;
        unsigned w=channels[i];
        weight+=w;rr+=w*style->colors[key].r;gg+=w*style->colors[key].g;bb+=w*style->colors[key].b;
    }
    return weight?(struct coa_rgb){rr/weight,gg/weight,bb/weight}:tex;
}
static bool layer_draw(struct dynasty_arms_runtime *state,SDL_Surface *out,
                       const struct coa_style *style,const struct coa_template_layer *tmpl,
                       const struct coa_layer *layer){
    if(layer->texture<0 || (size_t)layer->texture>=style->texture_count)return false;
    const struct coa_texture_definition *def=&style->textures[layer->texture];
    if(layer->frame<0 || layer->frame>=def->frames || def->columns<1 || def->rows<1)return false;
    SDL_Surface *atlas=image(state,def->path);
    if(!atlas || atlas->w<def->columns || atlas->h<def->rows)return false;
    int cellw=atlas->w/def->columns,cellh=atlas->h/def->rows;
    const int tilex=(layer->frame%def->columns)*cellw;
    const int tiley=(layer->frame/def->columns)*cellh;
    SDL_Surface *mask=image(state,tmpl->mask_path);
    if(!mask)return false; /* A missing template mask must not corrupt layered arms. */
    int dx=(int)lrint(tmpl->x*out->w),dy=(int)lrint(tmpl->y*out->h);
    int dw=(int)lrint(tmpl->scale*out->w),dh=(int)lrint(tmpl->scale*out->h);
    if(dw<1 || dh<1)return false;
    bool painted=false;
    for(int y=0;y<dh;y++)for(int x=0;x<dw;x++){
        int rx=dx+x,ry=dy+y;
        if(rx<0 || ry<0 || rx>=out->w || ry>=out->h)continue;
        uint8_t a;
        struct coa_rgb tex=at(atlas,tilex+x*cellw/dw,tiley+y*cellh/dh,&a);
        uint8_t m=coverage(mask,x,y,dw,dh);
        a=(uint8_t)((unsigned)a*m/255);
        if(!a)continue;
        put(out,rx,ry,pigment(tex,style,layer,def->colors),a);
        painted=true;
    }
    return painted;
}
static void overlay(SDL_Surface *out,SDL_Surface *src){
    if(!out || !src)return;
    SDL_SetSurfaceBlendMode(src,SDL_BLENDMODE_BLEND);
    SDL_BlitScaled(src,NULL,out,NULL);
}
static SDL_Texture *draw(struct dynasty_arms_runtime *state,SDL_Renderer *renderer,
                         const struct coa_style *style,const struct coa_definition *d){
    if(d->template_index<0 || (size_t)d->template_index>=style->template_count)return NULL;
    const struct coa_template *t=coa_find_template(state->catalog,style->template_names[d->template_index]);
    if(!t || !d->layer_count)return NULL;
    SDL_Surface *out=SDL_CreateRGBSurfaceWithFormat(0,ARMS_PIXELS,ARMS_PIXELS,32,SDL_PIXELFORMAT_RGBA32);
    if(!out)return NULL;
    SDL_FillRect(out,NULL,SDL_MapRGBA(out->format,0,0,0,0));
    size_t count=d->layer_count<t->layer_count?d->layer_count:t->layer_count;
    bool painted=false;
    for(size_t i=0;i<count;i++)
        painted |= layer_draw(state,out,style,&t->layers[i],&d->layers[i]);
    if(!painted){SDL_FreeSurface(out);return NULL;}
    /* Seal mask clips the emblem to the original coat-of-arms silhouette. */
    SDL_Surface *seal=image(state,t->mask_path);
    if(seal){
        for(int y=0;y<out->h;y++)for(int x=0;x<out->w;x++){
            Uint32 *p=(Uint32*)((Uint8*)out->pixels+y*out->pitch)+x;
            Uint8 r,g,b,a;SDL_GetRGBA(*p,out->format,&r,&g,&b,&a);
            if(!a)continue;
            a=(uint8_t)((unsigned)a*coverage(seal,x,y,out->w,out->h)/255);
            *p=SDL_MapRGBA(out->format,r,g,b,a);
        }
    }
    overlay(out,image(state,t->frame_path));
    /* Seal overlay is decorative; use original file when present. */
    overlay(out,image(state,t->seal_path));
    SDL_Texture *result=SDL_CreateTextureFromSurface(renderer,out);
    SDL_FreeSurface(out);
    return result;
}
struct dynasty_arms_runtime *dynasty_arms_open(const char *root){
    if(!root)return NULL;
    struct dynasty_arms_runtime *s=calloc(1,sizeof *s);
    if(!s)return NULL;
    s->catalog=calloc(1,sizeof *s->catalog);
    s->root=malloc(strlen(root)+1);
    if(!s->catalog || !s->root){dynasty_arms_close(s);return NULL;}
    strcpy(s->root,root);
    char *data=load_data(root,"interface/coat_of_arms/coats_of_arms.txt");
    if(data){coa_catalog_add_styles(s->catalog,data);SDL_free(data);}
    data=load_data(root,"interface/coats_of_arms.gfx");
    if(data){coa_catalog_add_templates(s->catalog,data);SDL_free(data);}
    struct ck2_asset_files files=ck2_list_assets(root,"common/dynasties");
    for(size_t i=0;i<files.count;i++){
        const char *name=files.names[i];size_t n=strlen(name);
        if(n<4 || strcmp(name+n-4,".txt"))continue;
        data=load_from_directory(root,"common/dynasties",name);
        if(data){coa_catalog_add_dynasties(s->catalog,data);SDL_free(data);}
    }
    ck2_free_asset_files(&files);
    /* Primary religion group lookup is data driven, not keyed to Christianity. */
    files=ck2_list_assets(root,"common/religions");
    for(size_t i=0;i<files.count;i++){
        const char *name=files.names[i];size_t n=strlen(name);
        if(n<4 || strcmp(name+n-4,".txt"))continue;
        data=load_from_directory(root,"common/religions",name);
        if(data){coa_catalog_add_religions(s->catalog,data);SDL_free(data);}
    }
    ck2_free_asset_files(&files);
    return s;
}
SDL_Texture *dynasty_arms_render(struct dynasty_arms_runtime *state,SDL_Renderer *renderer,
                                  unsigned dynasty_id,const char *religion,bool *approximate){
    if(approximate)*approximate=false;
    if(!state||!renderer||!dynasty_id)return NULL;
    const struct coa_definition *entry=coa_find_dynasty(state->catalog,dynasty_id);
    const char *faith=entry && entry->religion[0]?entry->religion:religion;
    const char *group=coa_religion_group(state->catalog,faith);
    const struct coa_style *style=coa_style_for(state->catalog,faith,group);
    if(!style)return NULL;
    if(entry && entry->authored && entry->layer_count)
        return draw(state,renderer,style,entry);
    struct coa_definition random;
    if(!coa_generate_approximate(style,dynasty_id,&random))return NULL;
    if(approximate)*approximate=true;
    return draw(state,renderer,style,&random);
}
void dynasty_arms_close(struct dynasty_arms_runtime *s){
    if(!s)return;
    for(size_t i=0;i<s->cache_count;i++)SDL_FreeSurface(s->cache[i].image);
    if(s->catalog){coa_catalog_free(s->catalog);free(s->catalog);}
    free(s->root);free(s);
}
