# CK2 UI status and TODO

This tracks the original files, the current implementation, and the work
behind deferred UI. It uses CK2 3.3.5.1 as the reference.

| Area | Original files | Implemented | Pending |
| --- | --- | --- | --- |
| Setup lists | `interface/frontend.gui` (`bookmarks_list`, `Chars_Listbox`, and their row templates), `interface/EU3_core.gui` (`standardlistbox_slider`), `interface/generalstuff.gfx` | Both lists use the shipped scrollbar sprites and their own original list bounds. Arrows, track, thumb drag, and wheel operate independently. | Matched native checks for exact scrollbar spacing, clipping, page stepping and drag behavior. |
| Province holdings | `interface/province.gui` (`province`, `capital_position`, `settlements_1_box`, `settlements_2_box`, `capital_entry`, `settlement_entry`), `interface/province.gfx`, `history/provinces/` | Holdings appear only in gameplay. The selected county's start-date history drives the capital, populated settlement art, and empty slots. Layout reads the original position and template fields. | Live settlement ownership and buildings, generated barons, culture-specific art, title shields, garrison and tax indicators, construction, and expanded layouts for larger modded counties. |
| Blank tab | `interface/frontend.gui` defines `character_tab` separately from `republics_tab`. | Removed OpenCK2's blank `character_tab` sprite from the setup ruler panel. | Trace when vanilla shows the Republic and other tabs and what state populates them. |
| Dynasty shield | `dynasty_shield` in `interface/frontend.gui`, `GFX_shield_small` in `interface/generalstuff.gfx`, authored `coat_of_arms` in `common/dynasties/`, `interface/coat_of_arms/coats_of_arms.txt` and the referenced textures. | The existing title shield still renders using title flags. | Approve a separate heraldry pass. Decode authored pattern/emblem data and render one known dynasty into the original shield mask at its original position. Then investigate generated arms, religion variants, and DLC. A title flag cannot supply the dynasty arms. |
| Difficulty | `difficultybar` and `difficulty` in `interface/frontend.gui`, `GFX_difficulty_progress` in `interface/frontend.gfx`, `gfx/FX/progress.lua`. | The original empty track and overlay use the installed sprite paths, coordinates, and dimensions. | Recover the selected-ruler difficulty score and range with matched vanilla rulers and native analysis. The shader consumes a runtime `CurrentState`; the loose UI has no per-ruler score. |
| Welcome | `interface/hints.gui` (`hints_splash_screen`, `hints_splash_screen_2`), `interface/hints.gfx`, `interface/game_rules.gfx`, and `HINT_SPLASH_*` in `localisation/text1.csv` and `v2_40d.csv`. | Located the original layout, background, banner image, vassal/heir/time-control art, and text keys. | Approve a two-page onboarding pass. First verify when it opens, whether it pauses the campaign, and how hints are dismissed or remembered. Then bind the first page and Next to the second page's ruler-dependent culture, government, and religion details. Use a separate pass for the other context-sensitive hint windows. |

## Controls showing “Not developed yet”

- Main and Single Player menus — Multiplayer, Load Game, Tutorial.
- Era cards — Shattered World and Random World affordances.
- Custom Game Setup — Load Game, day/month/year arrows, Shattered World,
  Random World, Counties, Duchies, Kingdoms, Religion, Culture, Governments.
- Campaign navigation — the placeholder HUD tab strip for Council, Laws,
  Technology, Military, Intrigue, Diplomacy, Religion, and Realm.
- Settings → Game — Difficulty, Autosave Interval, Language, Disable
  Wikipedia Links, Disable All Hints, Autosave to Cloud, Compress Autosaves,
  Steam Rich Presence.
- Settings → Video — Multisample Level, Refresh Rate, UI Scale, Gamma,
  Realm-colored Wastelands, Transparent Wastelands, VSync, Province Embellishment.
- Settings → Audio — Ambient volume, Dynamic Ambient Sounds, Context Sensitive
  Music, Death Sounds.
- Settings → Controls — Scroll Speed, Zoom Speed, Camera Smoothness, Camera Tilt,
  Zoom in to Cursor.

The setup difficulty meter and the Difficulty option in Settings are separate.
See [Settings backlog](SETTINGS-TODO.md) for the current settings implementation.

## Linux verification

In Late Middle Ages setup, both scrollbar strips rendered at the edge
of their list. Clicking the Interesting Characters down arrow changed
the visible rulers while the bookmark rows stayed in place. The blank
tab and setup holdings panel were absent. Starting the campaign showed
the selected province's capital and settlement slots in the province
panel. The setup layout test loaded the installed assets and the
launcher, campaign and holdings regressions passed with `-Werror`.

## Campaign construction

- Finish the vanilla building list statistics, level indicators, scroll controls, and tooltips.
- Compare the Create Holding dialog and refine title selection, price modifiers, duration, and eligibility against vanilla. See [construction evidence](reverse-engineering/CONSTRUCTION.md).
