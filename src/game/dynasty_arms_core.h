#ifndef OPENCK2_DYNASTY_ARMS_CORE_H
#define OPENCK2_DYNASTY_ARMS_CORE_H
/* CK2's authored dynasty arms and CoatOfArmsType metadata: no SDL dependency. */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

enum { COA_MAX_STYLES=16, COA_MAX_TEMPLATES=12, COA_MAX_LAYERS=4,
       COA_MAX_TEXTURES=24, COA_MAX_COLORS=32, COA_MAX_EMBLEMS=16 };
struct coa_rgb { uint8_t r,g,b; };
struct coa_layer { int texture, emblem, frame; int colors[3]; };
struct coa_definition {
    unsigned dynasty_id;
    char religion[64], culture[64], name[96];
    int template_index;
    struct coa_layer layers[COA_MAX_LAYERS];
    size_t layer_count;
    bool authored;
};
struct coa_texture_definition {
    char path[192];
    int columns, rows, frames, colors;
    bool random_allowed;
};
struct coa_template_layer {
    char mask_path[192];
    double x,y,scale;
};
struct coa_template {
    char name[80], frame_path[192], mask_path[192], seal_path[192];
    struct coa_template_layer layers[COA_MAX_LAYERS];
    size_t layer_count;
};
struct coa_style {
    char religion_group[64];
    char religions[40][64];
    size_t religion_count;
    char template_names[COA_MAX_TEMPLATES][80];
    int template_layers[COA_MAX_TEMPLATES];
    size_t template_count;
    struct coa_rgb colors[COA_MAX_COLORS];
    size_t color_count;
    struct coa_texture_definition textures[COA_MAX_TEXTURES];
    size_t texture_count;
    char emblems[COA_MAX_EMBLEMS][192];
    int emblem_frames[COA_MAX_EMBLEMS];
    size_t emblem_count;
};
struct coa_religion { char name[64], group[64]; };
struct coa_catalog {
    struct coa_religion *religions;
    size_t religion_count, religion_capacity;
    struct coa_definition *dynasties;
    size_t dynasty_count, dynasty_capacity;
    struct coa_style styles[COA_MAX_STYLES];
    size_t style_count;
    struct coa_template templates[64];
    size_t template_count;
};
bool coa_catalog_add_dynasties(struct coa_catalog *, const char *source);
bool coa_catalog_add_styles(struct coa_catalog *, const char *source);
bool coa_catalog_add_templates(struct coa_catalog *, const char *source);
bool coa_catalog_add_religions(struct coa_catalog *, const char *source);
const char *coa_religion_group(const struct coa_catalog *, const char *religion);
const struct coa_definition *coa_find_dynasty(const struct coa_catalog *, unsigned id);
const struct coa_template *coa_find_template(const struct coa_catalog *, const char *name);
const struct coa_style *coa_style_for(const struct coa_catalog *, const char *religion, const char *group);
/* Engineering fallback, NOT CK2's recovered RNG sequence. Returns false if style is unsuitable. */
bool coa_generate_approximate(const struct coa_style *, unsigned id, struct coa_definition *);
void coa_catalog_free(struct coa_catalog *);
#endif
