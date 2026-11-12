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

## Campaign character panel

- Click the player portrait in the campaign to open the character panel. The frame, tabs, labels, family rows, close control, and vassal list use `interface/domestic_dynastic.gui` and its sprite references. Artwork is resolved through `interface/domestic_dynastic.gfx` and the shared interface sprite files. Labels use the installed localization.
- The panel shows historical identity, the primary title's shield and crown, religion, and the campaign treasury. Family uses parent IDs and dated marriage, birth, and death entries in `history/characters`. Vassals use the existing historical title roster and support wheel scrolling.
- Finish Relations, Court, Pacts, and Abroad when their runtime data exists. The tabs show the red deferred tooltip. Dynasty arms still need the coat-of-arms renderer. The heir, wards, generated court, traits, live stats, diplomatic actions, character navigation, and vanilla vassal ordering and scrollbar need separate passes.
- Family is a start-date snapshot. The spouse display supports one dated spouse and family rows are bounded. Exact portrait appearance, kin ordering, and runtime relationship changes need further work.
- The character panel reads the five authored base attributes from character history and applies dated assignments up to the campaign start. Attribute icons and positions come from `domestic_dynastic.gui`. A dash marks an omitted value. Recover generated attributes, trait effects, spouse and council contributions, and state totals in a separate stats pass.
- The `choose_country_*` harp cues are used in ruler selection. Entering a campaign stops any still-playing harp, and campaign province selection and the Capital button use the generic click.

## Campaign top bar, titles, traits, and Great Works

- The top bar binds CK2's resource icons, extension, date strip, speed indicator, and plus/minus buttons from `interface/main.gui` and `interface/main.gfx`. The treasury and date use campaign state. The five speed settings use OpenCK2's clock intervals. Resource totals without simulation data show a dash.
- The character panel uses localized rank titles and flags for historically held titles. Culture-, government-, and religion-specific ruler titles and claim shields need additional work.
- The trait row reads authored traits and dated changes from `history/characters`, sprites from `interface/traits.gfx`, and direct skill modifiers from `common/traits`. The stat tooltip separates the authored base from trait modifiers. Generated traits, runtime effects, and state totals remain open.
- The province's Great Works button opens the original `interface/wonder.gui` panel for historically authored wonders. The reader uses dated province history, the picture reference in `common/wonders`, original sprites, and localization. Construction, upgrade eligibility, repair, and gameplay effects remain open.
- Dynasty arms require decoding `coat_of_arms/data` from `common/dynasties`, then composing the correct palette, atlas frame, emblem, and shield mask from `interface/coat_of_arms/coats_of_arms.txt` and associated assets. Verify an authored dynasty against vanilla before covering generated arms and DLC atlas ordering.
- Bloodlines use `interface/bloodlines.gui`, `interface/bloodlines.gfx`, `common/bloodlines`, and dated `create_bloodline` blocks in character history. Build a membership snapshot with verified DLC and inheritance rules before populating the character's bloodline list. A founder record alone does not establish which descendants inherit it.

## Character inspection and historical claims

- Spouse, family, and vassal entries can open the selected character. The player portrait returns to the player. A spouse of a king or emperor can display a consort title when they hold no title of their own. Opinion, court location, and culture-specific titles still need runtime data.
- Claims read top-level and dated historical assignments, including direct weak-claim effects. Only authored claims are shown. Inherited claims, war eligibility, and claim interactions remain open.
- Education icons sort first using `education = yes` from `common/traits`. The authored coronation traits can render through the normal trait path when present. Their runtime acquisition depends on `events/HF_coronation_events.txt` and related scripts.
- Section icons and tooltips use the character GUI's localization keys. The top bar reads authored prestige and piety where present and leaves generated totals blank. Wealth and the other resource tooltips use original localization.
- The [144-hour local AI queue](QWEN-144-HOUR-PLAN.md) covers these remaining systems in checkpointed parts.
