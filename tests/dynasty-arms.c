#include "../src/game/dynasty_arms_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *readfile(const char *path){
    FILE *file=fopen(path,"rb");if(!file)return NULL;
    fseek(file,0,SEEK_END);long n=ftell(file);rewind(file);
    if(n<0 || n>50*1024*1024){fclose(file);return NULL;}
    char *p=malloc((size_t)n+1);if(!p){fclose(file);return NULL;}
    if(fread(p,1,(size_t)n,file)!=(size_t)n){free(p);fclose(file);return NULL;}
    p[n]=0;fclose(file);return p;
}
static void fixtures(struct coa_catalog *c){
    const char *dynasties=
        "106={ name=Plantagenet religion=catholic coat_of_arms={ data={ 0 3 0 34 0 0 0 } religion=catholic }}"
        "7={ name=Ottoman religion=sunni coat_of_arms={data={ 0 1 0 43 12 3 10 } }}"
        "2={ name=Orsini coat_of_arms={template=0 layer={texture=5 texture_internal=0 emblem=0 color=0 color=0 color=0}}}"
        "9={name=Unauthored culture=frankish}";
    assert(coa_catalog_add_dynasties(c,dynasties));
    assert(c->dynasty_count==4);
    const struct coa_definition *plant=coa_find_dynasty(c,106);
    assert(plant && plant->authored && plant->layer_count==1 && plant->template_index==0);
    assert(plant->layers[0].texture==3 && plant->layers[0].frame==34);
    assert(coa_find_dynasty(c,7)->layers[0].frame==43);
    assert(coa_find_dynasty(c,2)->layers[0].texture==5);
    assert(!coa_find_dynasty(c,9)->authored);
    assert(coa_catalog_add_styles(c,
        "culture={ religion_group=christian templates={\"coa_plain:1\" \"coa_party_per_cross:4\"}"
        "color={{255 0 0}{0 0 255}} textures={texture={file=\"gfx\\\\coats_of_arms\\\\dynasties.tga\""
        "size={x=16 y=4} noOfFrames=51 color=0 random=no}}}"));
    assert(c->style_count==1 && c->styles[0].color_count==2 && c->styles[0].texture_count==1);
    assert(c->styles[0].textures[0].columns==16 && c->styles[0].textures[0].rows==4);
    assert(!strcmp(c->styles[0].textures[0].path,"gfx/coats_of_arms/dynasties.tga"));
    assert(coa_catalog_add_templates(c,
        "spriteTypes={CoatOfArmsType={name=coa_plain frame=\"gfx\\\\coats_of_arms\\\\frame.tga\" "
        "layer={mask=\"gfx\\\\coats_of_arms\\\\plain.tga\" center={x=0 y=0} scale=1}}}"));
    assert(coa_find_template(c,"coa_plain") && coa_find_template(c,"coa_plain")->layer_count==1);
    assert(coa_catalog_add_religions(c,"christian={ catholic={color={1 1 1}} orthodox={} } muslim={sunni={}}"));
    assert(!strcmp(coa_religion_group(c,"catholic"),"christian"));
    assert(!strcmp(coa_religion_group(c,"sunni"),"muslim"));
}
static void real_files(struct coa_catalog *c,const char *root){
    if(!root)return;
    char name[1024];
    snprintf(name,sizeof name,"%s/common/dynasties/00_dynasties.txt",root);
    char *p=readfile(name);
    if(!p){fprintf(stderr,"warning: CK2 files not found under %s; fixture tests only\n",root);return;}
    assert(coa_catalog_add_dynasties(c,p));free(p);
    snprintf(name,sizeof name,"%s/interface/coat_of_arms/coats_of_arms.txt",root);
    p=readfile(name);assert(p);assert(coa_catalog_add_styles(c,p));free(p);
    snprintf(name,sizeof name,"%s/interface/coats_of_arms.gfx",root);
    p=readfile(name);assert(p);assert(coa_catalog_add_templates(c,p));free(p);
    const struct coa_definition *plant=coa_find_dynasty(c,106);
    assert(plant && plant->authored && plant->layers[0].texture==3 && plant->layers[0].frame==34);
    assert(c->style_count>=5 && c->template_count>=10 && c->dynasty_count>1000);
    printf("CK2 catalogs: %zu dynasties, %zu styles, %zu template sprites\n",
           c->dynasty_count,c->style_count,c->template_count);
}
int main(int argc,char **argv){
    struct coa_catalog *c=calloc(1,sizeof *c);assert(c);
    fixtures(c);coa_catalog_free(c);
    if(argc>1)real_files(c,argv[1]);
    coa_catalog_free(c);free(c);
    puts("dynasty arms: all assertions passed");return 0;
}
