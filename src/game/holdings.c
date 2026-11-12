#include "holdings.h"
#include "ck2_script.h"
#include <stdio.h>
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
    if (is(key, "culture") && when >= county->culture_date) {
        copy_token(county->culture_key, sizeof county->culture_key, value);
        county->culture_date = when;
        return;
    }
    if (is(key, "religion") && when >= county->religion_date) {
        copy_token(county->religion_key, sizeof county->religion_key, value);
        county->religion_date = when;
        return;
    }
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
    const bool settlement = type || is(value, "none") || is(value, "no");
    /* A b_ assignment can designate either the holding's type or an already
     * built improvement. CK2 province history uses both forms. */
    if (!settlement && (!value.size || value.size >= sizeof county->slots[0].buildings[0]))
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
    if (!settlement) {
        /* A building alone cannot create a new holding of unknown type. The
         * assignment is kept until the holding's type is resolved. */
        struct holding *slot = &county->slots[i];
        for (size_t b = 0; b < slot->building_count; ++b)
            if (is(value, slot->buildings[b])) return;
        if (slot->building_count < HOLDING_BUILDING_LIMIT)
            copy_token(slot->buildings[slot->building_count++], sizeof slot->buildings[0], value);
    } else if (when >= county->slots[i].effective) {
        county->slots[i].effective = when;
        county->slots[i].type = type;
        if (!type) county->slots[i].building_count = 0;
    }
}

void holdings_read(struct county_holdings *county, const char *data, struct campaign_date date)
{
    *county = (struct county_holdings){0};
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
}

void holdings_load(struct county_holdings *county, const char *root, const struct province *province,
                    struct campaign_date date)
{
    char *data = province_history_read(province, root);
    holdings_read(county, data, date);
    SDL_free(data);
    const size_t count = county->count;
    struct ck2_translation terms[HOLDING_LIMIT + 2];
    for (size_t i = 0; i < count; ++i) {
        struct holding *slot = &county->slots[i];
        terms[i] = (struct ck2_translation){slot->id, slot->name, sizeof slot->name};
    }
    size_t translated = count;
    if (county->culture_key[0]) {
        SDL_strlcpy(county->culture_name, county->culture_key, sizeof county->culture_name);
        terms[translated++] = (struct ck2_translation){county->culture_key,
                                 county->culture_name, sizeof county->culture_name};
    }
    if (county->religion_key[0]) {
        SDL_strlcpy(county->religion_name, county->religion_key, sizeof county->religion_name);
        terms[translated++] = (struct ck2_translation){county->religion_key,
                                 county->religion_name, sizeof county->religion_name};
    }
    if (translated)
        ck2_translate(root, terms, translated);
}
