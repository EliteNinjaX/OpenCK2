#include "asset_paths.h"
#include "portraits.h"
#include "rulers.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static Uint32 render_hash(const struct portrait_source *source, SDL_Renderer *renderer,
                          const char *root, const struct ruler_profile *profile)
{
    struct era_ruler ruler = {.id = profile->id, .age = profile->age, .female = profile->female,
                              .age_known = profile->age_known, .portrait_year = 936};
    SDL_strlcpy(ruler.dna, profile->dna, sizeof ruler.dna);
    SDL_strlcpy(ruler.properties, profile->properties, sizeof ruler.properties);
    SDL_strlcpy(ruler.culture, profile->culture_key, sizeof ruler.culture);
    SDL_strlcpy(ruler.religion, profile->religion_key, sizeof ruler.religion);
    SDL_Texture *texture = portrait_render(source, renderer, root, &ruler);
    assert(texture);
    assert(SDL_RenderClear(renderer) == 0);
    assert(SDL_RenderCopy(renderer, texture, NULL, NULL) == 0);
    Uint32 pixels[152 * 152], hash = 2166136261u;
    assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_BGRA32, pixels, 152 * 4) == 0);
    for (size_t i = 0; i < SDL_arraysize(pixels); ++i)
        hash = (hash ^ pixels[i]) * 16777619u;
    SDL_DestroyTexture(texture);
    return hash;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: portraits-test CK2_DIRECTORY\n");
        return 1;
    }
    struct ruler_profile profiles[] = {
        {.id = 1282}, {.id = 1283}, {.id = 1284}, {.id = 7953}, {.id = 7992}, {.id = 10097},
        {.id = 144958}
    };
    struct content_catalog catalog = {0};
    ck2_catalog_load(&catalog, argv[1]);
    ck2_mount_content(argv[1], &catalog);
    rulers_load(profiles, SDL_arraysize(profiles), argv[1], (struct campaign_date){936, 8, 7});
    assert(!strcmp(profiles[0].dna, "rcrfdblvsvv"));
    assert(!strcmp(profiles[0].properties, "aj000a00000000000000000000000000000000"));
    assert(profiles[0].age_known && profiles[0].age == 23 && !profiles[0].female);
    assert(profiles[1].female && profiles[1].age_known && profiles[1].age == 22);
    assert(profiles[2].female && profiles[2].age_known && profiles[2].age == 5);
    for (size_t i = 1; i < SDL_arraysize(profiles); ++i)
        assert(profiles[i].name[0] && !profiles[i].dna[0]);

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 152, 152, 32, SDL_PIXELFORMAT_BGRA32);
    assert(surface);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    struct portrait_source source = {0};
    portraits_open(&source, argv[1]);
    Uint32 hashes[SDL_arraysize(profiles)];
    for (size_t i = 0; i < SDL_arraysize(profiles); ++i)
        hashes[i] = render_hash(&source, renderer, argv[1], &profiles[i]);
    assert(hashes[4] == render_hash(&source, renderer, argv[1], &profiles[4]));
    assert(hashes[3] != hashes[4]);
    struct ruler_profile adult = profiles[2];
    adult.age = 22;
    assert(hashes[2] != render_hash(&source, renderer, argv[1], &adult));
    struct ruler_profile without_prp = profiles[0];
    without_prp.properties[0] = 0;
    assert(hashes[0] != render_hash(&source, renderer, argv[1], &without_prp));
    portraits_close(&source);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    ck2_unmount_content();
    ck2_catalog_free(&catalog);
    puts("Verified Otto's historical properties and rendered 936 vassal, female, and child portraits.");
    return 0;
}
