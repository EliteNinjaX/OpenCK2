#include "holdings.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum holding_type type_for(struct token value)
{
    static const char *const types[] = {"", "castle", "city", "temple", "tribal", "nomad"};
    for (int i = 1; i < HOLDING_TYPE_COUNT; ++i)
        if (is(value, types[i]))
            return (enum holding_type)i;
    return HOLDING_EMPTY;
}

static void update(struct county_holdings *county, struct token key, struct token value, int when)
{
    if (is(key, "max_settlements")) {
        if (when >= county->capacity_date) {
            const unsigned capacity = number(value);
            county->capacity = SDL_min(capacity, (unsigned)HOLDING_LIMIT);
            county->capacity_date = when;
        }
        return;
    }
    if (is(key, "capital")) {
        if (when >= county->capital_date) {
            copy_token(county->capital, sizeof county->capital, value);
            county->capital_date = when;
        }
        return;
    }
    if (is(key, "remove_settlement")) {
        key = value;
        value = (struct token){"none", 4};
    }
    if (key.size < 3 || key.start[0] != 'b' || key.start[1] != '_')
        return;
    const enum holding_type type = type_for(value);
    if (!type && !is(value, "none") && !is(value, "no"))
        return;
    size_t i = 0;
    while (i < county->count && !is(key, county->slots[i].id))
        ++i;
    if (i == county->count) {
        if (i == HOLDING_LIMIT)
            return;
        ++county->count;
        copy_token(county->slots[i].id, sizeof county->slots[i].id, key);
    }
    if (when >= county->slots[i].effective) {
        county->slots[i].effective = when;
        county->slots[i].type = type;
    }
}

void holdings_load(struct county_holdings *county, const char *root, unsigned province,
                    struct campaign_date date)
{
    *county = (struct county_holdings){0};
    if (!province)
        return;
    struct ck2_asset_files files = ck2_list_assets(root, "history/provinces");
    char *data = NULL;
    for (size_t i = 0; i < files.count; ++i) {
        char *end;
        if (strtoul(files.names[i], &end, 10) == province && *end == ' ') {
            SDL_free(data);
            data = load_from_directory(root, "history/provinces", files.names[i]);
        }
    }
    ck2_free_asset_files(&files);
    const int cutoff = date.year * 10000 + date.month * 100 + date.day;
    for (const char *p = data; p && *p;) {
        struct token key = token(&p);
        if (!key.size || !is(token(&p), "="))
            break;
        const struct token value = token(&p);
        if (!is(value, "{")) {
            update(county, key, value, 0);
            continue;
        }
        char raw[24];
        copy_token(raw, sizeof raw, key);
        int y, m, d;
        if (sscanf(raw, "%4d.%2d.%2d", &y, &m, &d) == 3) {
            const int when = y * 10000 + m * 100 + d;
            const char *block = p;
            if (when <= cutoff) {
                for (struct token k; (k = token(&block)).size && !is(k, "}");) {
                    if (!is(token(&block), "="))
                        break;
                    struct token v = token(&block);
                    if (is(v, "{"))
                        skip_block(&block);
                    else
                        update(county, k, v, when);
                }
            }
        }
        skip_block(&p);
    }
    SDL_free(data);
    size_t count = 0;
    for (size_t i = 0; i < county->count; ++i)
        if (county->slots[i].type)
            county->slots[count++] = county->slots[i];
    county->count = count;
    for (size_t i = 1; i < count; ++i) {
        if (strcmp(county->slots[i].id, county->capital))
            continue;
        const struct holding capital = county->slots[i];
        memmove(county->slots + 1, county->slots, i * sizeof *county->slots);
        county->slots[0] = capital;
        break;
    }
    county->capacity = SDL_max(count, county->capacity);
    struct ck2_translation terms[HOLDING_LIMIT];
    for (size_t i = 0; i < count; ++i) {
        struct holding *slot = &county->slots[i];
        terms[i] = (struct ck2_translation){slot->id, slot->name, sizeof slot->name};
    }
    if (count)
        ck2_translate(root, terms, count);
}
