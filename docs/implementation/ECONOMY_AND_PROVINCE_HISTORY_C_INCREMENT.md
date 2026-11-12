# OpenCK2 — C17 economy and historical province attributes, increment 2

Baseline: `OpenCK2_Map_Holdings_Governments_C.zip` from the preceding OpenCK2 increment. No original CK2 data was bundled with the modified source. The code continues to read the user's installed loose files through the existing mounted asset lookup.

## What was fixed

### 1. Historical culture and religion (verified source integration)

- `holdings_read()` now reads root and dated `culture =` / `religion =` from `history/provinces/*.txt` using the selected bookmark date, independent of holding slots. Changes after the bookmark are not applied.
- `holdings_load()` localizes both values through the existing CK2 localization loader and still localizes holding names.
- `map_settlements_build()` preserves province culture and religion separately from the holding owner's own culture. A fact entry is stored for every province history file found, including provinces without an active holding. Previously, the map's one-capital culture was sometimes overwritten by the barony owner's culture; religion was discarded entirely.
- The selected-province panel and map/holding tooltips now expose these values, or explicitly say they are not recorded. The character's personal religion/culture and the province's religion/culture are different concepts.
- At campaign startup, the console reports the number of province histories loaded and how many contain a culture/religion key. Use that report to confirm your particular installed game's files are being read.

### 2. Wealth (direct holding tax foundation)

- Removed the placeholder `5 + 2 * improvements` monthly income. `campaign_monthly_income()` now reads the actual income model's current amount.
- `construction_load()` reads `tax_income` from each entry of `common/buildings/*.txt` and settlement base income from `common/static_modifiers.txt`, covering castle, city, temple, tribal and nomad (when defined). Values are annual; monthly income is annual / 12. Original supplied static modifiers include castle **4/year**, city **12/year**, temple **8/year**, tribal **0.5/year**, nomad **0/year**.
- `holdings_read()` preserves documented `b_holding = building_key` assignments dated no later than the bookmark. The tax calculation sums completed building types while treating a later `upgrades_from` chain as a replacement, not a separate stack of older tiers. It also includes completed construction projects in the existing running-game construction ledger, deduplicating against history.
- `economy_snapshot_build()` walks *all installed province histories* at the selected bookmark, not merely the currently selected province, and counts individually owned holdings from barony title history. When a capital barony lacks an explicit holder, a county-holder fallback is **provisional** and counted in the startup log; unknown noncapital holders are excluded rather than invented.
- The campaign receives direct holding tax on monthly rollover with fractional gold carried forward. Construction expenses still use the existing unsigned integer treasury API. After a building completes, its income is included in the next monthly refresh.
- Historical character `wealth` is respected when recorded. If absent, OpenCK2 currently starts the simulation with zero gold, **not an invented CK2-compatible starting treasury**.

**Important economic limitations:** This is *gross direct holding base + authored/completed building tax*. It is **not** a full CK2 realm income calculation: vassal contracts, church taxes, tax-law obligations, province/global modifiers, prosperity, technology, stewardship tax modifiers, trade posts, occupations, nomad tributaries and procedurally initialized buildings are not resolved. Do not compare the reported direct income to original CK2's *total* character income without adjusting for those missing systems. An explicit holder's income is not the same as an independent liege's tax revenue.

### 3. Prestige, piety, and demesne size (data-driven, partial)

- `economy_core.c` parses active `common/defines.lua` constants for `NTitle.*_TITLE_PRESTIGE`, `*_AS_VASSAL_PRESTIGE`, and `NDiplomacy.DEMESNE_*`. It also reads the monthly diplomacy->prestige and learning->piety multipliers from `common/static_modifiers.txt`.
- Personal prestige accrues for mapped personally held title ranks and the project's direct-vassal resolver, plus verified historical diplomacy when available. Personal piety accrues from base learning when that statistic is recorded. Both update on month rollover. Opening the played ruler's character panel displays the mutable campaign values rather than the frozen history profile.
- Demesne count is the number of counted personally owned holdings (including clearly reported provisional capital fallbacks). Demesne **limit** uses base + top rank + ruler/spouse stewardship + great-duke bonus from the installed defines, rounded down. Both are displayed together in the top bar.
- Starting prestige/piety come from dated historical character values when recorded; otherwise they begin at zero as provisional simulation seeds.

**Important prestige/piety/limit limitations:** Trait/event/focus bonuses, learned modifiers, church and council effects, dynasty prestige, landless sons, tribal/patrician and government special cases, title-law changes, succession/gavelkind, Jain bonus and game-rule variations are **not** evaluated. The topbar tooltip calls these partial known-rate calculations; do not claim numerical CK2 parity yet. There is no complete simulation of historical ruler changes after the bookmark date.

## Code overview

| File | Change |
|---|---|
| `src/game/economy_core.[ch]` | Standalone C17 defines parser; monthly-tax and base demesne helper |
| `src/game/economy.[ch]` | Dated holding ownership/income snapshot, partial monthly prestige/piety and demesne count |
| `src/game/holdings.[ch]` | Date-aware culture/religion and historical building IDs |
| `src/game/construction.[ch]` | Building tax, base holding tax, upgrade replacement, completed projects |
| `src/game/campaign.[ch]` | Fractional gold and mutable prestige/piety monthly tick |
| `src/game/map_settlements.[ch]` | Preserve province religion/culture, province fact cache |
| `src/game/character_view.[ch]` | Show current player's mutable prestige/piety/wealth |
| `src/game/main.c` | Initialize income after dated realms, render stats and provincial data, diagnostic logs |
| `tests/economy-core.c`, `tests/finance-holdings.c` | SDL-free synthetic fixture tests |
| `tests/campaign.c`, `Makefile` | Updated monthly tick expectations and checks |

## Verification performed

- `make -B check-economy check-map-core check-governments-core check-settlement-styles check-map-positions CFLAGS='-O1 -g -Werror -fsanitize=undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=undefined'`: **PASS** for all six test executables.
- `economy_core` parser checked against the actual supplied `common/defines.lua` and `common/static_modifiers.txt`: **PASS** (`demesne_ready`, `prestige_ready`, `attributes_ready`). The supplied CK2 `KING_TITLE_PRESTIGE` was 0.8/month; `DEMESNE_MAX_SIZE_KING_MULT` was 3.
- `gcc -fsyntax-only` with a *test-only stub* for the five data/adapter units: **PASS** with `-Werror`. This is not a full SDL compile.
- `make -B game CFLAGS='-O2 -g -Werror'`: **BLOCKED** on this execution host (no `sdl2` or `SDL2_image` pkg-config files; no SDL2 development headers). User's Linux machine previously completed the game build and is the intended integration test site.
- **Not verified:** runtime HUD positions, actual income comparison in a dated original CK2 save, generated buildings, local CK2 province title histories, full native game compile.

## Linux acceptance checklist

```bash
make -B game CFLAGS='-O2 -g -Werror'
make check-economy check-ck2-map-data check-map-core CFLAGS='-O2 -Werror'
```

1. Load 1066 and 1337 on the same original assets and DLC/mod configuration. Watch startup logs for `Province history audit` and `Economy bookmark snapshot`. Confirm the count of authored culture and religion keys is nonzero.
2. Select a Greek/Orthodox province and a different-culture/different-faith province. Confirm the selected province panel and hover tooltip display the *province's* culture/religion, not automatically those of its holder; compare against the original game.
3. Hover a castle, city and temple. Compare base + explicitly recorded improvements against their static definition lines and their actual history. Look for a `county-holder fallback` warning where explicit barony holder history is absent.
4. On the player HUD, compare prestige, piety, demesne current/limit, and direct tax against the source fields and known missing systems. Advance a few months and check fractional-tax carry (no repeated 1-gold rounding errors).
5. Complete a building that adds tax; confirm the next month rises by `annual tax delta / 12`, with upgrade replacement rather than stacking the prior tier.
6. Confirm existing construction/holding selection and original character/window controls still work.

## Prioritized TODOs

- **P0:** Native `make game` + screenshots/test logs from the player's CK2 installation. Fix any new warnings or rendering overlap before claiming runtime success.
- **P0:** Add full local asset integration fixtures for dated history provinces, barony owners and explicit `b_x = building` assignments. Confirm the real history syntax is covered; extend for nested structures if found.
- **P1:** CK2-generated initial holding buildings, technology-driven start levels and special building histories (the original C++ creates some dynamically).
- **P1:** Dynamic title/holding ownership on character death, inheritance and ongoing simulation dates, including generated barons and proper capital ownership.
- **P1:** Feudal city, temple and vassal taxes; laws/obligations; owner/government restrictions; economic modifiers; province culture/religion effects.
- **P1:** Full prestige/piety modifiers and per-event sources; gavelkind, Jain, government/game-rule demesne limit adjustments.
- **P2:** Floating-point purchase/treasury accounting, trade zone and nomad economy, tax breakdown UI, province-level culture/religion changes after bookmark.
- **C++ deferred:** No conversion required for these C data and simulation modules. A future full modifier/event evaluator or 3D settlement renderer could benefit from C++ memory/container management, but both remain viable in C and are not converted in this increment.
