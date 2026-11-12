#include "economy.h"
#include "ck2_script.h"
#include <math.h>
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

static unsigned title_rank(const char *id)
{
    if (!id || id[1] != '_') return 0;
    static const char ranks[] = "bcdke";
    const char *p = strchr(ranks, id[0]);
    return p ? (unsigned)(p - ranks) + 1 : 0;
}

static void calculate_rates(struct economy_snapshot *economy, const struct realm_map *realms,
                            unsigned ruler_id)
{
    const struct ruler_profile *r = realm_ruler(realms, ruler_id);
    struct realm_title_info primary;
    const unsigned rank = realm_primary_title(realms, ruler_id, &primary) ? title_rank(primary.id) : 0;
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
            duchies += title_rank(held[i].id) == 3;
        economy->demesne_limit = ck2_demesne_base_limit(&economy->defines, rank,
                stewardship, spouse_ste, rank == 3 && duchies > 1);
        economy->has_demesne_limit = true;
    }
    if (economy->defines.prestige_ready) {
        struct realm_title_info held[512];
        size_t n = realm_held_titles(realms, ruler_id, held, 512);
        for (size_t i = 0; i < n; ++i) {
            const unsigned tier = title_rank(held[i].id);
            if (tier) economy->monthly_prestige += economy->defines.prestige_title[tier - 1];
        }
        struct realm_title_info vassals[64];
        size_t total, offset = 0;
        do {
            total = realm_direct_vassals(realms, ruler_id, offset, vassals, 64);
            for (size_t i = 0; i < 64 && i + offset < total; ++i) {
                const unsigned tier = title_rank(vassals[i].id);
                if (tier) economy->monthly_prestige += economy->defines.prestige_vassal[tier - 1];
            }
            offset += 64;
        } while (offset < total && offset < 8192);
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
                economy_snapshot_free(economy);
                return -1;
            }
        }
    }
    economy_snapshot_refresh(economy, construction);
    return 0;
}

void economy_snapshot_refresh(struct economy_snapshot *economy, const struct construction *state)
{
    if (!economy || !state) return;
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

void economy_snapshot_free(struct economy_snapshot *economy)
{
    if (!economy) return;
    free(economy->direct_holdings);
    *economy = (struct economy_snapshot){0};
}
