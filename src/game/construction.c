#include "construction.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"

#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct building_source {
    struct building_source *next;
    char *data;
};

struct building_project {
    char holding[72];
    size_t index;
    unsigned days;
};

struct settlement_project {
    struct holding holding;
    unsigned province, days;
};

static const char *const type_ids[] = {"", "castle", "city", "temple", "tribal", "nomad"};

static unsigned positive_integer(struct token value)
{
    unsigned result = 0;
    for (size_t i = 0; i < value.size; ++i) {
        const int digit = value.start[i] - '0';
        if (digit < 0 || digit > 9 || result > (UINT_MAX - (unsigned)digit) / 10)
            return 0;
        result = result * 10 + (unsigned)digit;
    }
    return result;
}

static bool parse_real(struct token value, double *out)
{
    char raw[64], *end;
    if (!value.size || value.size >= sizeof raw) return false;
    copy_token(raw, sizeof raw, value);
    errno = 0;
    double amount = strtod(raw, &end);
    if (errno || *end || !isfinite(amount)) return false;
    *out = amount;
    return true;
}

static bool zero_tech(const char *p)
{
    for (struct token key; (key = token(&p)).size && !is(key, "}");) {
        if (key.size <= 5 || memcmp(key.start, "TECH_", 5) ||
            !is(token(&p), "=") || !is(token(&p), "0"))
            return false;
    }
    return true;
}

static bool read_buildings(struct construction *state, const char *p)
{
    for (struct token group; (group = token(&p)).size;) {
        if (!is(token(&p), "=") || !is(token(&p), "{"))
            break;
        enum holding_type type = HOLDING_EMPTY;
        for (int i = 1; i < HOLDING_TYPE_COUNT; ++i)
            if (is(group, type_ids[i]))
                type = (enum holding_type)i;
        if (!type) {
            skip_block(&p);
            continue;
        }
        for (struct token id; (id = token(&p)).size && !is(id, "}");) {
            if (!is(token(&p), "="))
                return true;
            if (!is(token(&p), "{"))
                continue;
            const char *body = p;
            skip_block(&p);
            struct building building = {.type = type, .supported = true, .requirements = body};
            copy_token(building.id, sizeof building.id, id);
            while (body < p) {
                const struct token key = token(&body);
                if (!key.size || is(key, "}") || !is(token(&body), "="))
                    break;
                const struct token value = token(&body);
                if (is(key, "gold_cost"))
                    building.gold = positive_integer(value);
                else if (is(key, "build_time"))
                    building.days = positive_integer(value);
                else if (is(key, "tax_income"))
                    (void)parse_real(value, &building.annual_tax);
                else if (is(key, "upgrades_from"))
                    copy_token(building.upgrades_from, sizeof building.upgrades_from, value);
                else if (is(key, "trigger"))
                    building.supported &= is(value, "{") && zero_tech(body);
                else if (is(key, "potential") || is(key, "is_active_trigger") ||
                         is(key, "prestige_cost") || is(key, "piety_cost") ||
                         is(key, "replaces") || is(key, "convert_to"))
                    building.supported = false;
                if (is(value, "{"))
                    skip_block(&body);
            }
            size_t i = 0;
            while (i < state->count && !is(id, state->catalog[i].id))
                ++i;
            if (i == state->count) {
                struct building *catalog = realloc(state->catalog, (i + 1) * sizeof *catalog);
                if (!catalog)
                    return false;
                state->catalog = catalog;
                ++state->count;
            }
            state->catalog[i] = building;
        }
    }
    return true;
}

static bool read_define(const char *data, const char *name, unsigned *per_mille)
{
    const size_t length = strlen(name);
    for (const char *line = data; line && *line;) {
        const char *end = strchr(line, '\n');
        const char *p = line;
        while (*p == ' ' || *p == '\t')
            ++p;
        if (!strncmp(p, name, length)) {
            p += length;
            while (*p == ' ' || *p == '\t')
                ++p;
            if (*p == '=') {
                ++p;
                while (*p == ' ' || *p == '\t')
                    ++p;
                const char *begin = p;
                while (isdigit((unsigned char)*p))
                    ++p;
                if (begin == p)
                    return false;
                const unsigned integer = positive_integer((struct token){begin, (size_t)(p - begin)});
                if (integer > 100)
                    return false;
                *per_mille = integer * 1000;
                if (*p == '.') {
                    ++p;
                    for (unsigned place = 100; place && isdigit((unsigned char)*p); place /= 10)
                        *per_mille += (unsigned)(*p++ - '0') * place;
                }
                return true;
            }
        }
        line = end ? end + 1 : NULL;
    }
    return false;
}

static void read_settlement_rules(struct construction *state, const char *root)
{
    char *defines = load_data(root, "common/defines.lua");
    unsigned require_each = 0;
    state->settlement_rules_ready =
        read_define(defines, "HOLDING_SLOT_BUILD_COST_MOD", &state->slot_cost_per_mille) &&
        read_define(defines, "ENFORCE_ONE_OF_EACH_HOLDING", &require_each);
    state->require_each = require_each != 0;
    SDL_free(defines);

    char *modifiers = load_data(root, "common/static_modifiers.txt");
    char *types = load_data(root, "common/holding_types/00_holding_types.txt");
    for (int i = HOLDING_CASTLE; modifiers && i < HOLDING_TYPE_COUNT; ++i) {
        const char *tax = modifiers;
        if (is(find(&tax, type_ids[i]), "{")) {
            double value;
            if (parse_real(field(tax, "tax_income"), &value)) {
                state->holding_base_tax[i] = value;
                state->holding_base_tax_known[i] = true;
            }
        }
    }
    for (int i = HOLDING_CASTLE; modifiers && types && i <= HOLDING_TEMPLE; ++i) {
        const char *body = modifiers, *type = types;
        if (!is(find(&body, type_ids[i]), "{") || !is(find(&type, type_ids[i]), "{") ||
            !is(find(&type, "trigger"), "{") || !is(token(&type), "}"))
            continue;
        state->settlement_type_allowed[i] = true;
        state->settlement_gold[i] = positive_integer(field(body, "build_cost_modifier"));
        state->settlement_days[i] = positive_integer(field(body, "build_time_modifier"));
    }
    SDL_free(modifiers);
    SDL_free(types);
}

int construction_load(struct construction *state, const char *root)
{
    const char *directory = "common/buildings";
    struct ck2_asset_files files = ck2_list_assets(root, directory);
    bool ok = true;
    for (size_t i = 0; ok && i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t length = strlen(name);
        if (length < 5 || strcmp(name + length - 4, ".txt"))
            continue;
        struct building_source *source = malloc(sizeof *source);
        if (!source) {
            ok = false;
            break;
        }
        source->data = load_from_directory(root, directory, name);
        source->next = state->sources;
        state->sources = source;
        ok = source->data && read_buildings(state, source->data);
    }
    ck2_free_asset_files(&files);
    if (!ok || !state->count) {
        construction_free(state);
        return SDL_SetError("Could not load common/buildings");
    }
    read_settlement_rules(state, root);
    struct ck2_translation *translations = malloc(state->count * sizeof *translations);
    if (translations) {
        for (size_t i = 0; i < state->count; ++i) {
            struct building *b = &state->catalog[i];
            translations[i] = (struct ck2_translation){b->id, b->name, sizeof b->name};
        }
        ck2_translate(root, translations, state->count);
        free(translations);
    }
    return 0;
}

void construction_reset(struct construction *state)
{
    free(state->projects);
    state->projects = NULL;
    state->project_count = 0;
    free(state->settlements);
    state->settlements = NULL;
    state->settlement_count = 0;
}

void construction_free(struct construction *state)
{
    construction_reset(state);
    while (state->sources) {
        struct building_source *source = state->sources;
        state->sources = source->next;
        SDL_free(source->data);
        free(source);
    }
    free(state->catalog);
    *state = (struct construction){0};
}

static bool record_project(struct construction *state, const char *holding, size_t index, unsigned days)
{
    size_t count = state->project_count;
    struct building_project *projects = realloc(state->projects, (count + 1) * sizeof *projects);
    if (!projects)
        return false;
    state->projects = projects;
    projects[count] = (struct building_project){.index = index, .days = days};
    snprintf(projects[count].holding, sizeof projects[count].holding, "%s", holding);
    ++state->project_count;
    return true;
}

static bool completed(const struct construction *state, const char *holding, struct token id)
{
    for (size_t i = 0; i < state->project_count; ++i) {
        const struct building_project *project = &state->projects[i];
        if (!project->days && !strcmp(project->holding, holding) &&
            is(id, state->catalog[project->index].id))
            return true;
    }
    return false;
}

static bool meets_requirements(const struct construction *state, const struct building *building,
                               const char *holding)
{
    if (!building->supported || !building->gold || !building->days)
        return false;
    const char *p = building->requirements;
    for (struct token key; (key = token(&p)).size && !is(key, "}");) {
        if (!is(token(&p), "="))
            return false;
        struct token value = token(&p);
        if (is(key, "prerequisites") && is(value, "{")) {
            while ((value = token(&p)).size && !is(value, "}"))
                if (!completed(state, holding, value))
                    return false;
        } else if (is(key, "upgrades_from")) {
            if (!completed(state, holding, value))
                return false;
        } else if (is(value, "{")) {
            skip_block(&p);
        }
    }
    return true;
}

enum building_status construction_status(const struct construction *state, const struct holding *holding,
                                         size_t index, bool owns, unsigned gold, unsigned *days)
{
    *days = 0;
    bool busy = false;
    for (size_t i = 0; i < state->project_count; ++i) {
        const struct building_project *project = &state->projects[i];
        if (strcmp(project->holding, holding->id))
            continue;
        if (project->index == index) {
            *days = project->days;
            return project->days ? BUILD_RUNNING : BUILD_COMPLETE;
        }
        busy |= project->days != 0;
    }
    if (!owns)
        return BUILD_UNOWNED;
    const struct building *building = &state->catalog[index];
    if (building->type != holding->type || !meets_requirements(state, building, holding->id))
        return BUILD_REQUIREMENTS;
    if (busy)
        return BUILD_BUSY;
    return gold < building->gold ? BUILD_UNAFFORDABLE : BUILD_AVAILABLE;
}

bool construction_start(struct construction *state, const struct holding *holding, size_t index,
                        bool owns, unsigned *gold)
{
    unsigned days;
    if (index >= state->count ||
        construction_status(state, holding, index, owns, *gold, &days) != BUILD_AVAILABLE)
        return false;
    const struct building *building = &state->catalog[index];
    if (!record_project(state, holding->id, index, building->days))
        return false;
    *gold -= building->gold;
    return true;
}

void construction_day(struct construction *state)
{
    for (size_t i = 0; i < state->project_count; ++i)
        if (state->projects[i].days)
            --state->projects[i].days;
    for (size_t i = 0; i < state->settlement_count; ++i)
        if (state->settlements[i].days)
            --state->settlements[i].days;
}

static void read_history(struct construction *state, const char *p, int cutoff)
{
    for (struct token key; (key = token(&p)).size && !is(key, "}");) {
        if (!is(token(&p), "="))
            break;
        const struct token value = token(&p);
        if (is(value, "{")) {
            char date[24];
            int y, m, d;
            copy_token(date, sizeof date, key);
            if (cutoff && sscanf(date, "%4d.%2d.%2d", &y, &m, &d) == 3 &&
                y * 10000 + m * 100 + d <= cutoff)
                read_history(state, p, 0);
            skip_block(&p);
            continue;
        }
        if (key.size < 3 || key.size >= sizeof ((struct building_project *)0)->holding ||
            memcmp(key.start, "b_", 2))
            continue;
        char holding[72];
        copy_token(holding, sizeof holding, key);
        for (size_t i = 0; i < state->count; ++i)
            if (is(value, state->catalog[i].id) && !completed(state, holding, value)) {
                bool running = false;
                for (size_t j = 0; j < state->project_count; ++j)
                    if (state->projects[j].index == i && !strcmp(state->projects[j].holding, holding))
                        running = true;
                if (!running)
                    record_project(state, holding, i, 0);
                break;
            }
    }
}

void construction_history(struct construction *state, const char *data, struct campaign_date date)
{
    if (data)
        read_history(state, data, date.year * 10000 + date.month * 100 + date.day);
}

unsigned construction_settlement_pending(const struct construction *state, unsigned province,
                                         const struct holding **holding)
{
    for (size_t i = 0; i < state->settlement_count; ++i) {
        const struct settlement_project *project = &state->settlements[i];
        if (project->province == province && project->days) {
            if (holding)
                *holding = &project->holding;
            return project->days;
        }
    }
    return 0;
}

bool construction_settlement_quote(const struct construction *state, const struct county_holdings *county,
                                   unsigned province, enum holding_type type, unsigned *gold, unsigned *days)
{
    if (!state->settlement_rules_ready || type < HOLDING_CASTLE || type > HOLDING_TEMPLE ||
        !state->settlement_type_allowed[type] || !state->settlement_gold[type] ||
        !state->settlement_days[type] || !county->count ||
        county->count >= county->capacity || county->count >= HOLDING_LIMIT ||
        construction_settlement_pending(state, province, NULL))
        return false;
    bool present[HOLDING_TYPE_COUNT] = {0};
    for (size_t i = 0; i < county->count; ++i)
        present[county->slots[i].type] = true;
    if (state->require_each && present[type] &&
        (!present[HOLDING_CASTLE] || !present[HOLDING_CITY] || !present[HOLDING_TEMPLE]))
        return false;
    const Uint64 factor = 1000 + (Uint64)state->slot_cost_per_mille * (county->count - 1);
    const Uint64 cost = ((Uint64)state->settlement_gold[type] * factor + 999) / 1000;
    if (cost > UINT_MAX)
        return false;
    *gold = (unsigned)cost;
    *days = state->settlement_days[type];
    return true;
}

bool construction_settlement_start(struct construction *state, const struct county_holdings *county,
                                   unsigned province, const struct holding *holding, unsigned *gold)
{
    unsigned price, days;
    if (!holding->id[0] || !construction_settlement_quote(state, county, province, holding->type,
                                                         &price, &days) || *gold < price)
        return false;
    for (size_t i = 0; i < county->count; ++i)
        if (!strcmp(county->slots[i].id, holding->id))
            return false;
    for (size_t i = 0; i < state->settlement_count; ++i)
        if (!strcmp(state->settlements[i].holding.id, holding->id))
            return false;
    const size_t count = state->settlement_count;
    struct settlement_project *projects = realloc(state->settlements, (count + 1) * sizeof *projects);
    if (!projects)
        return false;
    state->settlements = projects;
    projects[count] = (struct settlement_project){.holding = *holding, .province = province, .days = days};
    ++state->settlement_count;
    *gold -= price;
    return true;
}

void construction_settlement_apply(const struct construction *state, unsigned province, struct county_holdings *county)
{
    for (size_t i = 0; i < state->settlement_count && county->count < county->capacity; ++i) {
        const struct settlement_project *project = &state->settlements[i];
        if (project->province != province || project->days)
            continue;
        size_t index = 0;
        while (index < county->count && strcmp(county->slots[index].id, project->holding.id))
            ++index;
        if (index == county->count)
            county->slots[county->count++] = project->holding;
    }
}

bool construction_owns_settlement(const struct construction *state, const char *holding)
{
    for (size_t i = 0; i < state->settlement_count; ++i)
        if (!state->settlements[i].days && !strcmp(state->settlements[i].holding.id, holding))
            return true;
    return false;
}

/* The most advanced member of an upgrade chain replaces its predecessor.
 * A zero tax_income means that building contributes no direct annual gold. */
static bool replaces(const struct construction *state, size_t later, size_t former)
{
    const struct building *next = &state->catalog[later];
    for (size_t depth = 0; depth < 64 && next->upgrades_from[0]; ++depth) {
        if (!strcmp(next->upgrades_from, state->catalog[former].id)) return true;
        const struct building *parent = NULL;
        for (size_t i = 0; i < state->count; ++i)
            if (!strcmp(state->catalog[i].id, next->upgrades_from)) {
                parent = &state->catalog[i];
                break;
            }
        if (!parent || parent == next) break;
        next = parent;
    }
    return false;
}

bool construction_holding_annual_tax(const struct construction *state,
                                     const struct holding *holding, double *result)
{
    if (!state || !holding || !result || holding->type <= HOLDING_EMPTY ||
        holding->type >= HOLDING_TYPE_COUNT || !state->holding_base_tax_known[holding->type])
        return false;
    size_t indices[HOLDING_BUILDING_LIMIT + 128], n = 0;
    for (size_t b = 0; b < holding->building_count; ++b) {
        for (size_t i = 0; i < state->count; ++i) {
            if (state->catalog[i].type != holding->type ||
                strcmp(state->catalog[i].id, holding->buildings[b])) continue;
            size_t k;
            for (k = 0; k < n && indices[k] != i; ++k) { }
            if (k == n && n < SDL_arraysize(indices)) indices[n++] = i;
            break;
        }
    }
    /* Construction in the running campaign is kept in the existing project
     * list. History-loaded projects will match the above IDs and deduplicate. */
    for (size_t p = 0; p < state->project_count; ++p) {
        const struct building_project *project = &state->projects[p];
        if (project->days || strcmp(project->holding, holding->id) ||
            project->index >= state->count || state->catalog[project->index].type != holding->type)
            continue;
        size_t k;
        for (k = 0; k < n && indices[k] != project->index; ++k) { }
        if (k == n && n < SDL_arraysize(indices)) indices[n++] = project->index;
    }
    double annual = state->holding_base_tax[holding->type];
    for (size_t k = 0; k < n; ++k) {
        bool upgraded = false;
        for (size_t j = 0; j < n; ++j)
            if (k != j && replaces(state, indices[j], indices[k])) {
                upgraded = true;
                break;
            }
        if (!upgraded) annual += state->catalog[indices[k]].annual_tax;
    }
    *result = annual < 0 ? 0 : annual;
    return true;
}
