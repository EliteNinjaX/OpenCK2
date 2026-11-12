# OpenCK2 — original CK2 loose files, mechanics, UI and settings

This file consolidates the original game asset/data compatibility guide and the pending UI/settings inventory. The source snippets beneath are historical documentation; verified data in the installed CK2 files and current source code should take precedence when they disagree.

The binary's original function addresses, decompilation and recovered source names are maintained exclusively in [`EXE_RESEARCH.md`](EXE_RESEARCH.md). For current code priorities see [`../ROADMAP.md`](../ROADMAP.md).

## Sections

- Vanilla gameplay mechanics and data relationships
- Loose-file compatibility and asset-loading precedence
- Character, province, map and launcher UI backlog
- Settings backlog


<a id="source-docs-ck2-mechanics-md"></a>
## Source note: `docs/CK2-MECHANICS.md`

> Preserved from the prior source archive. Original SHA-256: `c0e76fe2f3542d2e3be70229d5868f54bd68b919fea29e0b3d8ed990f1328bc2`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Vanilla Crusader Kings II mechanics
This is a guide to the original game for contributors, including readers
without C experience. It describes mechanics at a level useful for planning
reference tests. The pinned baseline is CK2 3.3.5.1. Record the active DLC
and rules for each comparison, since they can change the available actions
and systems. The [implementation inventory](DEVELOPMENT.md#source-docs-implementation-md) describes
which parts OpenCK2 currently runs.

#### The world the player controls
A campaign follows a ruler and their dynasty. Characters have family
relationships, culture, religion, traits, and claims. Titles identify
political positions from barony through empire. A title's current holder
and its place in the de facto vassal hierarchy can differ from its de jure
relationship. A map click can therefore identify a county holder whose top
liege is a different character. A county's capital and its other holdings
are separate seats. The ruler's demesne consists of holdings they control
directly. Vassals govern their own lands within the hierarchy. [Paradox CK2 overview](https://www.paradoxinteractive.com/games/crusader-kings-ii/about)

The local installation provides starting conditions in
`common/bookmarks/`, `history/characters/`, `history/titles/`, and
`history/provinces/`. For example, `history/titles/k_france.txt` contains
both an early name override and a dated reset. Province records contain
holding type, slot count, and changes such as a capital move or removal.
The [historical selection notes](EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md)
document verified examples. Initial runtime object creation and the order
in which the engine applies all inputs remain investigation targets.

#### Succession, government, and relationships
Succession determines who inherits when a holder dies. Laws can select one
heir or divide titles. Eligibility and ordering depend on the applicable
succession law and character relationships. For example, primogeniture
favors an eldest eligible line, while gavelkind can divide holdings among
eligible heirs. The next player character and the recipients of individual
titles must be checked separately. Government and realm laws affect the
available actions and obligations. [Paradox CK2 manual, laws and succession](https://cdn.akamai.steamstatic.com/steam/apps/203770/manuals/CKII_MANUEL-ONLINE_FR_HD.pdf?t=1472131858)

Start with `common/laws/succession_laws.txt`, `common/governments/`, and
the dated title and character files. A useful test records the ruler, heirs,
title laws, and vassal relationships before a controlled death and checks
each title afterward. Track tie-breaking and update order as open until
observed or traced. Conclave's council powers are an expansion case and
belong in a separate fixture. [Paradox Conclave overview](https://www.paradoxinteractive.com/games/crusader-kings-ii/add-ons/crusader-kings-ii-conclave)

#### Economy, holdings, and levies
Holdings are sources of income and troops. Directly controlled holdings
and vassal contributions are separate sources. Buildings change a
holding's capabilities, with requirements and costs that depend on the
building and local conditions. The simulation must combine the installed
definitions with character, law, modifier, and holding state. The exact
tax and levy formulas, rounding, and update timing require a pinned
reference scenario. [Paradox CK2 manual, province and military interfaces](https://cdn.akamai.steamstatic.com/steam/apps/203770/manuals/CKII_MANUEL-ONLINE_FR_HD.pdf?t=1472131858)

Inspect `common/buildings/`, `common/laws/`, and the current province and
holder records. Record a monthly income change, one construction from
start to completion, and levies before and after a single controlled
input changes. OpenCK2's prototype income and upgrades are temporary
simulation rules.

#### Diplomacy, war, religion, and events
Diplomacy acts on characters and their relationships. Marriages, claims,
opinion, factions, and religious or legal conditions affect which actions
are available. A war's casus belli defines why it can start and what
outcomes can change. Armies move, fight, and besiege holdings. The peace
outcome depends on the war's rules, participants, and result. Religion and
culture also feed into scripted conditions and historical character
identity. [Paradox CK2 manual, diplomacy and warfare](https://cdn.akamai.steamstatic.com/steam/apps/203770/manuals/CKII_MANUEL-ONLINE_FR_HD.pdf?t=1472131858)

Read `common/cb_types/`, `common/combat_tactics/`,
`common/religions/`, `common/objectives/`, and `common/traits/`. A claim
war is a bounded test when claimant, target, casus belli, and peace
outcome are fixed. Extra casus belli can be expansion features. [Paradox Jade Dragon overview](https://www.paradoxinteractive.com/games/crusader-kings-ii/add-ons/crusader-kings-ii-jade-dragon)

The installed `events/`, `decisions/`, `common/on_actions/`,
`common/scripted_triggers/`, and `common/scripted_effects/` describe
scripted behavior. For a reconstruction fixture, select one trigger and
one effect, then capture the actor, scope, eligibility, and state change.
The order of dispatch, random draws, and AI decisions must be measured
or traced for the pinned build.

#### Time, rules, saves, and verification
The player can pause the campaign and change its speed while the calendar
advances. The project needs separate evidence for the simulation's day
boundary, scheduled updates, and rule effects. `common/game_rules/`
describes rule options. A menu label or default value establishes the
available choice. To verify its effect, compare the same reference
scenario with a single rule changed.

For save work, begin with an isolated, plain-text non-Ironman fixture.
Compare the restored ruler, titles, holdings, date, and treasury, then
observe the next transition. Record DLC, mods, checksum, and save variant.
The original game's Continue filename handoff has been traced. World
serialization and interoperability are open. [Launcher evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-launcher-decomp-md)

Use the [roadmap](../ROADMAP.md) and
[REA knowledge base](EXE_RESEARCH.md#source-docs-reverse-engineering-readme-md) to turn these
mechanics into reproducible comparisons. The original manual is a
player-facing reference from an earlier release. Use the installed
3.3.5.1 data and controlled runs for patch-specific details.

---

<a id="source-docs-loose-files-md"></a>
## Source note: `docs/LOOSE-FILES.md`

> Preserved from the prior source archive. Original SHA-256: `6d914d26eb6b6b3b07054710a2b08cd2c28f372efa7ead3b1cc18fef138819ba`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### CK2 loose-file compatibility
OpenCK2 targets CK2 3.3.5.1 (SOHY) behavior. Use the player's installation
for data, scripts, and art wherever the original engine exposes them as files.
The OpenCK2 launcher is outside this requirement. The engine supplies the
meaning of those files, including script execution, screen actions, and
simulation rules. The matrix below records current coverage by file family.
A listed reader establishes source coverage for the stated fields only.

#### Coverage and exceptions
| Installed files | What OpenCK2 reads today | Gap and required work |
| --- | --- | --- |
| `interface/frontend.gui` and the four setup `.gfx` files | The structural reader indexes every brace block and retains its source text. Custom Game Setup binds the top toolbar and nested date controls, the left and right panels, list positions and row sizes, the bottom description and DLC strip, and Play/Back. Selected labels resolve through the installed localization. | Temporary. Parsing the whole document establishes structure, while engine bindings still cover only the setup widgets listed here. Date arrows and Load Game are deferred. Difficulty has a frame and label, with no calculated score. Diplomacy indicators, DLC recommendations, scrollbar dragging, and vanilla vassal roster semantics remain pending. The visible vassal rows already use portraits and title shields, with arrows, track clicks, and wheel scrolling. |
| Other windows in `frontend.gui`, other `interface/*.gui`, and `connectui/` | The main menu reads the five primary button positions from `mainmenu_bottom_panel_banner`. Other main menu elements, era selection, Game Rules, Settings, and campaign still use C layout where no binding exists. Credits read the local text file. | Temporary. Main menu, ruler designer, save dialogs, multiplayer, and remaining frontend windows need widget and engine bindings. Browser/account integration needs its own investigation. |
| `map/definition.csv`, `map/provinces.bmp`, topology, normals, terrain, rivers, trees, and `map/terrain/` | Reads province colors and several bitmap/DDS layers for picking, terrain, water, forests, and Realms. | Temporary. Input filenames are still selected in C. Parse `map/default.map` and supporting region, adjacency, terrain, position, and climate files. The software renderer has approximate relief, borders, lighting, and label placement. Recover native sampling and draw order. |
| `history/{characters,titles,provinces}/`, `common/{landed_titles,dynasties}/` | Reads a historical snapshot for the selected date, including known holders, rulers, cultures, religions, dynasty names, and holding slots. | Temporary. Date handling and title ownership use selected fields and a simplified resolver. Other history families and scripted history effects remain unread or unapplied. Recover the history evaluation order and test repeated dated blocks, lifetimes, parent titles, and precedence. |
| `common/{cultures,religions,governments,government_flavor,holding_types}/` | Reads selected culture/religion metadata for names, icons, and portrait/title presentation. Holding slots come from province history. | Temporary. Government and holding rules, succession eligibility, conversion, and religion/culture mechanics need registries, script evaluation, and simulation. UI metadata provides no evidence of those rules running. |
| `common/bookmarks/`, `common/game_rules/`, `common/alternate_start/` | Reads selected bookmark and game-rule definitions from named files and uses the installed era and rules art. | Temporary. Directory-wide merging and rule application are pending. Rule choices can be viewed or stored locally. Alternate starts, Ironman, and achievement behavior need engine implementations. |
| `events/`, `decisions/`, `common/{on_actions,scripted_triggers,scripted_effects,event_modifiers}/` and mechanics databases such as buildings, laws, traits, CBs and technology | There is no general CK2 script execution. The campaign is a small simulation with C-defined income and upgrades. | Temporary. Implement typed registries, trigger and effect scopes, deterministic scheduling, and saved state. The local token reader can extract fields but cannot evaluate scripts. Add one reference event chain and one decision before widening opcode coverage. |
| `localisation/*.csv`, `common/customizable_localisation/`, bitmap fonts | Selected keys resolve through installed CSV files. Some screens use the original DDS/TGA bitmap fonts and `.fnt` metrics. | Temporary. Language selection, script substitutions, contextual localization, encoding and fallback parity, and many C-written labels still need work. |
| `interface/portraits/`, `gfx/characters/`, `gfx/flags/`, coat-of-arms assets | Reads a subset of sprite layers, graphical cultures, DNA and properties, and installed flag art. | Temporary. Recover scripted portrait and clothing rules, dynamic heraldry, DLC dependencies, and composition order. |
| `interface/*.gfx` model declarations, `.xac`, `.mesh`, `.xsm`, and other animation files | Static geometry imports for the 370 unique base model names checked earlier. The 3D map frame renders in-game. DDS support covers the formats used by existing readers. | Temporary. Per-part material binding, general actor placement, skinning, animations, and the full material/shader and DDS formats remain pending. See [map rendering evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-map-rendering-md). |
| `interface/sound.sfx`, `sound/`, `music/`, `soundtrack/` | Uses local sound definitions for selected cues and plays the installed menu track. | Temporary. Contextual playlists, ambient triggers, mixing rules, and the remaining audio declarations need implementation. |
| `common/defines.lua` and other native settings inputs | Selected portrait values are read as text. Settings uses a separate C layout and applies a subset of controls. | Temporary. Recover which defines are read, their override order, and which are Lua data or engine-bound behavior. UI scale, video pipeline and most simulation settings are pending. |
| DLC archives, folder and ZIP mods, `replace_path` | Existing resolver supplies mounted assets to supported readers. Toolbar tests cover a folder mod's GUI and a sprite definition overriding a base definition in another supported file. | Temporary. Vanilla discovery, duplicate declaration handling, ZIP overrides, total conversions, and order across multiple mods need matched reference runs. Format support and mount availability are separate. |
| `tutorial/`, save and conversion formats, multiplayer and account data | No native tutorial progression, vanilla save round-trip, or network runtime. | Temporary for tutorials and save data. Implement the engine systems behind their data. Steam, account and other online services require runtime integrations and cannot be recreated by parsing loose files alone. |

#### Toolbar evidence and temporary choices
The installed 3.3.5.1 `frontend.gui` directly supplies `top_window`
and its named children. In the shipped `main.gfx`, the eight map-mode
sprites name `.tga` files. The local Steam installation has corresponding
same-stem `.dds` files. The loader tries the declared asset and uses
that DDS when the TGA is absent. An existing corrupt TGA produces an
error. This compatibility rule is based on the installed files and the
visible toolbar. Vanilla's exact extension resolution remains unverified.

The loader consults `main.gfx`, `frontend.gfx`, `alternate_start.gfx`,
and `generalstuff.gfx`. For duplicate sprite
names across them, it prefers the higher mounted layer, then the first
matching file in the list. That tie-break is OpenCK2 behavior awaiting
reference evidence. It does not scan arbitrary new `.gfx` files yet.
Hover uses the second frame where available. Both hover-frame selection
and draw order still use C behavior. Unsupported required references
or malformed positions produce a setup error without replacing the
screen with a hardcoded toolbar. The frontend tree loads at startup. Setup sprites and labels bind on first entry. Text rendering uses the
existing bitmap font path and local layout rectangles. The holdings panel
remains an OpenCK2 extension, and the DLC strip uses OpenCK2's current
content catalog to choose icon state. List rows use native dimensions
and can be scrolled with the wheel. Native scrollbar appearance and
interaction remain pending. The date arrows and Load Game show the
existing deferred tooltip, while Play opens the existing Game Rules flow.

A bounded REA/Ghidra search for `frontend.gui` timed out on the local PE.
No native parsing or load-order semantics were established by that
probe. Follow the [REA workflow](../ROADMAP.md#rea-workflow) to trace
the file-open and widget-instantiation paths. Verify texture extension
resolution, sprite registration order, anchors, and button-state
selection against vanilla before broadening compatibility claims.

#### Verification
- `make game check-launcher check-campaign check-setup-toolbar CFLAGS='-O2 -Werror'`
  builds the game and exercises launcher, campaign, and toolbar checks.
- `bin/setup-toolbar-test "$CK2_DIR"` loads the full installed GUI tree and
  the setup bindings with a software renderer. The installed-file check
  also verifies the presence of currently unbound save and customization
  windows and the positioning of the bound sections. The fixture suite tests sprite
  sheet width, anchoring at two logical widths, click targets, shortcuts,
  mod overlays, missing files and references, a malformed layout, and a
  corrupt declared texture.
- A Linux runtime check opened 769 Custom Game Setup at 1280 × 720.
  An isolated asset mirror changed Terrain's x offset from −127 to −180
  and Realms' shortcut from W to E. Clicking the moved Terrain button
  selected Terrain. Pressing E selected Realms again using the same
  executable. The original Steam directory was left intact.
- A second Linux runtime check selected a featured ruler, scrolled the bookmarks,
  opened Game Rules with the original Play control, and returned to the era
  screen with the original Back control.
- The original screenshot and installed files guided the default layout.
  A synchronized vanilla run and Windows/macOS verification remain pending.

#### Next milestones
1. Finish the remaining Custom Game Setup bindings and compare text,
   clipping, scrollbars, and input in synchronized vanilla and OpenCK2
   runs. Reuse it for Settings and
   Game Rules once those semantics are verified.
2. Load the map's file references from `map/default.map` and validate
   base and overridden province layouts with a small synthetic map.
3. Define the shared script data model and recover history evaluation
   and content priority before implementing a small event/decision
   vertical slice. Compare date, state changes and save results
   against the original engine.

---

<a id="source-docs-ui-todo-md"></a>
## Source note: `docs/UI-TODO.md`

> Preserved from the prior source archive. Original SHA-256: `ca2d161fadfdb187d2337da1896cb8c8469236608f04747d01305dbbd396a193`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### CK2 UI status and TODO
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

#### Controls showing “Not developed yet”
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
See [Settings backlog](GAME_DATA_AND_UI.md#source-docs-settings-todo-md) for the current settings implementation.

#### Linux verification
In Late Middle Ages setup, both scrollbar strips rendered at the edge
of their list. Clicking the Interesting Characters down arrow changed
the visible rulers while the bookmark rows stayed in place. The blank
tab and setup holdings panel were absent. Starting the campaign showed
the selected province's capital and settlement slots in the province
panel. The setup layout test loaded the installed assets and the
launcher, campaign and holdings regressions passed with `-Werror`.

#### Campaign construction
- Finish the vanilla building list statistics, level indicators, scroll controls, and tooltips.
- Compare the Create Holding dialog and refine title selection, price modifiers, duration, and eligibility against vanilla. See [construction evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-construction-md).

#### Campaign character panel
- Click the player portrait in the campaign to open the character panel. The frame, tabs, labels, family rows, close control, and vassal list use `interface/domestic_dynastic.gui` and its sprite references. Artwork is resolved through `interface/domestic_dynastic.gfx` and the shared interface sprite files. Labels use the installed localization.
- The panel shows historical identity, the primary title's shield and crown, religion, and the campaign treasury. Family uses parent IDs and dated marriage, birth, and death entries in `history/characters`. Vassals use the existing historical title roster and support wheel scrolling.
- Finish Relations, Court, Pacts, and Abroad when their runtime data exists. The tabs show the red deferred tooltip. Dynasty arms still need the coat-of-arms renderer. The heir, wards, generated court, traits, live stats, diplomatic actions, character navigation, and vanilla vassal ordering and scrollbar need separate passes.
- Family is a start-date snapshot. The spouse display supports one dated spouse and family rows are bounded. Exact portrait appearance, kin ordering, and runtime relationship changes need further work.
- The character panel reads the five authored base attributes from character history and applies dated assignments up to the campaign start. Attribute icons and positions come from `domestic_dynastic.gui`. A dash marks an omitted value. Recover generated attributes, trait effects, spouse and council contributions, and state totals in a separate stats pass.
- The `choose_country_*` harp cues are used in ruler selection. Entering a campaign stops any still-playing harp, and campaign province selection and the Capital button use the generic click.

#### Campaign top bar, titles, traits, and Great Works
- The top bar binds CK2's resource icons, extension, date strip, speed indicator, and plus/minus buttons from `interface/main.gui` and `interface/main.gfx`. The treasury and date use campaign state. The five speed settings use OpenCK2's clock intervals. Resource totals without simulation data show a dash.
- The character panel uses localized rank titles and flags for historically held titles. Culture-, government-, and religion-specific ruler titles and claim shields need additional work.
- The trait row reads authored traits and dated changes from `history/characters`, sprites from `interface/traits.gfx`, and direct skill modifiers from `common/traits`. The stat tooltip separates the authored base from trait modifiers. Generated traits, runtime effects, and state totals remain open.
- The province's Great Works button opens the original `interface/wonder.gui` panel for historically authored wonders. The reader uses dated province history, the picture reference in `common/wonders`, original sprites, and localization. Construction, upgrade eligibility, repair, and gameplay effects remain open.
- Dynasty arms require decoding `coat_of_arms/data` from `common/dynasties`, then composing the correct palette, atlas frame, emblem, and shield mask from `interface/coat_of_arms/coats_of_arms.txt` and associated assets. Verify an authored dynasty against vanilla before covering generated arms and DLC atlas ordering.
- Bloodlines use `interface/bloodlines.gui`, `interface/bloodlines.gfx`, `common/bloodlines`, and dated `create_bloodline` blocks in character history. Build a membership snapshot with verified DLC and inheritance rules before populating the character's bloodline list. A founder record alone does not establish which descendants inherit it.

#### Character inspection and historical claims
- Spouse, family, and vassal entries can open the selected character. The player portrait returns to the player. A spouse of a king or emperor can display a consort title when they hold no title of their own. Opinion, court location, and culture-specific titles still need runtime data.
- Claims read top-level and dated historical assignments, including direct weak-claim effects. Only authored claims are shown. Inherited claims, war eligibility, and claim interactions remain open.
- Education icons sort first using `education = yes` from `common/traits`. The authored coronation traits can render through the normal trait path when present. Their runtime acquisition depends on `events/HF_coronation_events.txt` and related scripts.
- Section icons and tooltips use the character GUI's localization keys. The top bar reads authored prestige and piety where present and leaves generated totals blank. Wealth and the other resource tooltips use original localization.
- The [144-hour local AI queue](DEVELOPMENT.md#source-docs-qwen-144-hour-plan-md) covers these remaining systems in checkpointed parts.

---

<a id="source-docs-settings-todo-md"></a>
## Source note: `docs/SETTINGS-TODO.md`

> Preserved from the prior source archive. Original SHA-256: `4f51fdead4b2da18389aff566429cd2ce1c81d5043af98763c72b7b1c079fad0`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Settings TODO
Open **Options** on the main menu to find Game, Video, Audio, and Controls.
**Apply** updates the running session. **Back** or Escape discards edits made
since the last Apply. Hover over an unfinished control to see its status.

#### Game
- [ ] Difficulty and gameplay modifiers
- [ ] Autosave interval, cloud autosaves, and autosave compression
- [ ] Language selection and localized text
- [ ] Disable Wikipedia links and gameplay hints
- [ ] Steam Rich Presence

#### Video
- [x] Windowed resolution presets and windowed/borderless switching
- [ ] Exclusive fullscreen and display refresh rate selection
- [ ] Multisampling, UI scale, gamma, and VSync control
- [ ] Realm-colored and transparent wastelands
- [ ] Province embellishments

#### Audio
- [x] Master, Music, and Effect volume
- [ ] Ambient volume once its audio channel exists
- [ ] Dynamic ambient sounds, context-sensitive music, and death sounds

#### Controls
- [ ] Scroll speed and zoom speed
- [ ] Camera smoothness and tilt
- [ ] Zoom toward the cursor

#### Across sessions and main menu
- [ ] Persist applied preferences across restarts
- [ ] Multiplayer

---

### Screenshot-match HUD / shield asset bindings (2026-11-12)

OpenCK2 now binds `domestic_dynastic.gui`'s `dynastic_shield` separately from `character_shield`; the dynasty coat itself remains an explicitly labeled **title-flag proxy** until `common/dynasties` artwork is decoded. `main.gui` supplies `topbar_extension_0..2`, `topbar_ruler_shield`, and `topbar_ruler_crown`. `province.gui` supplies `pv_county_shield`, `settlement_entry`, `settlement_placeholder`, and `settlement_owner_frame`; `province.gfx` supplies culture-specific building placeholders and frames. `alerts.gui`/`alerts.gfx` supply the separate top-center alerts strip and its 69-frame sprite sheet. The eight grayed preview slots are **not** game-derived alerts. See `docs/DEVELOPMENT.md` for tests and replacement conditions. The original retail assets are loaded from the user's installed CK2 directory, not copied into this repository.

## Dynasty heraldry: original asset binding and open questions (2026-11-12)

The character panel's `dynastic_shield` is distinct from `character_shield` (title arms). Original coat files are `common/dynasties/*.txt` for authored IDs, `interface/coat_of_arms/coats_of_arms.txt` for religion-specific texture and color catalogs, and `interface/coats_of_arms.gfx` for templates and masks. Textures live under the user's mounted CK2 `gfx/coats_of_arms/`, with `gfx/FX/coa_shield.lua` supplying the original effect. OpenCK2 now parses these loose files and renders a first C17 approximation without copying their images into the repository.

Remaining: exact shader compositing; emblem sprites/placement; DLC/mod style overrides and provenance validation; CK2 random-generation seed, banned-color pair logic and dynasty-on-title exceptions; real screenshot parity at UI scale. Title flags are still rendered by their separate `gfx/flags` pipeline. **Do not substitute the title flag for an authored dynasty when its original atlas and mask are available.**


## Localization and tree-view data (2026-11-12)

- Read original semicolon-delimited `localisation/*.csv` via the same mounted asset path as all other CK2 resources. CSV columns for this implementation: English = 1, French = 2, German = 3, Spanish = 5. Blank selected fields fall back to English. Keep the indexed catalog until language changes; never reopen the files for each text draw.
- Prefer existing original keys (`SM_LANGUAGE`, `SM_SETTINGS`, `SM_GAME`, `SM_VIDEO`, `SM_AUDIO`, `SM_CONTROLS`, `FAMILYTREE_INTERFACE_LABEL`, `REALM_TREE_INTERFACE_LABEL`, `DYNASTYTREE_INTERFACE_LABEL`, `DYNATREE_HOUSEOF`, `DYNATREE_LIVINGMEMBERS`, `DYNATREE_TOTALMEMBERS`, and character stat `*_MOD`). Unmatched original keys and new engine-only strings need explicit follow-up rather than invented translations.
- Tree window structures are read from `interface/dynasty_tree.gui` and atlas definitions from `interface/dynasty_tree.gfx`. Relationship source is `history/characters` (dynasty/father/mother/dates) and `history/titles` (realm ownership). The implementation uses the same source-provided dynasty coat-of-arms renderer. The family, dynasty and realm graphs use OpenCK2's own simplified layout: not copied binary C++ algorithms.
