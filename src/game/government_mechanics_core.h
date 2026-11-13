#ifndef OPENCK2_GOVERNMENT_MECHANICS_CORE_H
#define OPENCK2_GOVERNMENT_MECHANICS_CORE_H
#include <stdbool.h>
#include <stddef.h>

/* Native OpenCK2 simulation records, NOT CK2's undocumented in-memory layout.
 * All constants below that affect calculations come from common/defines.lua.
 * Absence of a required definition disables the corresponding action. */
enum { CK2_CLAN_LIMIT = 9, CK2_PATRICIAN_LIMIT = 5,
       CK2_TRADE_POST_LIMIT = 128, CK2_BISHOP_LIMIT = 128,
       CK2_TRADE_BUILDING_LIMIT = 8 };
struct ck2_government_rules {
    double nomad_growth, nomad_start, nomad_max_manpower, nomad_tax;
    double nomad_max_population_empty_holding;
    double respect_age, respect_prestige, respect_fund, respect_random;
    double trade_post_distance_cost;
    unsigned max_clans, provinces_per_clan, max_patricians, min_trade_posts;
    bool nomad_loaded, merchant_loaded;
};
bool ck2_government_rules_parse(struct ck2_government_rules *, const char *defines_lua);

struct ck2_trade_building {
    char key[72], upgrades_from[72];
    double gold_cost, tradevalue, tax_income;
    unsigned build_days;
    bool has_cost, has_days;
};
struct ck2_trade_catalog {
    struct ck2_trade_building *items;
    size_t count, capacity;
};
bool ck2_trade_catalog_parse(struct ck2_trade_catalog *, const char *source);
const struct ck2_trade_building *ck2_trade_find(const struct ck2_trade_catalog *, const char *key);
void ck2_trade_catalog_free(struct ck2_trade_catalog *);

struct ck2_nomad_clan {
    unsigned head, provinces;
    double population, capacity, manpower;
};
struct ck2_patrician_family {
    unsigned head;
    double campaign_fund;
};
struct ck2_trade_post {
    unsigned province, patrician;
    char completed[CK2_TRADE_BUILDING_LIMIT][72];
    size_t completed_count;
    char current_build[72];
    unsigned days_left;
};
struct ck2_bishop_nomination {
    char temple[72];
    unsigned liege, candidate;
    bool papal_approval_required, approved;
};
struct ck2_government_mechanics {
    struct ck2_nomad_clan clans[CK2_CLAN_LIMIT];
    struct ck2_patrician_family families[CK2_PATRICIAN_LIMIT];
    struct ck2_trade_post posts[CK2_TRADE_POST_LIMIT];
    struct ck2_bishop_nomination bishops[CK2_BISHOP_LIMIT];
    size_t clan_count, family_count, post_count, bishop_count;
};

enum ck2_government_action {
    CK2_GOV_OK, CK2_GOV_MISSING_RULES, CK2_GOV_INVALID, CK2_GOV_DUPLICATE,
    CK2_GOV_LIMIT, CK2_GOV_NO_RESOURCE, CK2_GOV_UNSUPPORTED, CK2_GOV_NO_RECORD
};
const char *ck2_government_action_text(enum ck2_government_action);
/* Register only after the campaign layer establishes the ruler's government
 * and actual domain. Capacity and province count are explicit evidence; there
 * is deliberately no fabricated CK2 population for an unverified holding. */
enum ck2_government_action ck2_nomad_add(struct ck2_government_mechanics *,
    const struct ck2_government_rules *, unsigned head, unsigned provinces, double capacity);
enum ck2_government_action ck2_nomad_split(struct ck2_government_mechanics *,
    const struct ck2_government_rules *, unsigned head, unsigned new_head,
    unsigned provinces_to_new, double population_to_new);
/* Called once per completed month; CK2 population tick frequency and terrain
 * bonuses need more research, so this is labeled a provisional extrapolation. */
void ck2_nomad_month(struct ck2_government_mechanics *, const struct ck2_government_rules *);
double ck2_nomad_monthly_tax(const struct ck2_government_mechanics *,
                            const struct ck2_government_rules *, unsigned head);

enum ck2_government_action ck2_patrician_add(struct ck2_government_mechanics *,
    const struct ck2_government_rules *, unsigned head);
enum ck2_government_action ck2_patrician_fund(struct ck2_government_mechanics *,
    unsigned head, double gold);
/* CK2 documented Respect subtotal. Missing prestige/age must be rejected by
 * the caller; election random factor and other vote modifiers are NOT applied. */
bool ck2_patrician_respect(const struct ck2_government_rules *, unsigned age,
    double prestige, double campaign_fund, double *out);
/* A deterministic leading subtotal is NOT an actual doge-election winner. */
unsigned ck2_patrician_respect_leader(const struct ck2_government_mechanics *,
    const struct ck2_government_rules *, const unsigned *ages,
    const double *prestiges, const bool *known);

enum ck2_government_action ck2_trade_build(struct ck2_government_mechanics *,
    const struct ck2_trade_catalog *, unsigned patrician, unsigned province,
    const char *building, bool verified_port, double *gold_cost);
void ck2_trade_day(struct ck2_government_mechanics *);
double ck2_trade_value(const struct ck2_trade_post *, const struct ck2_trade_catalog *);
double ck2_trade_annual_tax(const struct ck2_trade_post *, const struct ck2_trade_catalog *);
/* The investiture law must be explicitly sourced from a dated title law or
 * a developer override. Catholic papal nominations require later approval. */
enum ck2_government_action ck2_bishop_nominate(struct ck2_government_mechanics *,
    const char *temple, unsigned liege, unsigned candidate,
    const char *investiture_law);
enum ck2_government_action ck2_bishop_approve(struct ck2_government_mechanics *,
    const char *temple, bool papal_authority);
/* Reject malformed or corrupt checkpoint states before live state mutation. */
bool ck2_government_mechanics_valid(const struct ck2_government_mechanics *);
#endif
