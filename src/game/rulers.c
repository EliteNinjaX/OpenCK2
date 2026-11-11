#include "rulers.h"
#include "ck2_script.h"
#include "../open_launcher/asset_paths.h"
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

static void read_character(struct ruler_profile *r, const char *block, int cutoff)
{
    copy_token(r->name, sizeof r->name, field(block, "name"));
    copy_token(r->culture_key, sizeof r->culture_key, field(block, "culture"));
    copy_token(r->religion_key, sizeof r->religion_key, field(block, "religion"));
    copy_token(r->dna, sizeof r->dna, field(block, "dna"));
    copy_token(r->properties, sizeof r->properties, field(block, "properties"));
    r->dynasty_id = number(field(block, "dynasty"));
    int culture = 0, religion = 0, dynasty = 0;
    for (struct token key; (key = token(&block)).size && !is(key, "}");) {
        if (!is(token(&block), "="))
            break;
        if (!is(token(&block), "{"))
            continue;
        const int when = date_value(key);
        if (when && when <= cutoff) {
            if (field(block, "birth").size) {
                const int year = cutoff / 10000 - when / 10000;
                r->age = (unsigned)SDL_max(0, year - (cutoff % 10000 < when % 10000));
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
            t = field(block, "dynasty");
            if (t.size && when >= dynasty) {
                dynasty = when;
                r->dynasty_id = number(t);
            }
        }
        skip_block(&block);
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
