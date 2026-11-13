#ifndef OPENCK2_MAP_SETTLEMENTS_H
#define OPENCK2_MAP_SETTLEMENTS_H
#include "campaign.h"
#include "realms.h"
#include "holdings.h"
#include "settlement_styles.h"
#include "governments.h"

/* One visible, dated capital holding per landed province for the first map
 * pass. Later: multiple holding anchors and mesh/model instances. */
struct map_settlement_marker {
    unsigned province_id, holder_id;
    char holding_id[72], name[96], owner[96], culture[64], religion[64], graphical[64], actor[112];
    enum holding_type type;
    float x, y;
    unsigned rgb;
    bool owner_known, county_holder_fallback, culture_color_known, actor_known;
};
/* Province history is distinct from a holding owner's personal culture or faith. */
struct map_province_facts {
    char culture[64], religion[64];
    bool loaded;
};
struct map_settlement_scene {
    struct government_holding_evidence *holding_evidence; /* exact barony owners only */
    size_t holding_evidence_count, holding_evidence_capacity;
    struct map_settlement_marker *markers;
    struct map_province_facts *province_facts; /* indexed by province_map.provinces */
    size_t count, province_fact_count;
    struct campaign_date date;
    struct settlement_styles styles;
    struct government_catalog governments;
    struct government_religions religions;
};
/* Returns -1 if it cannot allocate; optional culture/actor/government assets
 * fail gracefully. Province/holding history must come from mounted CK2 root. */
int map_settlements_build(struct map_settlement_scene *, const char *root,
                          const struct province_map *, const struct realm_map *,
                          struct campaign_date date);
const struct map_settlement_marker *map_settlement_by_id(const struct map_settlement_scene *, unsigned province_id);
void map_settlements_free(struct map_settlement_scene *);
#endif
