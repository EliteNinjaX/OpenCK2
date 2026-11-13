#include "economy.h"
#include "ck2_script.h"
#include "title_prestige.h"
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool append_holding(struct economy_snapshot *economy, const struct holding *holding)
{
    if (economy->direct_count == economy->direct_capacity) {
        const size_t capacity = economy->direct_capacity ? economy->direct_capacity * 2 : 16;
        struct holding *next = realloc(economy->direct_holdings, capacity * sizeof *next);
        if (!next) return false;
        economy->direct_holdings = next;
        economy->direct_capacity = capacity;
    }
    economy->direct_holdings[economy->direct_count++] = *holding;
    return true;
}

static bool append_vassal_holding(struct economy_snapshot *economy,
                                  const struct holding *holding,unsigned id)
{
    if (economy->vassal_count==economy->vassal_capacity) {
        const size_t cap=economy->vassal_capacity ? economy->vassal_capacity*2 : 32;
        if (cap<economy->vassal_capacity || cap>SIZE_MAX/sizeof(*economy->vassal_holdings)) return false;
        void *next=realloc(economy->vassal_holdings,cap*sizeof(*economy->vassal_holdings));
        if (!next) return false;
        economy->vassal_holdings=next; economy->vassal_capacity=cap;
    }
    struct taxable_vassal_holding *v=&economy->vassal_holdings[economy->vassal_count++];
    v->holding=*holding; v->holder_id=id;
    return true;
}
static bool known_immediate_vassal(const unsigned *ids, size_t count, unsigned id)
{
    for (size_t i=0;i<count;++i) if (ids[i]==id) return true;
    return false;
}

bool economy_snapshot_track_county(struct economy_snapshot *economy, unsigned province_id)
{
    if (!economy || !province_id) return false;
    for (size_t i = 0; i < economy->direct_county_count; ++i)
        if (economy->direct_counties[i] == province_id) return true;
    if (economy->direct_county_count == economy->direct_county_capacity) {
        const size_t cap = economy->direct_county_capacity ? economy->direct_county_capacity * 2 : 16;
        if (cap <= economy->direct_county_capacity || cap > SIZE_MAX / sizeof(unsigned)) return false;
        unsigned *next = realloc(economy->direct_counties, cap * sizeof(*next));
        if (!next) return false;
        economy->direct_counties = next;
        economy->direct_county_capacity = cap;
    }
    economy->direct_counties[economy->direct_county_count++] = province_id;
    return true;
}

static bool direct_county(const struct economy_snapshot *economy, unsigned id)
{
    for (size_t i = 0; i < economy->direct_county_count; ++i)
        if (economy->direct_counties[i] == id) return true;
    return false;
}

static bool known_direct_holding(const struct economy_snapshot *economy, const char *id)
{
    for (size_t i = 0; i < economy->direct_count; ++i)
        if (!strcmp(economy->direct_holdings[i].id, id)) return true;
    return false;
}

static void calculate_rates(struct economy_snapshot *economy, const struct realm_map *realms,
                            unsigned ruler_id)
{
    const struct ruler_profile *r = realm_ruler(realms, ruler_id);
    struct realm_title_info primary;
    const unsigned rank = realm_primary_title(realms, ruler_id, &primary) ? ck2_title_rank(primary.id) : 0;
    if (economy->defines.demesne_ready && rank) {
        const int stewardship = r && (r->known_stats & (1u << RULER_STEWARDSHIP))
            ? r->stats[RULER_STEWARDSHIP] : 0;
        const struct ruler_profile *spouse = r && r->spouse ? realm_ruler(realms, r->spouse) : NULL;
        const int spouse_ste = spouse && (spouse->known_stats & (1u << RULER_STEWARDSHIP))
            ? spouse->stats[RULER_STEWARDSHIP] : 0;
        struct realm_title_info held[512];
        const size_t n = realm_held_titles(realms, ruler_id, held, 512);
        size_t duchies = 0;
        for (size_t i = 0; i < n; ++i)
            duchies += ck2_title_rank(held[i].id) == 3;
        economy->demesne_limit = ck2_demesne_base_limit(&economy->defines, rank,
                stewardship, spouse_ste, rank == 3 && duchies > 1);
        economy->has_demesne_limit = true;
    }
    if (economy->defines.prestige_ready) {
        struct realm_title_info held[512];
        size_t n = realm_held_titles(realms, ruler_id, held, 512);
        for (size_t i = 0; i < n; ++i) {
            const unsigned tier = ck2_title_rank(held[i].id);
            if (tier) ck2_title_prestige_add(&economy->title_prestige, held[i].id, false, economy->defines.prestige_title);
        }
        struct realm_title_info vassals[64];
        size_t total, offset = 0;
        do {
            total = realm_direct_vassals(realms, ruler_id, offset, vassals, 64);
            for (size_t i = 0; i < 64 && i + offset < total; ++i) {
                const unsigned tier = ck2_title_rank(vassals[i].id);
                if (tier) ck2_title_prestige_add(&economy->title_prestige, vassals[i].id, true, economy->defines.prestige_vassal);
            }
            offset += 64;
        } while (offset < total && offset < 8192);
        economy->monthly_prestige += economy->title_prestige.total_monthly;
        economy->has_prestige_rate = true;
    }
    if (r && economy->defines.attributes_ready) {
        if ((r->known_stats & (1u << RULER_DIPLOMACY))) {
            economy->monthly_prestige += r->stats[RULER_DIPLOMACY] *
                economy->defines.monthly_prestige_per_diplomacy;
        }
        if ((r->known_stats & (1u << RULER_LEARNING))) {
            economy->monthly_piety += r->stats[RULER_LEARNING] *
                economy->defines.monthly_piety_per_learning;
            economy->has_piety_rate = true;
        }
    }
}

int economy_snapshot_build(struct economy_snapshot *economy, const char *root,
                           const struct province_map *provinces, const struct realm_map *realms,
                           const struct construction *construction, unsigned player, struct campaign_date date)
{
    if (!economy || !root || !provinces || !realms || !construction) return -1;
    economy_snapshot_free(economy);
    char *defines = load_data(root, "common/defines.lua");
    char *modifiers = load_data(root, "common/static_modifiers.txt");
    ck2_economy_parse_defines(&economy->defines, defines, modifiers);
    SDL_free(defines);
    SDL_free(modifiers);
    calculate_rates(economy, realms, player);
    /* A baron/county must be demonstrably held by a *direct* vassal;
     * never count unverified lower-vassal income as player income. */
    const size_t nv=realm_direct_vassals(realms,player,0,NULL,0);
    unsigned *vassals=nv && nv<SIZE_MAX/sizeof(unsigned) ? calloc(nv,sizeof(unsigned)):NULL;
    if (nv && !vassals) { economy_snapshot_free(economy); return -1; }
    for (size_t start=0;start<nv;start+=64) {
        struct realm_title_info page[64];
        (void)realm_direct_vassals(realms,player,start,page,64);
        const size_t count=nv-start<64 ? nv-start:64;
        for (size_t k=0;k<count;++k) vassals[start+k]=page[k].holder;
    }
    for (size_t p = 0; p < provinces->count; ++p) {
        const struct province *province = &provinces->provinces[p];
        if (!province->land) continue;
        char *history = province_history_read(province, root);
        if (!history) continue;
        struct county_holdings county;
        holdings_read(&county, history, date);
        SDL_free(history);
        if (!county.count) continue;
        struct realm_title_info county_title;
        const bool has_county = realm_county_info(realms, p, false, &county_title);
        /* Retain only title-history verified ownership. The owner of a new
         * settlement is known because constructing it required this county. */
        if (player && has_county && county_title.holder == player &&
            !economy_snapshot_track_county(economy, province->id)) {
            free(vassals);
            economy_snapshot_free(economy);
            return -1;
        }
        for (size_t h = 0; h < county.count; ++h) {
            const struct holding *holding = &county.slots[h];
            struct realm_title_info barony;
            const bool explicit_holder = realm_title_info(realms, holding->id, &barony) && barony.holder;
            unsigned holder = explicit_holder ? barony.holder : 0;
            if (!holder && h == 0 && has_county && county_title.holder) {
                /* Unrecorded capital baronies require an owner-generation
                 * algorithm. County holder is temporary, explicitly counted. */
                holder = county_title.holder;
                if (holder == player) ++economy->holder_fallbacks;
            } else if (!holder) {
                ++economy->unknown_holders;
            }
            if (holder == player && player && !append_holding(economy, holding)) {
                free(vassals);
                economy_snapshot_free(economy);
                return -1;
            }
            if (holder && holder != player && known_immediate_vassal(vassals,nv,holder) &&
                !append_vassal_holding(economy,holding,holder)) {
                free(vassals);
                economy_snapshot_free(economy);
                return -1;
            }
        }
    }
    free(vassals);
    economy_snapshot_refresh(economy, construction);
    return 0;
}

void economy_snapshot_refresh(struct economy_snapshot *economy, const struct construction *state)
{
    if (!economy || !state) return;
    /* The static bookmark holdings were loaded once; a finished newly
     * founded settlement is not in that history. Add it once, never while
     * its construction is pending. Repeated monthly refreshes are safe. */
    const size_t count = construction_settlement_count(state);
    for (size_t i = 0; i < count; ++i) {
        unsigned province = 0;
        const struct holding *holding = NULL;
        bool complete = false;
        if (!construction_settlement_info(state, i, &province, &holding, &complete) ||
            !complete || !holding || !direct_county(economy, province) ||
            known_direct_holding(economy, holding->id)) continue;
        if (!append_holding(economy, holding)) {
            /* The snapshot remains consistent and can be retried next tick. */
            break;
        }
    }
    economy->annual_direct_tax = 0;
    economy->missing_tax_definitions = 0;
    for (size_t i = 0; i < economy->direct_count; ++i) {
        double annual;
        if (construction_holding_annual_tax(state, &economy->direct_holdings[i], &annual))
            economy->annual_direct_tax += annual;
        else
            ++economy->missing_tax_definitions;
    }
}

void economy_snapshot_apply_laws(struct economy_snapshot *economy,const struct construction *buildings,
                                 const struct ck2_law_catalog *catalog,const struct ck2_law_state *selected)
{
    if (!economy || !buildings) return;
    economy->annual_law_vassal_tax=0;
    economy->vassal_tax_skipped=0;
    for (size_t i=0;i<economy->vassal_count;++i) {
        const struct holding *holding=&economy->vassal_holdings[i].holding;
        double rate,annual;
        if (!ck2_law_holding_tax(selected,catalog,holding->type,&rate) ||
            !construction_holding_annual_tax(buildings,holding,&annual)) {
            ++economy->vassal_tax_skipped;
            continue;
        }
        economy->annual_law_vassal_tax+=annual*rate;
    }
}

void economy_snapshot_free(struct economy_snapshot *economy)
{
    if (!economy) return;
    free(economy->direct_holdings);
    free(economy->direct_counties);
    free(economy->vassal_holdings);
    *economy = (struct economy_snapshot){0};
}
