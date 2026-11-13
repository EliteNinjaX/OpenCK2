#include "rulers.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ruler_compare(const void *a, const void *b)
{
    const unsigned x = ((const struct ruler_profile *)a)->id, y = ((const struct ruler_profile *)b)->id;
    return (x > y) - (x < y);
}

static int date_value(struct token key)
{
    char value[24], extra;
    int y, m, d;
    copy_token(value, sizeof value, key);
    return sscanf(value, "%4d.%2d.%2d%c", &y, &m, &d, &extra) == 3 ? y * 10000 + m * 100 + d : 0;
}

const char *const ruler_stat_keys[RULER_STAT_COUNT] = {
    "diplomacy", "martial", "stewardship", "intrigue", "learning"
};

static void read_stats(struct ruler_profile *ruler, const char *block, int when, int *effective)
{
    for (size_t i = 0; i < RULER_STAT_COUNT; ++i) {
        struct token value = field(block, ruler_stat_keys[i]);
        char raw[24], *end;
        if (!value.size || value.size >= sizeof raw || when < effective[i])
            continue;
        copy_token(raw, sizeof raw, value);
        errno = 0;
        const long number = strtol(raw, &end, 10);
        if (*end || errno || number < INT_MIN || number > INT_MAX)
            continue;
        ruler->stats[i] = (int)number;
        ruler->known_stats |= 1u << i;
        effective[i] = when;
    }
}

struct authored_trait {
    char key[64];
    int when;
    bool active;
};

static void read_traits(const char *p, int when, struct authored_trait *traits, size_t *count)
{
    for (struct token key; (key = token(&p)).size && !is(key, "}");) {
        if (!is(token(&p), "="))
            break;
        const struct token value = token(&p);
        if (is(value, "{")) {
            skip_block(&p);
            continue;
        }
        const bool add = is(key, "trait") || is(key, "add_trait");
        if ((!add && !is(key, "remove_trait")) || !value.size || value.size >= sizeof traits[0].key)
            continue;
        size_t i = 0;
        while (i < *count && !is(value, traits[i].key))
            ++i;
        if (i == *count) {
            if (i == RULER_TRAIT_LIMIT)
                continue;
            copy_token(traits[i].key, sizeof traits[i].key, value);
            ++*count;
        }
        if (when >= traits[i].when) {
            traits[i].when = when;
            traits[i].active = add;
        }
    }
}

struct historical_claim {
    struct ruler_claim claim;
    int date;
    bool active;
};

static void read_claims(const char *p, int date, struct historical_claim *claims, size_t *count, bool effects)
{
    for (struct token key; (key = token(&p)).size && !is(key, "}");) {
        if (!is(token(&p), "="))
            break;
        const struct token value = token(&p);
        if (is(value, "{")) {
            if (effects && is(key, "effect"))
                read_claims(p, date, claims, count, false);
            skip_block(&p);
            continue;
        }
        const bool weak = is(key, "add_weak_claim");
        const bool add = weak || is(key, "add_claim") || is(key, "add_strong_claim");
        if ((!add && !is(key, "remove_claim")) || value.size < 3 ||
            value.size >= sizeof claims[0].claim.title || value.start[1] != '_')
            continue;
        size_t i = 0;
        while (i < *count && !is(value, claims[i].claim.title))
            ++i;
        if (i == *count) {
            if (i == RULER_CLAIM_LIMIT)
                continue;
            copy_token(claims[i].claim.title, sizeof claims[i].claim.title, value);
            ++*count;
        }
        if (date >= claims[i].date) {
            claims[i].date = date;
            claims[i].active = add;
            claims[i].claim.weak = weak;
        }
    }
}

static void read_resources(struct ruler_profile *ruler, const char *block, int when, int *effective)
{
    static const char *const keys[] = {"wealth", "prestige", "piety"};
    for (size_t i = 0; i < SDL_arraysize(keys); ++i) {
        const struct token value = field(block, keys[i]);
        char raw[32], *end;
        if (!value.size || value.size >= sizeof raw || when < effective[i])
            continue;
        copy_token(raw, sizeof raw, value);
        errno = 0;
        const double amount = strtod(raw, &end);
        if (*end || errno || !isfinite(amount))
            continue;
        ruler->resources[i] = amount;
        ruler->known_resources |= 1u << i;
        effective[i] = when;
    }
}

static void read_character(struct ruler_profile *r, const char *block, int cutoff)
{
    copy_token(r->name, sizeof r->name, field(block, "name"));
    copy_token(r->culture_key, sizeof r->culture_key, field(block, "culture"));
    copy_token(r->religion_key, sizeof r->religion_key, field(block, "religion"));
    copy_token(r->government_key, sizeof r->government_key, field(block, "government"));
    copy_token(r->dna, sizeof r->dna, field(block, "dna"));
    struct token properties = field(block, "prp");
    if (!properties.size)
        properties = field(block, "properties");
    copy_token(r->properties, sizeof r->properties, properties);
    r->female = is(field(block, "female"), "yes");
    r->age = 0;
    r->age_known = false;
    r->dynasty_id = number(field(block, "dynasty"));
    r->father = number(field(block, "father"));
    r->mother = number(field(block, "mother"));
    r->spouse = 0;
    r->deceased = false;
    r->government_inferred = false;
    struct authored_trait traits[RULER_TRAIT_LIMIT] = {0};
    size_t trait_count = 0;
    read_traits(block, 0, traits, &trait_count);
    struct historical_claim claims[RULER_CLAIM_LIMIT] = {0};
    size_t claim_count = 0;
    read_claims(block, 0, claims, &claim_count, true);
    r->known_resources = 0;
    int resource_dates[RULER_RESOURCE_COUNT] = {0};
    read_resources(r, block, 0, resource_dates);
    r->known_stats = 0;
    int stat_dates[RULER_STAT_COUNT] = {0};
    read_stats(r, block, 0, stat_dates);
    int culture = 0, religion = 0, dynasty = 0, marriage = 0, government = 0;
    for (struct token key; (key = token(&block)).size && !is(key, "}");) {
        if (!is(token(&block), "="))
            break;
        if (!is(token(&block), "{"))
            continue;
        const int when = date_value(key);
        if (when && when <= cutoff) {
            read_stats(r, block, when, stat_dates);
            read_traits(block, when, traits, &trait_count);
            read_claims(block, when, claims, &claim_count, true);
            read_resources(r, block, when, resource_dates);
            if (field(block, "birth").size) {
                const int year = cutoff / 10000 - when / 10000;
                r->age_known = true;
                r->age = (unsigned)SDL_max(0, year - (cutoff % 10000 < when % 10000));
            }
            r->deceased |= field(block, "death").size != 0;
            const unsigned spouse = number(field(block, "add_spouse"));
            const unsigned divorce = number(field(block, "remove_spouse"));
            if (when >= marriage && (spouse || (divorce && divorce == r->spouse))) {
                marriage = when;
                r->spouse = spouse;
            }
            struct token t = field(block, "culture");
            if (t.size && when >= culture) {
                culture = when;
                copy_token(r->culture_key, sizeof r->culture_key, t);
            }
            t = field(block, "religion");
            if (t.size && when >= religion) {
                religion = when;
                copy_token(r->religion_key, sizeof r->religion_key, t);
            }
            t = field(block, "government");
            if (t.size && when >= government) {
                government = when;
                copy_token(r->government_key, sizeof r->government_key, t);
            }
            t = field(block, "dynasty");
            if (t.size && when >= dynasty) {
                dynasty = when;
                r->dynasty_id = number(t);
            }
        }
        skip_block(&block);
    }
    r->claim_count = 0;
    for (size_t i = 0; i < claim_count; ++i)
        if (claims[i].active)
            r->claims[r->claim_count++] = claims[i].claim;
    r->trait_count = 0;
    for (size_t i = 0; i < trait_count; ++i)
        if (traits[i].active) {
            SDL_strlcpy(r->traits[r->trait_count], traits[i].key, sizeof r->traits[0]);
            ++r->trait_count;
        }
}

static void read_files(struct ruler_profile *rulers, size_t count, const char *root,
                       const char *dir, int cutoff, bool dynasty)
{
    struct ck2_asset_files files = ck2_list_assets(root, dir);
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 4, ".txt"))
            continue;
        char *data = load_from_directory(root, dir, name);
        const char *p = data;
        if (p && !strncmp(p, "\xef\xbb\xbf", 3))
            p += 3;
        while (p && *p) {
            struct token key = token(&p);
            if (!key.size || !is(token(&p), "="))
                break;
            if (!is(token(&p), "{"))
                continue;
            const unsigned id = number(key);
            if (id && dynasty) {
                const struct token family = field(p, "name");
                for (size_t j = 0; j < count; ++j)
                    if (rulers[j].dynasty_id == id)
                        copy_token(rulers[j].dynasty, sizeof rulers[j].dynasty, family);
            } else if (id) {
                struct ruler_profile match = {.id = id};
                struct ruler_profile *r = bsearch(&match, rulers, count, sizeof *r, ruler_compare);
                if (r)
                    read_character(r, p, cutoff);
            }
            skip_block(&p);
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
}

void rulers_family_ids(const char *root, const struct ruler_profile *ruler, struct campaign_date date,
                       unsigned *children, size_t child_limit, unsigned *siblings, size_t sibling_limit)
{
    const int cutoff = date.year * 10000 + date.month * 100 + date.day;
    size_t child_count = 0, sibling_count = 0;
    const char *directory = "history/characters";
    struct ck2_asset_files files = ck2_list_assets(root, directory);
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t length = strlen(name);
        if (length < 4 || strcmp(name + length - 4, ".txt"))
            continue;
        char *data = load_from_directory(root, directory, name);
        const char *p = data;
        if (p && !strncmp(p, "\xef\xbb\xbf", 3))
            p += 3;
        for (struct token key; p && (key = token(&p)).size;) {
            if (!is(token(&p), "="))
                break;
            if (!is(token(&p), "{"))
                continue;
            const unsigned id = number(key);
            const unsigned father = number(field(p, "father")), mother = number(field(p, "mother"));
            const bool child = id && id != ruler->id && (father == ruler->id || mother == ruler->id);
            const bool sibling = id && id != ruler->id &&
                ((father && father == ruler->father) || (mother && mother == ruler->mother));
            if (child || sibling) {
                struct ruler_profile relative = {.id = id};
                read_character(&relative, p, cutoff);
                if (relative.age_known) {
                    if (child && child_count < child_limit)
                        children[child_count++] = id;
                    if (sibling && sibling_count < sibling_limit)
                        siblings[sibling_count++] = id;
                }
            }
            skip_block(&p);
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
}

static void read_identity_definitions(struct ruler_profile *rulers, size_t count, const char *root,
                                      bool culture)
{
    const char *directory = culture ? "common/cultures" : "common/religions";
    struct ck2_asset_files files = ck2_list_assets(root, directory);
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.names[i];
        const size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 4, ".txt"))
            continue;
        char *data = load_from_directory(root, directory, name);
        const char *p = data;
        if (p && !strncmp(p, "\xef\xbb\xbf", 3))
            p += 3;
        while (p && *p) {
            struct token group = token(&p);
            if (!group.size || !is(token(&p), "="))
                break;
            if (!is(token(&p), "{"))
                continue;
            const bool group_default = is(field(p, "dynasty_title_names"), "yes");
            for (struct token key; (key = token(&p)).size && !is(key, "}");) {
                if (!is(token(&p), "="))
                    break;
                if (!is(token(&p), "{"))
                    continue;
                const struct token option = field(p, culture ? "dynasty_title_names" : "icon");
                const unsigned icon = culture ? 0 : number(option);
                for (size_t j = 0; j < count; ++j) {
                    if (culture && is(key, rulers[j].culture_key))
                        rulers[j].dynasty_titles = option.size ? is(option, "yes") : group_default;
                    else if (!culture && icon && is(key, rulers[j].religion_key))
                        rulers[j].religion_icon = icon;
                }
                skip_block(&p);
            }
        }
        SDL_free(data);
    }
    ck2_free_asset_files(&files);
}

void rulers_load(struct ruler_profile *rulers, size_t count, const char *root, struct campaign_date date)
{
    if (!count)
        return;
    const int cutoff = date.year * 10000 + date.month * 100 + date.day;
    read_files(rulers, count, root, "history/characters", cutoff, false);
    read_files(rulers, count, root, "common/dynasties", cutoff, true);
    read_identity_definitions(rulers, count, root, true);
    read_identity_definitions(rulers, count, root, false);
    struct ck2_translation *terms = malloc(2 * count * sizeof *terms);
    if (terms) {
        for (size_t i = 0; i < count; ++i) {
            struct ruler_profile *r = &rulers[i];
            terms[2 * i] = (struct ck2_translation){r->culture_key, r->culture, sizeof r->culture};
            terms[2 * i + 1] = (struct ck2_translation){r->religion_key, r->religion, sizeof r->religion};
        }
        ck2_translate(root, terms, 2 * count);
        free(terms);
    }
    for (size_t i = 0; i < count; ++i) {
        if (!rulers[i].culture[0])
            SDL_strlcpy(rulers[i].culture, rulers[i].culture_key, sizeof rulers[i].culture);
        if (!rulers[i].religion[0])
            SDL_strlcpy(rulers[i].religion, rulers[i].religion_key, sizeof rulers[i].religion);
    }
}
