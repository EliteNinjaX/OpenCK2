#include "../src/game/levy_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool near(double a, double b) { return fabs(a - b) < 1e-7; }
int main(void)
{
    struct levy_effect base = {0}, barracks = {0}, walls = {0};
    assert(levy_effect_assign(&base, "light_infantry", 60));
    assert(levy_effect_assign(&base, "heavy_infantry", 150));
    assert(levy_effect_assign(&base, "light_cavalry", 15));
    assert(levy_effect_assign(&barracks, "heavy_infantry", 40));
    assert(levy_effect_assign(&walls, "levy_size", .1));
    assert(levy_effect_assign(&walls, "fort_level", .4));
    assert(!levy_effect_assign(&walls, "tax_income", 4));
    assert(!levy_effect_assign(&walls, "heavy_infantry", NAN));
    levy_effect_add(&base, &barracks);
    levy_effect_add(&base, &walls);
    struct levy_potential potential = levy_potential_calculate(&base);
    assert(potential.known);
    assert(near(potential.troops[LEVY_HEAVY_INFANTRY], 209));
    assert(near(potential.total, 265 * 1.1));
    assert(near(potential.fort_level, .4));
    assert(!levy_potential_calculate(NULL).known);
    struct levy_effect penalty = {0};
    assert(levy_effect_assign(&penalty, "levy_size", -3));
    assert(levy_effect_assign(&penalty, "archers", 50));
    assert(near(levy_potential_calculate(&penalty).total, 0));
    puts("levy core: all assertions passed");
    return 0;
}
