# OpenCK2 — Title prestige and combat modifier increment

## Implemented in C17

1. `src/game/title_prestige.c/.h`: counts held and direct-vassal titles by barony/county/duchy/kingdom/empire and accumulates monthly prestige contributions using existing CK2 `defines.lua` rates. Connected to `economy.c` and exposed as `economy_snapshot.title_prestige`.
2. `character_view.c/.h`: reads `combat_rating` values from original trait definitions and displays the **trait-derived combat modifier** (with `+`/`-` sign). It is not a complete personal combat skill score.
3. Fixed a GCC `-Wmissing-braces` build warning in `character_view.c`.
4. Added `tests/title-prestige.c` and Makefile `check-title-prestige` target.

## Tests run

- `make check-title-prestige` — passed.
- `make check-economy-construction CFLAGS='-O2 -Werror'` — passed.
- `make check-economy CFLAGS='-O2 -Werror'` — passed.

Full SDL compilation not verified in this environment. Run on Linux: `make -B game CFLAGS='-O2 -g -Werror'` and `make check-title-prestige check-economy-construction`.

## Important remaining work

- **Learning and piety:** Current monthly piety uses historical base learning multiplied by the static modifier read from CK2 files; trait-adjusted learning, councillor/religious effects, building piety, technology and detailed localized monthly tooltips are **not** implemented in this increment. Existing piety calculations remain unchanged.
- **Title gameplay:** Held/vassal prestige is based on bookmark title history. Granting/revoking titles, succession, claims, de jure structure, real-time title ownership and per-title event modifiers need new simulation code.
- **Combat:** The displayed signed modifier is only the sum of authored trait `combat_rating` entries; other modifiers are unknown. Avoid interpreting it as exact CK2 total.
- **Executable:** Original `title.cpp` path evidence is documented in `docs/EXE_RESEARCH.md`. No original C++ implementation was recovered, copied or asserted to be functionally identical.
- **Visual fidelity:** Original CK2 tooltips in the screenshots show a detailed breakdown. A matching localized breakdown UI is still pending.
