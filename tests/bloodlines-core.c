#include "../src/game/bloodline.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {fclose(file);return NULL;}
    const long n = ftell(file);
    if (n < 0 || n > 12000000 || fseek(file, 0, SEEK_SET) != 0) {fclose(file);return NULL;}
    char *data = malloc((size_t)n + 1);
    if (!data) {fclose(file);return NULL;}
    if (fread(data, 1, (size_t)n, file) != (size_t)n) {free(data);fclose(file);return NULL;}
    data[n] = 0;
    fclose(file);
    return data;
}

int main(int argc, char **argv)
{
    ck2_bloodline_registry registry = {0};
    const char script[] = "test_line = { inheritance = patrilineal picture = GFX_bloodlines_symbol_lion flags = { historical_bloodline } }";
    assert(ck2_bloodlines_parse_text(&registry, script));
    assert(registry.count == 1);
    assert(ck2_bloodline_find(&registry, "test_line"));
    assert(ck2_bloodline_find(&registry, "test_line")->inheritance == CK2_BLOOD_PATRILINEAL);
    ck2_bloodlines_free(&registry);
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            char *text = read_file(argv[i]);
            assert(text != NULL);
            if (!ck2_bloodlines_parse_text(&registry, text)) {
                fprintf(stderr, "failed to parse bloodline definitions: %s\n", argv[i]);
                abort();
            }
            free(text);
        }
        assert(registry.count > 10);
        assert(ck2_bloodline_find(&registry, "carolingian"));
        assert(ck2_bloodline_find(&registry, "william_the_conqueror"));
        printf("bloodline definitions parsed: %zu\n", registry.count);
    }
    ck2_bloodlines_free(&registry);
    puts("bloodlines core: all assertions passed");
    return 0;
}
