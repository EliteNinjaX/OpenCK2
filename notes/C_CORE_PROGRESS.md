# OpenCK2 C-first implementation — Increment 1

Baseline: `OpenCK2_character_icons_extended.zip` (SHA-256 `1bdf89edea06742e52ffa095215d6cc00d09963b5561c99b57c6919f0e0face0`).

**Scope:** The first C implementation increment against the source-aware UI/map plan. It is not a complete playable CK2 implementation, and the changes have **not** been verified by launching OpenCK2 against a licensed installation. The project source remains C17. The original loose files are consumed from the user's existing CK2 installation; no CK2 assets have been packaged here.

## Implemented / changed

| Area | Files | Result | Verification |
|---|---|---|---|
| Pointer classification | `src/game/map_interaction.c/.h` | SDL-independent, four-logical-pixel threshold distinguishes clicks from drags; mismatched button releases cannot consume the gesture; focus loss cancels gestures. | Unit tests passed under GCC + ASan + UBSan. |
| Province right-click navigation | `src/game/main.c`, `src/game/realms.c/.h` | On stationary right click over the map, resolves county holder and opens the existing character panel; Shift+right-click targets independent/top ruler. Right drag still pans the map. Null/missing owner is a no-op. | Source-integrated; **requires runtime verification**. |
| Ownership read model | `src/game/realms.c/.h` | `realm_province_ownership()` returns separate known flags and title records for county and independent ruler. Both hover and navigation share this resolver. | Source inspection; not yet in runtime test. |
| Province tooltip | `src/game/main.c` | Shows authentic province name/ID, county, county holder, independent realm and liege when available; shows only supported right-click hints. Missing terrain, climate, winter, supply and defensive data are omitted rather than invented. Uses existing `panel_tooltip()`. | Source-integrated; visual parity **unverified**. |
| Input ownership | `src/game/main.c` | Blocks map interactions behind the open character panel, an open wonder modal, building panel, campaign HUD and mapped mode buttons. This corrects the coarse fixed rectangle for the character panel. | Source-integrated; runtime tests required. |
| Campaign map-mode catalog | `src/game/map_modes.c/.h` | 18 original `main.gui` campaign button names, keys and semantic IDs; only Terrain and Realms are enabled because only those renderers exist. | 18/18 ids verified against `interface/main.gui`; C unit tests passed. |
| Campaign GUI rendering | `src/game/campaign_hud.c/.h` | Loads original HUD nav and map-mode buttons by name from `interface/main.gui` and corresponding GFX sources; renders available sprites, dims unsupported map modes, highlights the selected supported mode and displays explanatory tooltip. No invented province colors. | Source-integrated; texture and position verification in-game required. |
| Society icon | `src/game/setup_toolbar.c` | Added `interface/domestic_societies.gfx` to the sprite lookup inputs so `GFX_topbar_folder_societies` resolves when present. Original `main.gui` positioning is used; existing raw icons remain as fallback. | Identifiers verified against loose files; visual verification pending. |
| Vassals data | `src/game/realms.c/.h`, `src/game/character_view.c` | New `realm_direct_vassals()` scans all titles held by a liege, deduplicates candidate direct vassals by character ID, chooses a display title and returns stable rank/ID order. Existing setup list API is unchanged. | Source-integrated; requires comparative gameplay test. |
| Build integration | `Makefile` | Adds the new C modules to the game build and `make check-map-core` to compile/run tests without SDL development files. | `make check-map-core` passed. `make -n game` lists the new files. |

## Tests run

```
make check-map-core
=> map core: all assertions passed

gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/map-core.c src/game/map_interaction.c src/game/map_modes.c \
    -o map-core-asan
./map-core-asan
=> map core: all assertions passed
```

The new `campaign_hud.c` also passed a `-std=c17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only` check using a narrow local SDL API declaration stub. This is a **syntax/warning check only**, not an SDL runtime test or a complete project compile.

`interface/main.gui` was additionally checked to contain all 18 catalog widget names; `interface/main.gfx` defines corresponding GFX names. This confirms identifiers, **not** availability of actual texture bytes in the game installation.

**Not run:** Full OpenCK2 executable compilation/gameplay screenshot comparison. This runtime lacks the SDL2, SDL2_image and SDL2_mixer development headers and pkg-config metadata. The package index is unavailable due to DNS/network restrictions. The uploaded baseline also omitted earlier `tests/*.c`/`tools/*.c` targets referenced by its Makefile. The new `map-core` test is included, but the older missing tests cannot be claimed to pass.

## Known limitations and outstanding UI work

- Map modes other than Terrain and Realms remain deliberately disabled. Their icons appear when their GUI/GFX and texture assets load, with a not-yet-implemented tooltip. There is no simulation or data-driven coloring for Religion, Culture, de jure, etc.
- Map-mode widget **positions are parsed** using the existing `frontend_gui_rect()` function. Its handling of nested CK2 orientations and the minimap's expanded/collapsed layout still requires an actual game comparison. No fake corrected hardcoded coordinates were introduced.
- Province tooltip is a safe first functional iteration using the existing basic tooltip renderer. Original skin, localization, dynamic weather, supply/defender bonuses, modifier-key interaction menus and caching remain unresolved.
- County holder may not always equal capital holding holder in vanilla CK2. The county-holder choice is explicit, not assumed to be CK2-exact. Owners and realm state use bookmark history (`start_date`) until live state simulation exists.
- The direct-vassals algorithm uses title history's explicit `liege` key, or `parent` for baronies, and deduplicates the resulting holders. This improves the previous title-only/primary-title view but is not a proof of exact CK2 allegiance, tax, opinion or sorting semantics.
- The screenshot's reported blank HUD element was **not identified with sufficient certainty**. No arbitrary icon was deleted. Republic/Nomad/Back remain separate GUI controls requiring capability/visibility checks.
- Character screen still lacks authentic Tax/Opinion values, original list sorting and fully functional tabs. Those must not be populated with unrelated treasury or invented opinion values.
- User's original executable debug-string research identifies promising methods, but **this increment did not perform fresh disassembly**. It implements bounded UI and data functions against the available C source and loose GUI names, not unverified decompiled algorithms.

## Next C increments (in priority order)

1. Obtain SDL development dependencies, original game data mount and the complete current repository, then build and run at consistent logical resolution; run screenshot A/B checks at the same date/province/character.
2. Implement GUI hitbox/anchor diagnostics for `main.gui` to confirm Society and all mode icons' actual positions; identify the reported blank element before hiding it.
3. Add character Vassals list sort controls, true row columns and scrollbar from the loose GUI; preserve separate unknown flags for tax and opinion.
4. Add terrain/climate/religion/culture province history facts with date-aware caching and localization. Do not map a terrain texture pixel to a terrain type without evidence.
5. Generalize province-color overlay cache for data-backed Religion/Culture/de jure/Direct-vassal modes as their data becomes reliable. Keep original GUI id mapping isolated.
6. Extend gesture tests to SDL integration and owner/top-liege fixtures including missing history and input under active modals.

See `notes/CPP_DEFERRED.md` for tasks held for a possible future C++ boundary; none of those tasks were started in this increment.
