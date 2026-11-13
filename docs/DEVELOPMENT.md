## Original CK2 elective-voting rules and Byzantine succession diagnostics — C17 increment (2026-11-12)

**Reference:** The supplied original `common/laws/succession_laws.txt`
contains `succ_byzantine_elective` with an `effect = { succession = {
base_type = feudal_elective voting_rules = byzantine_elective } }` block.
`common/succession_voting/01_succession_voting_specials.txt` defines that
voting rule. Its comment gives the election scopes (`ROOT` incumbent,
`FROM` candidate, `FROMFROM` contested title). Elector selection starts at
`days = -5`; it uses conditionally applied +30 for the emperor and +10 for
council members/commanders. Vote strength starts at `days = 5`, with +195
for the emperor subject to a large number of additional trait, age and
title-specific modifiers. Candidate selection explicitly considers the
ruler's relatives, claimants and commanders, and has disqualifying traits
and other scoped conditions. These are **authored script observations**, not
confirmed numerical election outcomes from the original executable.

**Implemented:** `election_core.c/.h` loads `common/succession_voting/*.txt`
from the mounted CK2 game. It indexes the four original rule sections,
retains the original nested scripted bodies for eventual trigger execution,
and extracts *top-level* integer baselines and counts without activating
conditional modifiers. The original law parser now extracts only static
`effect/succession/{base_type,voting_rules}` assignments, while still
ignoring unrelated nested law effects and potential/allow triggers. Each
installed law can therefore find its original voting script rather than
being assigned a generic elective algorithm. Original files remain read-only;
later loaded mod overrides replace the earlier named rule.

**Developer console:** `setlaw succ_byzantine_elective` followed by `election`
reports the current election rule, its section baselines, modifier counts,
and authored council/commander job/minor-title requirements. The separate
`electioncandidate <characterID>` (alias `byzcandidate`) can reject a known
dead candidate or prove rejection when the original candidate trigger
excludes the incumbent or a currently recorded prohibited trait. The
three-valued trigger evaluator supports character-ID comparisons, *known
present* traits, `ROOT`/`FROM` scope and Boolean `AND`/`OR`/`NOT`/`NOR`/`NAND`.
Unsupported scopes and absent traits are **UNKNOWN, not false**. Even if
recognized clauses pass, an unresolved clause prevents a candidate from
being declared valid; no full electorate, ballot, vote strength, predicted
winner or actual succession is fabricated. No council members/commanders are
assigned by the diagnostic, and death still does **not** transfer an elective
title. This is a necessary scripting/data bridge, **not a functioning
Byzantine election yet**.

**Verification:** `make check-election-core check-law-core
check-byzantine-government check-console-warfare CFLAGS='-O2 -g -Werror'`.
Real-file check:
`make check-election-core CK2_ELECTION_TEST_FILES='/path/to/CK2/common/succession_voting/00_succession_voting.txt /path/to/CK2/common/succession_voting/01_succession_voting_specials.txt'`.
The supplied data indexed six complete voting-rule sets, including all four
Byzantine voting sections. Tests include exact script overrides, misleading
nested clauses, unknown scopes and disqualified known-trait candidates.
Full SDL game compilation and UI runtime validation remain outstanding on
Linux Mint. Next: load council and commander appointments; represent actual
voter/candidate sets; implement safe scoped CK2 trigger evaluation for vote
scoring and tie-breaking; finally connect proven vote outcomes to in-memory
title succession. Do not use this diagnostic as proof of elections.

---

## Byzantine imperial government and government diagnostics — C17 increment (2026-11-12)

**Original evidence:** `common/governments/feudal_governments.txt` defines
`roman_imperial_government`, preferred `CASTLE CITY`, color `{ 143 12 125 }`,
`frame_suffix = _romanimperial`. Its authored `potential` is **not** a castle
classifier: an eligible ruler must have primary title `e_byzantium` or
`e_roman_empire` and either (a) be at the initial non-save-game setup, or
(b) have `succ_byzantine_elective` on that primary title; `is_patrician = no`
is also required. `common/laws/succession_laws.txt` defines Byzantine elective.
This is based on available script files, **not** decompiled government logic.

**Fixed:** Before this increment OpenCK2 parsed government definitions and
read occasional explicit character-history `government = ...` assignments but
never evaluated a government trigger. It therefore left the historical
Byzantine emperor unclassified, or showed a misleading castle-based government
candidate. A bounded, SDL-free, three-valued evaluator now interprets the
actual AND/OR/title/has_law/date/is_patrician subset used by the Roman imperial
script. Unrecognized triggers return UNRESOLVED and never count as true.
Original character-history assignments take precedence. During campaign
initialization, OpenCK2 finds eligible imperial title IDs from the **installed
CK2 government potential script**, confirms the actual dated holder's
primary title, reads any dated title succession law from `history/titles`,
then tags the holder's **in-memory** government as Roman imperial when the
original new-game condition is true. The source files are never modified.
The new result reaches Governments map-mode colors (CK2's authored purple),
province holding tooltips, and the character government tooltip. An explicit
`government_inferred` flag distinguishes this from an authored history value.

**Extra development:** Console command `government [ruler ID]` (alias `gov`)
reports a landed ruler's title, government and data provenance. The character
panel no longer reuses a stale family's government when displaying a ruler;
`setlaw <key>` reevaluates a *playable* inferred imperial government against
its selected succession law and refreshes the Governments map overlay/panel if
the type changes. After runtime inheritance the new playable ruler receives
the same conservative recheck. Missing active-law evidence yields unresolved,
not an invented switch. Other governments, especially republic, tribal,
nomadic and feudal, **still need their full potential/allow evaluator**; the
current holding preference is only a candidate, never a verified assignment.
The current government's name/icon/frame is not yet a 1:1 replica of CK2.
The no-patrician bootstrap predicate assumes an imperial holder with no
explicit patrician character-history government is not a patrician; merchant
republic histories and patrician status must ultimately be cross-checked.

**Tests:** `make check-governments-core check-byzantine-government
check-technology-console`. Synthetic fixtures verify precedence, initial
Byzantine and restored Roman claims, unsupported triggers staying unresolved,
patrician exclusion, Byzantine-elective transitions and dated title history;
`check-governments-core CK2_GOVERNMENT_TEST_FILES=...` also verified original
supplied CK2 government scripts (12 definitions in this snapshot). Full SDL
build and visual comparison against Byzantium at a matching bookmark must be
run on the user's CK2 installation; original government-specific decision
ordering, viceroyalty, imperial elective voting and succession remain pending.

---

## Laws, succession, inheritance and tax obligations — C17 increment (2026-11-12)

**Sources observed:** supplied original CK2 `common/laws/{succession_laws,demesne_laws,ze_obligation_laws,crown_laws,ze_crown_laws,ze_demesne_laws,ze_council_power_laws}.txt`; the runtime also optionally reads `history/titles/<primary_title>.txt` when present in the installed game. These files contain law groups, eligibility triggers, scripted effects, tax/levy modifier values, gender inheritance rules, and dated title law assignments. A law definition is **not** proof of a character's actual historical law or of whether a DLC is active.

**Implemented:** `law_core.c/.h` parses direct CK2 law fields, groups, succession and gender keys, explicitly stored `law = ...` assignments in dated title history, castle/city/temple/tribal vassal tax fractions, centralization's `demesne_size` and levy minimum values. It refuses to read values embedded inside nested scripted triggers/`effect` clauses as unconditional modifiers. The project loads the installed law files on new-game start, then loads explicitly authored laws from the selected ruler's primary title at the chosen bookmark date. Missing historical laws remain **unresolved**; it does not guess defaults from religion/government/DLC. `setlaw` is an intentionally unrestricted *developer override*, not a CK2 law vote.

**Economy:** `economy_snapshot_build()` now tracks the dated holder of each direct holding **and holdings of individually verified direct vassals**; the additional `economy_snapshot_apply_laws()` applies only explicit direct-field tax fractions to those known vassal holdings. Missing tax definitions/rules are counted and exposed by `taxes`. It does not count unknown holders, fabricated tax rates or unverified indirect vassals, and recomputes after construction and law changes without double-counting. Centralization's authored demesne-size bonus now applies on top of the existing demesne baseline. More complex building benefits, law approvals, opinion-based tax, levy contributions, religion-specific bishop payments, over-demesne penalties, tax technology, councillors, vassals' own tax income and legacy taxation exceptions are deferred. Other law categories parse but do not yet change gameplay.

**Succession:** console `heir` previews current selected succession/gender law. `die <current player ID>` runs the existing death system and, **only if** a supported hereditary law and an eligible child are verifiable, transfers in-memory landed titles to direct descendants (primogeniture, ultimogeniture, simplified gavelkind). Runtime children and bookmark historical children are both considered. Historical titled/unlanded children can become in-memory landed rulers. The new holder becomes playable; realm map, selected province, personal holdings, demesne limits and portrait refresh. Original CK2 `history/titles` remains immutable. For split inheritance, the highest rank goes to the first heir and remaining titles are shared round-robin: this is a transparent **temporary algorithm**, not the real CK2 de jure gavelkind allocator. Age comparisons use integer years with character ID for ties rather than exact birth timestamps. No dynasty-wide seniority, elective votes, Byzantine/papal/nomadic/Muslim succession, grandchild representation, election candidates, succession loss events, AI ruler death succession, disinheritance, law change cooldowns, legal `allow`/`potential`, primary-title per-title laws, claims, treasury partition, or game-over handling. When no supported heir is verified, the title holder stays unchanged and the console reports the unresolved succession instead of fabricating an heir.

**Console** (backtick): `laws` lists current primary-title and developer-selected laws; `laws succession`, `laws gender_succession`, `laws feudal_tax`, `laws city_tax`, `laws tax_levy_law_feudal` browse law IDs; `setlaw succ_primogeniture`, `setlaw agnatic_succession`, `heir`, `die <player ID>` demonstrate supported inheritance; `setlaw feudal_tax_2`, `setlaw city_tax_1`, `setlaw centralization_2`, `taxes` demonstrate data-defined taxes/demesne limit. These are **developer commands**, not the original CK2 console commands or a player-facing Laws window.

**Tests:** `make check-law-core check-law-economy check-realm-inheritance` covers law parser nesting, date-scoped history, original supplied CK2 laws, hereditary candidate ordering and distribution, explicit tax fractions, missing definitions and non-duplicated monthly recomputation. Existing economic, family lifecycle and console tests also remain green. Full SDL compilation, original GUI laws screen, UI verification, complete political map and succession visuals, and comparison against original CK2 remain to be tested on Linux Mint. The law-file test can use `CK2_LAW_TEST_FILES='/path/to/common/laws/demesne_laws.txt ...'`.

---

# OpenCK2 — implementation notes, plans, and test history

This document replaces scattered progress reports, Qwen checkpoints, work plans, testing lists and source layout notes. The **current C17 source tree is the implementation authority**; earlier stage reports are preserved below as historical snapshots and may describe unimplemented work that was completed later (or vice versa).

## Character lifecycle and initial warfare — C17 increment (2026-11-12)

**Source baseline:** `OpenCK2_Events_Console_C.zip`. No original game files are rewritten, and no C++ modules were added.

### Character lifecycle

- `family_gameplay_birth()` creates a playable runtime character record with actual parent IDs, dynamic ID, age 0, parental dynasty inheritance (including matrilineal marriage), and the campaign birth date. The `birth` / `charborn` console command creates a baby for the playable ruler and a verified spouse; it reuses the original `common/cultures` naming parser where possible. `conceive` still schedules a later birth.
- `family_gameplay_death()` records the death date, marks the runtime character deceased and clears known runtime spouse/pregnancy links. `die <character ID>` / `chardie <ID>` / `kill <ID>` can act on a character with either a runtime record or a resolved CK2 character-history record. Historical character files remain unchanged. A dead playable ruler is **not automatically replaced**; succession, inheritance, funeral events and historical AI deaths are future systems.
- Death is explicitly invoked, not automatically generated from guessed mortality probabilities. Dynamic children have no guaranteed save persistence. The high runtime ID range is distinct from ordinary CK2 historical character IDs.

### Warfare: a playable but provisional military loop

- `warfare_core.c/.h` stores runtime wars, armies, troop counts, movements, battle outcomes and occupations. `warfare_map.c/.h` builds an adjacency graph from the installed CK2 `map/provinces.bmp` raster, excluding water. Straight-line teleportation is not permitted; crossing one adjacent province takes two days **as a placeholder**. Straits, naval movement, supply, neutral-territory military access, and movement restrictions are missing.
- The console accepts `declarewar <rulerID>`, `peace <rulerID>`, `raise [provinceID]`, `raiseenemy <provinceID>`, `march <armyID> <provinceID>`, `armies`, and `disband <armyID>`. The player's own county must be held directly to raise a levy; enemy armies can only be raised for a declared opposing independent ruler. A county can contribute its potential levy only once in this snapshot to prevent free troop regeneration after disbanding. The province's existing Raise Levies button now calls the same validation path.
- Army strengths are drawn from `construction_holding_levy()` using historical holdings and completed buildings. Army counters appear on the campaign map; province hover shows present armies and military occupation. Two hostile armies meeting resolve a **deterministic simplified** battle; losing armies are removed, not properly retreated. An unopposed enemy army occupies a province after a **provisional 30-day siege**. Military occupation never transfers title ownership, religion, government or actual CK2 control effects.
- War identity currently uses independent ruler IDs from bookmark title history; non-independent revolts and detailed vassal obligations are unsupported. War declarations don't validate CK2 casus belli or diplomatic restrictions; peace clears occupations without implementing demands or warscore. No war AI, player succession, commander traits, phases/flanks, morale, tactics, naval fleets, attrition, siege garrisons, army reinforcements, war exhaustion, event chain, alliances or multiplayer state is claimed.
- Source-of-truth separation: `realm_map` contains dated CK2 title history; the warfare layer stores temporary military occupation independently. The renderer uses existing province projection and settlement positions. Graph routes are built lazily on the first march rather than slowing ordinary map navigation.

**Verification:** `make check-warfare-core check-warfare-map check-family-lifecycle check-console-warfare` passed. A 17-target SDL-independent regression sweep passed; the three new tests also passed with AddressSanitizer and UndefinedBehaviorSanitizer. Full `make -B game CFLAGS='-O2 -g -Werror'` and interactive gameplay still need Linux/SDL verification. No completed CK2 combat formula or event/death probability is asserted.

**Next high-priority implementation:** live landed-title ownership and succession, military recruitment pool/replenishment, armies with persistent force composition, actual CK2 movement/strait adjacencies and modifiers, army battle tactics/commanders and robust war/peace outcomes. All can continue in C17, with any later architecture rewrite explicitly deferred.

---

## Runtime religion and culture conversion — C17 increment (2026-11-12)

**Verified original CK2 source data:** `common/job_actions/00_job_actions.txt` defines the Court Chaplain `action_inquisition` using **Learning** and event references `900 901 902 903`; it defines the Steward `action_settle_tribe` using **Stewardship** and event references `925 20206`. The latter requires a **tribal ruler**, a province governed by that ruler, and different province culture; its `culture_conversion=restricted` rule adds province-neighbor/port qualifications. Inquisition's province-owner and independent-conversion trigger checks are more elaborate than simple cultural/religious mismatch, including special pagan cases. These are observed script facts, **not** fully evaluated CK2 triggers.

**Implemented:** `conversion_core.c/.h` is an SDL-free mutable campaign layer. It parses nested religion/culture definitions (including DLC/mod content from the mounted asset set), validates keys, records province and ruler conversion effects without editing bookmark files, and stages one mission of each type based on the original `common/job_actions` entries. Country history, runtime birth records, and selected provincial culture/religion remain separate. `main.c` loads definitions at campaign start, adds developer-console controls, refreshes province text, settlement graphical culture, and historical Culture/Religion map-mode overlays, and reapplies changed ruler religion/culture after map/history reloads. `realms.c` exposes a mutable **in-memory** ruler profile accessor; the underlying CK2 history files are immutable.

**Console** (backtick): select a county on the map, then `faith orthodox` or `culture english` to apply a **developer event effect** to that province; `charfaith catholic` or `charculture norman` to change the current landed ruler; `missionfaith` / `missionculture` stage a council action on the selected eligible county targeting the active ruler's religion/culture. `conversions` shows staged missions; `resolvefaith` / `resolveculture` manually resolves the staged effect. Use actual CK2 keys; unsupported keys are rejected. These commands are developer tools, not replicas of CK2's player-facing conversion decisions or console syntax. Original localisation is used for selected province and character display values.

**Critical limitations:** The supplied base `common.zip` includes the two council job-action definitions but not their corresponding event scripts, MTTH modifiers, global `on_actions`, council appointments, religious conversion opinion/costs, spontaneous culture spread, province-neighbor checks or an evaluator for CK2's full scope/trigger language. Consequently staged missions **do not automatically succeed or spend piety**; direct and resolve commands are explicit developer-effect injection only. `missionculture` currently requires the player's recorded `tribal_government` and direct county holding; `missionfaith` limits targets to historically in-realm counties. These checks are a conservative subset, **not full original action eligibility**. No saves or mod-trigger execution yet; state is lost on restart. Dynamic population religious and cultural modifiers, taxation/morale penalties, vassal and diplomatic relationships, and historical culture-based names are not recalculated by these effects. The game remains a bookmark-state simulation with some mutable overlays.

**Verification:** `make check-conversion-core` and `make check-technology-console` passed. Parsed uploaded source files: **56** authored faiths and **121** cultures from `00_religions.txt` and `00_cultures.txt`, plus the two council actions. The SDL-independent regression targets (20 executables) passed, as did strict source syntax checks with temporary SDL type stubs. A full SDL build and runtime screenshot comparison require the user's Linux installation.

**Next:** obtain original conversion event scripts (IDs 900–903, 925, 20206, and general culture spread events), implement CK2 scope evaluation and `on_actions` with actual event odds; add Chancellor/Chaplain/Steward assignment UI from original GUI files, realm/province restrictions, mod rules and runtime saves. Keep data and actual effects separate from provisional UI. **Do not invent monthly conversion probabilities.**

---

## Documentation map (new structure)

- [`../README.md`](../README.md): setup and quick start.
- [`../ROADMAP.md`](../ROADMAP.md): active work priorities and compatibility goals.
- [`EXE_RESEARCH.md`](EXE_RESEARCH.md): all original-executable reverse engineering, including addresses and the source-filename inventory.
- [`GAME_DATA_AND_UI.md`](GAME_DATA_AND_UI.md): CK2 loose-file support, mechanics, UI and settings backlog.
- **This file:** implemented features, historical milestones, tests, developer tools, deferred C++ considerations, and old Qwen notes.

### Current development guardrails

- Keep the engine in C17 unless an explicitly reviewed architectural reason justifies a different module; the deferred C++ list is only a suggestion, not a migration authorization.
- Preserve regression tests even when a local AI checkpoint says they failed. Diagnose the fixture or implementation, and do not delete assertions to achieve a green build.
- Do not use historical progress documents as proof of runtime equivalence or of current source state. Re-run relevant Makefile targets and compare against the installed CK2 game.
- Keep exact engine executable evidence under **EXE_RESEARCH.md**, not scattered implementation reports. Some archived reports below include context and links; the research document is the canonical executable-evidence record.


## Levies and historical map-mode overlay increment — C17 (2026-11-12)

**Baseline:** `OpenCK2_Province_Polish_C.zip`. No conversion to C++ or replacement of CK2's original loose-file definitions.

### Levy foundations (implemented and tested)

- `levy_core.c/.h` represents nine land troop types; tracks `levy_size`, `garrison_size`, `levy_reinforce_rate` and `fort_level` as data-defined modifiers. These are separate from actual raised armies or losses.
- `construction.c` now parses troop types and levy modifiers for buildings and reads base troops from `common/static_modifiers.txt`. History and completed projects are merged by existing building IDs and only the highest installed level in an upgrade chain contributes. The prior economy/tax code shares the effective-building selector, preserving its behavior.
- `construction_holding_levy()` supplies an **unraised potential** breakdown from a holding's base and completed buildings. The province's Raise Levies button displays an informational troop breakdown on hover; it does NOT create an invisible army. The read-only console command `levy` displays the selected province's potential.
- Tested the production Castle building parser against the supplied CK2 `00_Castle.txt`: the original `ca_militia_barracks_1` light infantry/archer values and `ca_keep_1` levy-size modifier were recovered correctly. A gameplay regression ensures completed building upgrades replace previous levy bonuses rather than stacking them. `make check-levy-buildings CK2_CASTLE_BUILDING_FILE=/path/to/common/buildings/00_Castle.txt` requires the user's installed loose file.

### Campaign map modes (11 selectable, 7 deferred)

| Mode | Status / data source |
|---|---|
| Terrain, Realms | Previous working renderer preserved |
| Religion, Culture | Province history keys; authored `common/religions/` and `common/cultures/` colors where present |
| Economy | Estimated annual sum of historical holding/base/building taxes, **not** the full taxable realm economy |
| De jure Duchies/Kingdoms/Empires | `common/landed_titles` de jure ancestry and authored title colors where available |
| Dynasties | County title holder's recorded dynasty ID; stable fallback palette |
| Direct Vassals | County title's recorded immediate liege; **not** verified CK2 full character-vassal display rules |
| Governments | Only authoritatively recorded character-history government ID; original government color when defined |
| Diplomacy, Revolt Risk, Opinions, Trade Zones, Coalitions, Epidemics, Players | Not selected until gameplay data/state exists to render them honestly |

`map_themes.c` renders a perspective-aligned, translucent overlay on the existing `map_view_pick()` geometry. Category lookups are cached between camera movements so panning does not re-read CK2 history/culture files. `map_palette_core.c` parses original RGB float palettes from the CK2 religious and cultural definitions; scripts outside palette definitions are not executed.

**Important limits:** neither CK2's exact map-coloring shader nor its conditional logic is recovered from this work. Color values fall back to a deterministic palette if a referenced item lacks authored colors. Unknown government or province data remains uncolored. The map is a bookmark snapshot: date advancement does not yet change title holders, disease, vassal contracts or opinion state.

**Tests:** `make check-levy-core check-economy-construction check-map-core check-map-themes-core check-map-palette check-technology-console`. Optional original-data tests run with `CK2_PALETTE_FILE` and `CK2_CASTLE_BUILDING_FILE`. The supplied CK2 religion and culture palette files passed in this environment. Full SDL game compilation and runtime screenshots must be verified on Linux Mint with the user's installed dependencies and CK2 assets.

**Future order:** integrate actual available/raised levy pools, game-date reinforcement, army entities and selection/movement; then military diplomacy, war/opinion, disease, trade zones and player ownership for the remaining map modes. Do not pretend current estimated potential troops are immediately raisable or vassal-accessible. These can remain in C unless a concrete architectural need justifies another language.

## Province rendering and holding-state cleanup — C17 increment (2026-11-12)

**Source baseline:** `OpenCK2_Holdings_Graphics_Reconstruction_C.zip`.
This is an additional source pass, not a verified screen-perfect recreation.

### Confirmed sprite-sheet defect

`interface/province.gfx` defines **both** `GFX_province_capitol_owner_frame`
and `GFX_province_settlement_owner_frame` as four-frame horizontal
sprite sheets (`noOfFrames=4`). Previously `load_holding_layout()` retained
only the ordinary frame texture and `draw_campaign_holdings()` copied the
**entire** four-frame sheet into every card. This produces oversized opaque
blocks extending across neighboring holding art and even into the map.

The new code retains separate `struct setup_widget` instances for capital
and ordinary holdings, draws only a validated single cell with
`ck2_sprite_frame_rect()`, and retains the original GUI-local offsets. The
first frame is selected until original CK2's dynamic frame/state rules have
been verified. It does **not** pretend to implement the unknown state logic.

### Province text and interaction consistency

- Header fields now respect `province.gui` text alignment: name centered,
  title hierarchy and labels left aligned, tax/culture/religion values right
  aligned. They clip to their GUI text bounds; smaller UI font is preferred
  for these dense fields and longer translations.
- Both the building-click target and holding hover target share the same
  `holding_frame_rect()` function as the renderer, including culture-specific
  holding-frame images when available. This prevents mismatched draw/hit
  rectangles after a graphical-style change.
- The religion-to-graphical-style lookup uses `religion_style_cache.c` (bounded
  C cache). The resolver still reads original mod/DLC priority loose files on
  cache misses. Holding sprites are now reloaded by **graphical style pair**,
  not by a different religion ID sharing an identical style. The cache is
  session-scoped; changes to mounted loose files need a restart/reload.

### Source and checks

Changed: `src/game/main.c`, `Makefile`, `docs/DEVELOPMENT.md`, and
`docs/EXE_RESEARCH.md`. Added:
`src/game/sprite_frame_core.{c,h}`,
`src/game/religion_style_cache.{c,h}`,
`tests/sprite-frame-core.c`, and `tests/religion-style-cache.c`.

`make check-sprite-frame check-religion-style-cache check-province-layout`
and 15 other SDL-independent regression targets all pass with `-Werror`
(17 test executables in total).
New components also pass UndefinedBehaviorSanitizer. Targeted `main.c`
syntax checks with test-only SDL shims found no new non-shim warnings;
**full SDL compilation and screenshot comparison are pending on Linux Mint**.

To verify in game: visit Middlesex in 1337, select another county and return,
compare capital vs ordinary card backgrounds and check no four-frame sheets
spill across other holdings; click and hover each populated holding to confirm
only that card responds. Check Catholic vs Orthodox temple visuals as well as
Muslim/other graphical cultures, and switch the interface language to see
whether the narrower statistics labels remain readable. Existing incomplete
levy/supply and generated historical holding owner data remain separate tasks.

## Province GUI and holding-art reconstruction (2026-11-12)

**Baseline:** `OpenCK2_Province_View_C.zip`; the code remains **C17**.
This update is a source-level reconstruction from `interface/province.gui`,
`interface/province.gfx`, the actual OpenCK2 C implementation, original
`common/religions` definitions, and side-by-side CK2 screenshots. It is **not**
a recovered CK2 renderer or an in-game verified pixel-perfect clone.

### Root causes traced and corrected

1. The panel used the declared `province` window size (1024x224) as if it were
   the visible panel, despite CK2 using an approximately 344px-wide, 624px-high
   bottom-left art panel. The original `settlements_1_box` and
   `settlements_2_box` are deliberately 520px wide to support an extended view.
   Unclipped rendering let slots cover the map. `province_layout_core.c`
   constrains the grid to the visible artwork (three entries per row) and
   supplies the same hit and drawing origins. An SDL clip bounds the card
   graphics. The original `capital_position` is independent of `province`;
   row-box positions are children of `province`.
2. **Populated settlements became black rectangles** because the original
   `GFX_province_settlement_owner_frame` was rendered **after** the castle/city/
   temple placeholder, covering the painting. Empty slots were unaffected.
   It now draws as a backdrop **before** the settlement art. This ordering is
   supported by the original `settlement_entry` and `capital_entry` widget order.
3. The old panel showed custom diagnostic strings in CK2's portrait and
   province statistics areas, instead of using the authored GUI positions.
   Those strings have been replaced by the actual panel widgets, names,
   hierarchy positions and original stat icons; unsupported values are omitted.
4. The province close button used an undeclared `point` variable in the mouse
   handler. That compiler error has been corrected.

### Data-driven UI bindings

- The panel background now loads `GFX_province_bg` via `province.gfx` and
  draws to the original control span; the old `hud_province` is a fallback.
- County holder portrait and original frame, county/title shield, top-liege
  shield, province title, culture and religion labels, duke/kingdom hierarchy,
  military and technology icons, tax icon, close button and settlement cards
  use original GUI/GFX widget names.
- Title hierarchy in the panel now calls `realm_dejure_parent_info()` to read
  original `common/landed_titles` parentage, rather than assuming the changing
  de facto liege is the same as the de jure Duke/King. Dynamic de jure drift
  is not yet simulated.
- For an occupied holding, the small owner crest is the title-history **holder's
  primary landed title shield** (where a real texture exists), not an arbitrary
  copy of the county shield. Missing historical holders do not receive an
  invented crest.
- `religion_gfx_core.c` resolves a religion's `graphical_culture` from the
  original `common/religions` group/religion rules. Temple imagery selects
  `GFX_settlement_temple_<province graphical culture>_<religion gfx>` with the
  original western texture as fallback. Castle and city imagery continue using
  the culture's registered placeholders.
- A visible annual base-plus-built-holdings tax is calculated from the
  existing economic catalog; **no fake levy, garrison, revolt risk, supply or
  technology levels** are displayed. Those require their original simulation.

### Validation

- `make check-province-layout` — passed with `-Werror`. Verifies all settlement
  origins remain in the panel; a deliberately narrow panel rejects overflow.
- `make check-religion-gfx` — passed with `-Werror`. Validates inherited and
  overridden religious graphical culture.
- Original `common/religions/00_religions.txt` probe matched catholic/orthodox
  `westerngfx`, sunni/shiite `muslimgfx`, norse `norsegfx`, Jewish `jewishgfx`.
- Map-core, government, settlement, map-position, economy-construction,
  technology/console, dynasty-arms, tree layout and title-prestige tests passed.
- Changed C files passed targeted syntax checks with test-only SDL stand-ins
  and `-Wall -Wextra -Wpedantic -Werror`, but these stand-ins cannot substitute
  for a real SDL build or in-game visual inspection.

### Remaining fidelity gaps

- Exact texture size/scale and portrait/title crest placement need comparison
  on Linux with the same resolution and bookmark as the CK2 reference.
- Province panel extension variants, wonder/trade/fort/hospital sections,
  special siege state, barony controller shields and hover interaction remain
  incomplete. Original CK2 progress bars and levy numbers need a real troop
  and military statistics model, not placeholder percentages.
- CK2's exact province income, fort/garrison values, tech values, supply,
  revolt risk, holding construction shading and province-level modifier icons
  remain unimplemented. The shown annual tax is a limited base-income summary.
- Dynamic title ownership, dejure drift, generated barony CoAs and per-holding
  independent cultural change need further work.

### Linux visual acceptance checklist

```sh
make -B game CFLAGS='-O2 -g -Werror'
make check-province-layout check-religion-gfx check-map-core check-economy-construction
```

- Select Middlesex in 1337 and compare the ruler portrait, England/Essex
  shields, county title, labels, and settlement artwork at the same resolution.
- Confirm that **every populated** castle/city/temple has visible art (not just
  vacant land) and that no holding card crosses into the map.
- Check occupied and empty slots, tooltips and construction clicking: the
  clickable hitboxes must coincide with the visible cards.
- Repeat for Catholic, Muslim, Norse and Jewish temples and a different
  graphical culture to check culture-and-religion-specific art.
- Close the province panel and reopen it, switch counties, then complete a
  building and verify the economic data and artwork remain consistent.

## C17 gameplay increment — founded holdings and construction income (2026-11-12)

**Source baseline:** `OpenCK2_Tree_Layout_C.zip` (the latest prior released source archive).
All CK2 history and economic definition inputs remain mounted loose files, not
hardcoded historical names or example amounts. No original `FreeCK2game.exe`
source-code algorithms are claimed to have been recovered for this increment.

### Fixed in source

- `economy_snapshot_build()` now records province IDs for counties actually
  held by the player in the selected bookmark's title history. It does **not**
  assume the player owns every holding in their top-liege realm.
- `economy_snapshot_refresh()` now incorporates newly constructed settlement
  holdings **after completion** and only when their county belongs to that
  owned-county set. Repeated refreshes do not count holdings twice, and an
  unowned county does not contribute income. Authored base tax and building
  upgrades are still calculated by `construction_holding_annual_tax()`.
- `construction_settlement_count()` and `construction_settlement_info()` expose
  read-only settlement order records to the economic subsystem without making
  the construction module's internal structs public.
- `construction_day()` now reports a completed project; `advance_campaign_day()`
  refreshes direct-holding monthly tax, the 'known' flag, and the demesne size
  immediately when a building or holding completes. The treasury still receives
  **monthly**, not daily, income at the campaign's existing month boundary.
- Invalid construction catalog indexes, missing required project arguments,
  and absent requirements strings now fail safely instead of dereferencing
  invalid input. This is defensive C code, not a recovered CK2 gameplay rule.

### What remains deliberately out of scope

- Ownership/liege/title changes **after** the selected bookmark and baron
  creation mechanics. The owned-county set is a historical snapshot, not a
  simulated feudal ownership graph.
- Vassal tax laws, church/realm tax, modifiers, upkeep, levy income, and
  government-specific income rules. `monthly_tax` is only known direct-holding
  income, not the original game's complete tax calculation.
- Save/load of newly founded holdings; current campaign construction remains
  in memory. 3D building models and map refresh on capital changes are separate.
- Original CK2 AI, succession, event effects, and campaign-wide ownership
  simulation. A different future data model may benefit from C++ but was not
  introduced into this C17 increment.

### Reproduce / test

```sh
make check-economy-construction
make check-map-core check-economy-core check-technology-console
make -B game CFLAGS='-O2 -g -Werror'  # requires actual SDL2, SDL2_image, SDL2_mixer
```

The new SDL-free regression links the real `construction.c` and `economy.c`
with unused SDL asset-loading functions stripped. It tests a two-day city
founding, a two-day market building, a three-day market upgrade, tax-base
replacement, duplicate prevention, and excluding another county's income.
It also tests malformed construction indexes and uses `-Werror`. A separate
AddressSanitizer + UndefinedBehaviorSanitizer invocation passed during the
implementation session. The graphical game was **not** launched or fully built
in this environment because SDL development libraries are unavailable;
runtime acceptance remains necessary on Linux Mint.

**Manual acceptance:** start with a known directly held county that has free
slots; found a City and observe treasury deduction; advance days and verify
its tax and demesne count rise only on completion; then build a taxable market
and upgrade it, checking the monthly HUD rate changes on the finishing day.
Switch provinces and come back to ensure the new settlement remains visible.
Separately confirm that income from a foreign county is not added.

## Migrated source-file index

The entries below were consolidated into this document, EXE_RESEARCH.md, or GAME_DATA_AND_UI.md. Original notes are identified by source path and SHA-256 in their migrated sections. `RESEARCH_BLOODLINES_GOVERNMENTS.md` was byte-identical to `OpenCK2_research_report.md`; only one copy was retained. The `research/` folder remains because it holds actual analysis scripts, instruction dumps, and machine-readable cross-references.

| Old Markdown path | New file / retained note | Original SHA-256 |
|---|---|---|
| `OpenCK2_UI_Map_Implementation_Plan.md` | `docs/EXE_RESEARCH.md` | `45e453288398f1c6c5e08a8cb6bad31556060d77fa0895b341e2cbe6aa4114c3` |
| `OpenCK2_research_report.md` | `docs/EXE_RESEARCH.md` | `d204b3cd0a99c48b75c3205a7e401f4f9d0ddbfe3b74abdbcb19befb378c13b5` |
| `RESEARCH_BLOODLINES_GOVERNMENTS.md` | `EXE_RESEARCH.md (exact duplicate removed)` | `d204b3cd0a99c48b75c3205a7e401f4f9d0ddbfe3b74abdbcb19befb378c13b5` |
| `docs/CK2-MECHANICS.md` | `docs/GAME_DATA_AND_UI.md` | `c0e76fe2f3542d2e3be70229d5868f54bd68b919fea29e0b3d8ed990f1328bc2` |
| `docs/IMPLEMENTATION.md` | `docs/DEVELOPMENT.md` | `ce800e3a14052b26b8e6372cb18cbd3fe975b39aaae0c07bc364e3a8f67d0030` |
| `docs/LOOSE-FILES.md` | `docs/GAME_DATA_AND_UI.md` | `6d914d26eb6b6b3b07054710a2b08cd2c28f372efa7ead3b1cc18fef138819ba` |
| `docs/PROJECT-ORGANIZATION.md` | `docs/DEVELOPMENT.md` | `4132f56ef1bf92ab414a153aaef6ecbe4b39c2a514dc3c37190acc8c2a90af08` |
| `docs/QWEN-144-HOUR-PLAN.md` | `docs/DEVELOPMENT.md` | `935710b1db9f4e75974f8d420f102c3b5d8442eb39bf97658c14ac4dbb9a2211` |
| `docs/QWEN_PROGRESS_REVIEW.md` | `docs/DEVELOPMENT.md` | `0aaba892afcb17a7a6879e97923f0fbb7f96da5b491424ff07abad12b5a3024e` |
| `docs/README.md` | `docs/DEVELOPMENT.md` | `e311f5dc8200b5c4a71ad77847612d42b76de17b146672b26d4b99b14e8b657a` |
| `docs/SETTINGS-TODO.md` | `docs/GAME_DATA_AND_UI.md` | `4f51fdead4b2da18389aff566429cd2ce1c81d5043af98763c72b7b1c079fad0` |
| `docs/TECHNOLOGY_CONSOLE_PROGRESS.md` | `docs/DEVELOPMENT.md` | `43c6ba36a470246fac4dabd2922e24c1661c7de4fb3c94e4475147b032baeada` |
| `docs/UI-TODO.md` | `docs/GAME_DATA_AND_UI.md` | `ca2d161fadfdb187d2337da1896cb8c8469236608f04747d01305dbbd396a193` |
| `docs/implementation/ECONOMY_AND_PROVINCE_HISTORY_C_INCREMENT.md` | `docs/DEVELOPMENT.md` | `650b5e18cba21a1c5e9bfa5385f218511ea3f6d78a478c89952b69d67cc421c0` |
| `docs/implementation/MAP_HOLDINGS_GOVERNMENTS_C_INCREMENT.md` | `docs/DEVELOPMENT.md` | `a9dfff7bd28827561758f9ac5bf03ed42f43e12d1d406d11ae3215e9fe881bc2` |
| `docs/reverse-engineering/ARCHITECTURE.md` | `docs/EXE_RESEARCH.md` | `da9d3ceaf5a89a9f19f4af90d42f2fe17ef9c44725b6d9068d694c2c2604deb3` |
| `docs/reverse-engineering/BOOKMARKS-AND-CURSORS.md` | `docs/EXE_RESEARCH.md` | `3d65255af92e809ce1f1ad6c6821beeb615f67c2eb6cb23b15c0a68d8511dc2c` |
| `docs/reverse-engineering/CONSOLE-FINDINGS.md` | `docs/EXE_RESEARCH.md` | `26aaf7603196ddd61afdf2c3e12a63e919f332ddb28b98770f33a2a47620358c` |
| `docs/reverse-engineering/CONSTRUCTION.md` | `docs/EXE_RESEARCH.md` | `1a34788b68aa4583ba3ea1acbdf12e7733ce27ff984552bb29ce8255f15b563f` |
| `docs/reverse-engineering/FINDING-TEMPLATE.md` | `docs/EXE_RESEARCH.md` | `3a3106c851bc96ad8352a7097054a7baa79545ca55092cbde819d922bf73ef4e` |
| `docs/reverse-engineering/LAUNCHER-DECOMP.md` | `docs/EXE_RESEARCH.md` | `f172db01978556fa133e53d4be25e6ac856d5d0dc6de561a0a477db350f25642` |
| `docs/reverse-engineering/MAP-RENDERING.md` | `docs/EXE_RESEARCH.md` | `ae3c178f8906274ed9f6de8c9ddc107c9d527761cd70b9cab3da61d5ae497a2f` |
| `docs/reverse-engineering/PORTRAITS-AND-RULER-PANEL.md` | `docs/EXE_RESEARCH.md` | `11dff3e48740ce0c4d68d5d97709423b08c6d1715af55f50bfba3fd0920088bf` |
| `docs/reverse-engineering/PROVINCE-HOLDINGS.md` | `docs/EXE_RESEARCH.md` | `84d48953b5f451a31c1ab02ae20282de050ca389fc0fcfb31ba1ca6e2beeec5d` |
| `docs/reverse-engineering/README.md` | `docs/EXE_RESEARCH.md` | `97d0d09eab19c14f1065bfdb96c4fed54fe7c256590eb200db049e6811ab5953` |
| `docs/reverse-engineering/STARTING-ERAS.md` | `docs/EXE_RESEARCH.md` | `93ec1cde1d298a2c8012d8e922435ff8d57c146b10bc3805b1643558e9493c3f` |
| `docs/reverse-engineering/source_filenames_report.md` | `docs/EXE_RESEARCH.md` | `e757354e81244ebe7b681f72e3d536ec1a084383b3fbb7a9abc538b750c60078` |
| `notes/CPP_DEFERRED.md` | `docs/DEVELOPMENT.md` | `1cd5c39a5c88f0d73cfbdc8da9ce91d6013b447ffd00c08db3d7f96675d52a0e` |
| `notes/C_CORE_PROGRESS.md` | `docs/DEVELOPMENT.md` | `382c315a71702b9bf33c05f4a69d3123d79b2a44a229a717947c37cf2f626084` |
| `notes/TEST_PLAN.md` | `docs/DEVELOPMENT.md` | `013a0ffd67a2b98996bad63091d50201e7c6f92588228ee8265d765ed412445d` |
| `tools/README.md` | `docs/DEVELOPMENT.md` | `2809ef23249c72eb80185d3b901b97c82a25ce0cd242aa3db191e38ec12cb72f` |

## Preserved implementation notes

<a id="source-docs-readme-md"></a>
## Source note: `docs/README.md`

> Preserved from the prior source archive. Original SHA-256: `e311f5dc8200b5c4a71ad77847612d42b76de17b146672b26d4b99b14e8b657a`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 documentation
| Document | Use it to |
| --- | --- |
| [Implementation inventory](DEVELOPMENT.md#source-docs-implementation-md) | Find working features, source files, and current limits. |
| [Loose-file compatibility](GAME_DATA_AND_UI.md#source-docs-loose-files-md) | See which original files are consumed, what is missing, and how to verify each migration. |
| [Bookmarks and cursors](EXE_RESEARCH.md#source-docs-reverse-engineering-bookmarks-and-cursors-md) | Find the original extra dates, scrollbar, and pointer art, plus implementation plans. |
| [Vanilla CK2 mechanics](GAME_DATA_AND_UI.md#source-docs-ck2-mechanics-md) | Understand the rules and game systems OpenCK2 aims to reproduce. |
| [Province and holding audit](EXE_RESEARCH.md#source-docs-reverse-engineering-province-holdings-md) | Reproduce the Călărași fix and verify map-wide holding history. |
| [Portraits and ruler panel](EXE_RESEARCH.md#source-docs-reverse-engineering-portraits-and-ruler-panel-md) | Check the 936 regression, supported portrait inputs, and remaining vanilla differences. |
| [Reverse engineering](EXE_RESEARCH.md#source-docs-reverse-engineering-readme-md) | Find REA evidence, architecture notes, and the finding template. |
| [Repository organization](DEVELOPMENT.md#source-docs-project-organization-md) | See how the Fallout: New Vegas decompilation project separates its tools and source, and how OpenCK2 applies that pattern. |
| [Settings backlog](GAME_DATA_AND_UI.md#source-docs-settings-todo-md) | Check which settings apply and which still need implementation. |
| [UI TODO](GAME_DATA_AND_UI.md#source-docs-ui-todo-md) | Track setup and gameplay UI changes, the original source files, and all deferred controls. |
| [Roadmap](../ROADMAP.md) | Choose an investigation or implementation milestone. |
| [Contributor instructions](../AGENTS.md) | Get started as a human contributor or AI agent. |
| [Tools](DEVELOPMENT.md#source-tools-readme-md) | Build and run the standalone local probes. |

---

<a id="source-docs-implementation-md"></a>
## Source note: `docs/IMPLEMENTATION.md`

> Preserved from the prior source archive. Original SHA-256: `ce800e3a14052b26b8e6372cb18cbd3fe975b39aaae0c07bc364e3a8f67d0030`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 implementation inventory
OpenCK2 is a C17 engine reconstruction. The current build runs on Linux and
reads the player's local CK2 installation. The [roadmap](../ROADMAP.md) tracks
parity and the Windows and macOS targets. The [vanilla mechanics guide](GAME_DATA_AND_UI.md#source-docs-ck2-mechanics-md)
describes the reference game separately from the current implementation.

#### Executables and content
| Feature | Implementation and current limit |
| --- | --- |
| Steam reference runner | [`src/launcher/`](../src/launcher) starts the original Steam app. The separate startup, lifecycle, and content models cover the recovered bypass branch, launcher actions, Continue filename selection, and selection restoration. [REA evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-launcher-decomp-md) |
| OpenCK2Launcher | [`src/open_launcher/main.c`](../src/open_launcher/main.c) implements the fixed-size launcher, local CK2 styling, separate DLC and Mods master toggles, categories, store links, and the installation picker. Play starts the adjacent native game. |
| Installation and catalog | `install.c` finds and remembers the Steam installation. `catalog.c` discovers descriptors and persists selections. DLC availability is based on local files and installed-depot records. Steam account entitlement checks remain pending. |
| Content resolver | `asset_paths.c` and `zip_assets.c` mount enabled DLC and folder or ZIP mods at startup. Mods take precedence over DLC and base files. Supported reads use the overlay and honor `replace_path`. Arbitrary CK2 script execution, full conversion support, and full load-order parity remain pending. |

#### Frontend and campaign
| Feature | Implementation and current limit |
| --- | --- |
| Main menu, credits, and settings | [`src/game/main.c`](../src/game/main.c), `assets.c`, and `settings.c` draw the installed frames, title art, bitmap fonts, DLC icons, and version plate. Options applies window mode, windowed resolution, and volume controls during the session. See the [settings backlog](GAME_DATA_AND_UI.md#source-docs-settings-todo-md) for unfinished controls. |
| Music and sound | `music.c` plays the local main theme and loads cues from the installed sound definitions. Ruler selection uses the country-selection clips. Ambient and contextual playback remain pending. |
| Single Player and rules | `era.c`, `rules.c`, and `ck2_script.c` read five era bookmarks, localization, featured rulers, biographies, and game-rule definitions. The rules screen supports filtering, changing options, reset, and a local preset. Rule effects, Ironman, and achievements remain pending. |
| Setup layout | `frontend_gui.c` indexes the installed frontend tree. `setup_toolbar.c` binds widgets across the four setup panels and resolves their sprites through the installed `.gfx` definitions. See the [loose-file compatibility audit](GAME_DATA_AND_UI.md#source-docs-loose-files-md) for coverage and exceptions. |
| Historical setup | `realms.c`, `rulers.c`, and `holdings.c` read dated title, character, and province history. Setup distinguishes county holder, primary title, and independent realm. The panels expose culture, religion, dynasty, lieges, vassals, and dated holding slots. Barons without recorded holders are still unassigned. |
| Portraits | `portraits.c` and `era.c` use installed sprite and portrait definitions, DLC, age and period variants, DNA, properties, and rank frames. Historical portraits and bookmark thumbnails can use different properties. Scripted clothing and dynamic dynasty arms remain incomplete. |
| Terrain and Realms | `map_view.c` and `realms.c` render a pannable, zoomable map, CPU terrain and water shading, projected picking, political colors, province and sea-zone borders, and territory-based labels. The map uses the starting historical snapshot. The full vanilla rendering pipeline and live political changes remain pending. |
| Campaign prototype | `campaign.c` supplies daily advancement, fixed monthly income, and holding upgrades. Continue resumes the campaign in the current process. Save compatibility, the full economy, succession, diplomacy, combat, events, and AI remain milestones. |

See [starting-era and UI evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md)
and [terrain and water evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-map-rendering-md) for
the installed inputs, REA findings, and known differences.

#### Build and verification
Run from the repository root.

```sh
make launcher game title-history check-launcher check-campaign CFLAGS='-O2 -Werror'
```

The tests in [`tests/`](../tests) cover launcher model branches and the
prototype campaign. The standalone [title-history probe](DEVELOPMENT.md#source-tools-readme-md)
checks a dated holder in a local game file. Visual and gameplay parity
requires matched reference scenarios. The [evidence template](EXE_RESEARCH.md#source-docs-reverse-engineering-finding-template-md)
records the build, inputs, results, and remaining differences.

---

<a id="source-docs-project-organization-md"></a>
## Source note: `docs/PROJECT-ORGANIZATION.md`

> Preserved from the prior source archive. Original SHA-256: `4132f56ef1bf92ab414a153aaef6ecbe4b39c2a514dc3c37190acc8c2a90af08`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Repository organization
The reference is [momusdigital/fnv_decomp at 91e1b6e](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6).
Its matching targets are listed in a per-unit manifest and checked with the
original compiler toolchain. OpenCK2 adopts its separation of implementation,
tooling, build recipes, and evidence. Our current acceptance checks target
documented behavior and reference scenarios.

| Reference directory | What it contains | OpenCK2 practice |
| --- | --- | --- |
| [`src/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/src) and [`include/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/include) | Reconstructed C++ implementation units and reusable declarations. | Keep C17 runtime code under `src/` by subsystem. Private headers stay with their implementation. A public header directory can follow an actual cross-module API. |
| [`tools/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/tools) | `msvc.py` runs historical compiler tools, `build_and_match.py` builds units and compares them against a local executable, `compare.py` checks relocations, `function_spans.py` defines function extents, and `compare_builds.py` checks changed units against the base build. | The independent title-history probe lives under [`tools/`](DEVELOPMENT.md#source-tools-readme-md), with a dedicated Make target. Add repeatable scenario and comparison tools here as they become available. |
| [`build/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/build) | Tracked metadata includes `units.json` for source files, compiler options, and function bindings, and `image_check.json` for reference image checks. | The root Makefile is the current build recipe. Keep generated files under ignored `bin/`. Add versioned reference manifests when comparisons need machine-readable inputs. |
| [`.github/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/.github) | Workflow and scripts for build comparisons. The pull request check uses two builds and can run without the commercial game files. | Keep automated checks under `tests/` and separate synthetic checks from local CK2 comparisons. Cross-platform CI is a roadmap milestone. |
| [`third_party/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/third_party) and [`patches/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/patches) | The reference vendors a specific zlib version, documents its license, and records changes in a separate patch. | OpenCK2 currently uses system dependencies. If vendoring becomes necessary, record the upstream version, license, and local patches together. |

The current layout separates runtime code in `src/`, automated regression
checks in `tests/`, independent probes in `tools/`, and prose in `docs/`.
Reference notes and reusable investigation evidence live in
[`docs/reverse-engineering/`](EXE_RESEARCH.md#source-docs-reverse-engineering-readme-md). The project
roadmap and contributor entry points stay at the root. No reference-project
code was imported.

---

<a id="source-docs-implementation-map-holdings-governments-c-increment-md"></a>
## Source note: `docs/implementation/MAP_HOLDINGS_GOVERNMENTS_C_INCREMENT.md`

> Preserved from the prior source archive. Original SHA-256: `a9dfff7bd28827561758f9ac5bf03ed42f43e12d1d406d11ae3215e9fe881bc2`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2: culture-aware settlement map + government definitions (C17 increment)
Status: **implemented as a bounded foundation, not CK2 map or government parity**. Source snapshot supplied in `OpenCK2.zip` on 12 November 2026. All changes in this increment are C17; no C++ migration.

#### What this increment does
1. **Original government definitions.** `src/game/government_core.[ch]` parses the group/type structure of `common/governments/*.txt`, color, preferred and allowed holding categories, selected property flags, prefixes and the raw `potential` block. `src/game/governments.[ch]` uses the existing mounted asset system to collect the files, accounting for enabled DLC/mods via that system. The raw trigger is preserved, **not executed**. Candidate lookup is explicitly only a holding preference, not assignment. If character history explicitly includes a `government` value, `rulers.c` preserves it and the character tooltip identifies it as authored history.
2. **Graphical-culture lookups.** `src/game/settlement_styles.[ch]` reads `common/cultures/*.txt` and `interface/settlements.gfx`. It resolves a culture's authored graphical-culture group, its color, and settlement actor name/path by holding type. It does not interpret mesh format, materials, or building level.
3. **Province positions.** `src/game/map_positions.[ch]` parses initial `position = {x y ...}` values from `map/positions.txt` when present and verifies whether the y-axis mapping points back into the correct province. Otherwise scene initialization chooses a sampled province center. Both paths use the original map's coordinate system; no arbitrary ruler/province IDs are hardcoded.
4. **Capital-holding scene.** `src/game/map_settlements.[ch]` loads one *capital* holding per populated land province at the bookmark date, preserves holding ID and type, localizes the holding name, consults explicit barony title ownership (with an explicitly labeled county-holder fallback), reads owner/province culture, resolves the actor ID, and stores world positions. Map `main.c` renders each marker by projecting into the existing perspective terrain camera. It shows a name at close zoom and an owner name at the closest zoom; the 2D icon/outline is color-modulated by authored culture color. Hover shows holding ID, name, ownership confidence, graphical actor and government status. The existing province panel's individual holdings now expose explicit barony ownership on hover.
5. **Map projection.** `map_view_project_ground()` projects authored/sampled world coordinates using the same camera and elevation logic as the map renderer instead of a separate flat projection.
6. **Integration.** New sources, headers, and SDL-free tests are added to the Makefile. Scene and catalogs are loaded at campaign initialization and released at shutdown; map markers are not recalculated each draw.

#### Evidence and confidence
- **Directly verified [HIGH]:** the user supplied five government definition files and culture files, containing 12 parsed government types and 122 parsed cultures; `interface/settlements.gfx` contains 138 identified actor assets. The parsers operated successfully on those files. The project already provides dated province holdings and title-history lookup.
- **Source-supported [HIGH]:** exact code additions and original parsing targets; the C unit tests and SDL-free checks below passed.
- **Inference/design [MEDIUM]:** using an explicitly authored barony holder when available and treating an absent barony holder as a labeled county-holder fallback; exact CK2 autogenerated holding owners are not recovered. A culture-colored 2D marker is a temporary visual design, not the original graphical-culture appearance.
- **Not verified [LOW]:** in-game positions and appearance, native full-game compilation, original date-dependent government selection, full province/holding culture fallback rules, all CK2 inheritance of culture/model properties, actual map models or all barony marker placement.

#### Tests run in the analysis environment
From project root:

```bash
make check-ck2-map-data check-map-core CFLAGS='-O2 -Werror'
```

Results: `government core`, `settlement styles`, `map positions`, and existing `map core` assertion tests passed. The three new tests were also compiled independently with GCC C17, warnings-as-errors and UndefinedBehaviorSanitizer; all passed. Government and culture/actor parsers were exercised on the user-provided original CK2 definition files.

For the SDL-dependent data loaders, `governments.c`, `map_settlements.c` and changed `rulers.c` passed an additional **syntax-only** compile against a small test-only SDL header shim. **That is not equivalent to a real SDL build**.

**Cannot yet verify here:** `make game`, gameplay startup, actual textured rendering, original meshes, camera coordinates on real game assets, DLC visibility, or historical barony ownership. SDL2, SDL2_image and SDL2_mixer development headers are missing from this execution environment. Your Linux Mint installation previously compiled OpenCK2 successfully; test the source on that machine.

Suggested local commands:

```bash
make -B game CFLAGS='-O2 -g -Werror'
make check-ck2-map-data check-map-core CFLAGS='-O2 -Werror'
```

At date 1066 or 1337, zoom close to the Bosporus, hover castle/city/temple icons, and check their holding IDs and explicit owners against the province panel. Repeat with a non-Western culture. Capture any compiler output or screenshots, and use the original screenshot at matched camera, date, resolution, assets and enabled DLC.

#### Remaining work (prioritized)
- **P0: Native build and visual tests.** Validate camera-based marker positions, clickable areas and no crashes; fix any warnings from `make game CFLAGS='-O2 -g -Werror'`.
- **P1: All holding slots on the actual map.** Currently one capital marker per province is shown. Read authored settlement positions/rotation/scale for secondary holdings from original map data and verify how CK2 displays the individual settlement models. Do not invent arbitrary per-barony coordinates.
- **P1: Actor/mesh/material renderer.** Resolve settlement actors' `.asset`/`.mesh`/texture and model-level variations, zoom levels, map shadows, rendering pass order and world ownership. The current 2D icon tint is only a placeholder.
- **P1: Government assignment.** Implement and test scripted `potential` predicates, character/realm/holding scope, religious and nomadic/merchant-republic exceptions, and dynamic changes. Do not infer a ruler's government from `preferred_holdings` alone.
- **P1: Generated barons and titles.** Complete the missing historical holder/liege fallback, record provenance, and use a shared dated simulation snapshot.
- **P2: Holding culture.** Distinguish province culture, founder culture, owner culture, and authored holding culture/model; test actual original CK2 precedence and date-dependent changes.
- **P2: Government-specific gameplay.** Apply actual government eligibility, succession, army/levy/raid/economy rules and interface state only after their engine consumers are modeled and compared to the reference game.
- **P2: Campaign time.** The map uses bookmark-date holdings and historic titles; campaign date advancement is not a full historical/simulated ownership transition yet.

#### C vs. possible later C++
**Keep in C:** CK2 script lexing, government/graphical culture registries, holding positions, projection data, deterministic snapshot selection, input/tooltip infrastructure, and portable test harnesses. The original CK2 being implemented in C++ does not itself require C++.

**Consider C++ later; do not start now:** if native actor/mesh/material scene graph, complex resource ownership/caching, or large-scale government trigger evaluator become significantly easier to maintain with C++ containers and RAII. All of these remain viable in C. No source has been converted.

#### DLL ZIP inspection
The newly supplied `dlls and others.zip` includes platform/runtime libraries (`SDL2.dll`, Lua, Steam, PhysFS, TBB, Paradox services) and `tbb*.pdb`. **No matching `CK2game.pdb` was present.** These are useful for dependency/API context, not a substitute for the game's mesh definitions or gameplay code, and should not be bundled with this reconstructed engine source.

---

<a id="source-docs-implementation-economy-and-province-history-c-increment-md"></a>
## Source note: `docs/implementation/ECONOMY_AND_PROVINCE_HISTORY_C_INCREMENT.md`

> Preserved from the prior source archive. Original SHA-256: `650b5e18cba21a1c5e9bfa5385f218511ea3f6d78a478c89952b69d67cc421c0`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 — C17 economy and historical province attributes, increment 2
Baseline: `OpenCK2_Map_Holdings_Governments_C.zip` from the preceding OpenCK2 increment. No original CK2 data was bundled with the modified source. The code continues to read the user's installed loose files through the existing mounted asset lookup.

#### What was fixed
##### 1. Historical culture and religion (verified source integration)
- `holdings_read()` now reads root and dated `culture =` / `religion =` from `history/provinces/*.txt` using the selected bookmark date, independent of holding slots. Changes after the bookmark are not applied.
- `holdings_load()` localizes both values through the existing CK2 localization loader and still localizes holding names.
- `map_settlements_build()` preserves province culture and religion separately from the holding owner's own culture. A fact entry is stored for every province history file found, including provinces without an active holding. Previously, the map's one-capital culture was sometimes overwritten by the barony owner's culture; religion was discarded entirely.
- The selected-province panel and map/holding tooltips now expose these values, or explicitly say they are not recorded. The character's personal religion/culture and the province's religion/culture are different concepts.
- At campaign startup, the console reports the number of province histories loaded and how many contain a culture/religion key. Use that report to confirm your particular installed game's files are being read.

##### 2. Wealth (direct holding tax foundation)
- Removed the placeholder `5 + 2 * improvements` monthly income. `campaign_monthly_income()` now reads the actual income model's current amount.
- `construction_load()` reads `tax_income` from each entry of `common/buildings/*.txt` and settlement base income from `common/static_modifiers.txt`, covering castle, city, temple, tribal and nomad (when defined). Values are annual; monthly income is annual / 12. Original supplied static modifiers include castle **4/year**, city **12/year**, temple **8/year**, tribal **0.5/year**, nomad **0/year**.
- `holdings_read()` preserves documented `b_holding = building_key` assignments dated no later than the bookmark. The tax calculation sums completed building types while treating a later `upgrades_from` chain as a replacement, not a separate stack of older tiers. It also includes completed construction projects in the existing running-game construction ledger, deduplicating against history.
- `economy_snapshot_build()` walks *all installed province histories* at the selected bookmark, not merely the currently selected province, and counts individually owned holdings from barony title history. When a capital barony lacks an explicit holder, a county-holder fallback is **provisional** and counted in the startup log; unknown noncapital holders are excluded rather than invented.
- The campaign receives direct holding tax on monthly rollover with fractional gold carried forward. Construction expenses still use the existing unsigned integer treasury API. After a building completes, its income is included in the next monthly refresh.
- Historical character `wealth` is respected when recorded. If absent, OpenCK2 currently starts the simulation with zero gold, **not an invented CK2-compatible starting treasury**.

**Important economic limitations:** This is *gross direct holding base + authored/completed building tax*. It is **not** a full CK2 realm income calculation: vassal contracts, church taxes, tax-law obligations, province/global modifiers, prosperity, technology, stewardship tax modifiers, trade posts, occupations, nomad tributaries and procedurally initialized buildings are not resolved. Do not compare the reported direct income to original CK2's *total* character income without adjusting for those missing systems. An explicit holder's income is not the same as an independent liege's tax revenue.

##### 3. Prestige, piety, and demesne size (data-driven, partial)
- `economy_core.c` parses active `common/defines.lua` constants for `NTitle.*_TITLE_PRESTIGE`, `*_AS_VASSAL_PRESTIGE`, and `NDiplomacy.DEMESNE_*`. It also reads the monthly diplomacy->prestige and learning->piety multipliers from `common/static_modifiers.txt`.
- Personal prestige accrues for mapped personally held title ranks and the project's direct-vassal resolver, plus verified historical diplomacy when available. Personal piety accrues from base learning when that statistic is recorded. Both update on month rollover. Opening the played ruler's character panel displays the mutable campaign values rather than the frozen history profile.
- Demesne count is the number of counted personally owned holdings (including clearly reported provisional capital fallbacks). Demesne **limit** uses base + top rank + ruler/spouse stewardship + great-duke bonus from the installed defines, rounded down. Both are displayed together in the top bar.
- Starting prestige/piety come from dated historical character values when recorded; otherwise they begin at zero as provisional simulation seeds.

**Important prestige/piety/limit limitations:** Trait/event/focus bonuses, learned modifiers, church and council effects, dynasty prestige, landless sons, tribal/patrician and government special cases, title-law changes, succession/gavelkind, Jain bonus and game-rule variations are **not** evaluated. The topbar tooltip calls these partial known-rate calculations; do not claim numerical CK2 parity yet. There is no complete simulation of historical ruler changes after the bookmark date.

#### Code overview
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

#### Verification performed
- `make -B check-economy check-map-core check-governments-core check-settlement-styles check-map-positions CFLAGS='-O1 -g -Werror -fsanitize=undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=undefined'`: **PASS** for all six test executables.
- `economy_core` parser checked against the actual supplied `common/defines.lua` and `common/static_modifiers.txt`: **PASS** (`demesne_ready`, `prestige_ready`, `attributes_ready`). The supplied CK2 `KING_TITLE_PRESTIGE` was 0.8/month; `DEMESNE_MAX_SIZE_KING_MULT` was 3.
- `gcc -fsyntax-only` with a *test-only stub* for the five data/adapter units: **PASS** with `-Werror`. This is not a full SDL compile.
- `make -B game CFLAGS='-O2 -g -Werror'`: **BLOCKED** on this execution host (no `sdl2` or `SDL2_image` pkg-config files; no SDL2 development headers). User's Linux machine previously completed the game build and is the intended integration test site.
- **Not verified:** runtime HUD positions, actual income comparison in a dated original CK2 save, generated buildings, local CK2 province title histories, full native game compile.

#### Linux acceptance checklist
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

#### Prioritized TODOs
- **P0:** Native `make game` + screenshots/test logs from the player's CK2 installation. Fix any new warnings or rendering overlap before claiming runtime success.
- **P0:** Add full local asset integration fixtures for dated history provinces, barony owners and explicit `b_x = building` assignments. Confirm the real history syntax is covered; extend for nested structures if found.
- **P1:** CK2-generated initial holding buildings, technology-driven start levels and special building histories (the original C++ creates some dynamically).
- **P1:** Dynamic title/holding ownership on character death, inheritance and ongoing simulation dates, including generated barons and proper capital ownership.
- **P1:** Feudal city, temple and vassal taxes; laws/obligations; owner/government restrictions; economic modifiers; province culture/religion effects.
- **P1:** Full prestige/piety modifiers and per-event sources; gavelkind, Jain, government/game-rule demesne limit adjustments.
- **P2:** Floating-point purchase/treasury accounting, trade zone and nomad economy, tax breakdown UI, province-level culture/religion changes after bookmark.
- **C++ deferred:** No conversion required for these C data and simulation modules. A future full modifier/event evaluator or 3D settlement renderer could benefit from C++ memory/container management, but both remain viable in C and are not converted in this increment.

---

<a id="source-docs-technology-console-progress-md"></a>
## Source note: `docs/TECHNOLOGY_CONSOLE_PROGRESS.md`

> Preserved from the prior source archive. Original SHA-256: `43c6ba36a470246fac4dabd2922e24c1661c7de4fb3c94e4475147b032baeada`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2: Technology screen and developer console — C17 increment
**Baseline:** `OpenCK2_Economy_History_C.zip` (most recent complete source snapshot supplied in the conversation, before this change). Existing code was retained. No original CK2 binaries or proprietary artwork are packaged.

#### Completed changes
##### Technology- Added `src/game/technology_core.c/.h`: SDL-free parser for `common/technology.txt`. Reads technology definition identifiers grouped under `TECH_GROUP_MILITARY`, `TECH_GROUP_ECONOMY`, and `TECH_GROUP_CULTURE`, skipping nested scripted effects and level unlock blocks without evaluating them.
- The supplied CK2 `common/technology.txt` parses as **18 technologies, six in each group**. This count is about definitions, **not** any ruler's/province's technology progress.
- Added `src/game/technology_view.c/.h`: uses the original `interface/domestic_technology.gui` positions and original `interface/domestic_technology.gfx` images (`GFX_tech_bg` and category point icons). Uses the original close button sprite from `interface/generalstuff.gfx` and attempts to localize technology keys from the mounted CK2 localisation files.
- The Technology topbar button and **F5** open the modal screen. Escape or the original close button closes it. While open, map and other UI input cannot click through.
- The screen displays named technologies in their three original columns; it does **not** fabricate progress, points, levels, focus, spread, or bonuses when such values are not yet loaded.

##### Console- Added `src/game/debug_console.c/.h`: a bounded command tokenizer and output-history buffer (SDL independent). The engine never executes shell commands, Lua strings, or unknown commands.
- Added the overlay and SDL text-input dispatch in `src/game/main.c`. **Backtick** opens/closes the console in Campaign, **Enter** submits, **Backspace** edits and **Escape** closes it. While open it blocks gameplay shortcuts and clicks.
- Read-only commands: `help`, `clear`, `date`, `province`, `ruler`, `gold`, `prestige`, `piety`, and `tech [TECH_ID]`. `tech` without an ID reports the number of available definitions; with an ID it confirms whether a matching definition was parsed, not its in-game researched value.
- `mapmode terrain` and `mapmode realms` use the already-implemented map-mode selectors; other modes are explicitly rejected rather than showing fake data.
- **This is a limited OpenCK2 developer console**, not an implementation of every CK2 cheat/script/effect command. For example, `gold` reports current wealth; it does not award money.

#### Modified files- `src/game/main.c` — topbar click/F5, console keyboard/SDL input, modal drawing and event priority, cleanup.
- `Makefile` — add the modules to the C17 game sources and a portable test target.
- `src/game/technology_core.c`, `.h`; `src/game/technology_view.c`, `.h` — new.
- `src/game/debug_console.c`, `.h` — new.
- `tests/technology-console.c` — new.

#### Verification performed here- `make check-technology-console CK2_TECHNOLOGY_TEST_FILE=<supplied common/technology.txt>`: PASS.
- `make check-map-core`: PASS.
- `make check-economy`: PASS (economy core + finance holdings).
- `make check-ck2-map-data`: PASS (government definitions + settlement styles + positions).
- New technology and console core tests also passed `-fsanitize=address,undefined` with `-Wall -Wextra -Wpedantic -Werror`.
- `technology_view.c` passed syntax checking with project-local temporary SDL type stubs and `-Werror`; this is **not** a substitute for compiling against real SDL.
- Full `make -B game` **could not be completed in this environment** because it lacks SDL2/SDL2_image/SDL2_mixer development headers and pkg-config entries. Game UI appearance and SDL input handling remain unverified in the real engine.

#### Run on Linux Mint (with your CK2 installation mounted)```sh
make -B game CFLAGS='-O2 -g -Werror'
make check-technology-console check-map-core check-economy check-ck2-map-data CFLAGS='-O2 -Werror'
```

Then launch a campaign and check:
- [ ] Technology topbar button opens the screen; F5 does the same.
- [ ] Military/Economy/Culture have six correctly localized technology labels each.
- [ ] Original CK2 background/point icons load; close button and Escape work.
- [ ] Clicking behind the tech window does not change province selection or open another screen.
- [ ] Backtick opens the console, typing does not pan the map, Enter returns useful output, and backtick/Escape closes it.
- [ ] `help`, `date`, `province`, `ruler`, `gold`, `prestige`, `piety`, `tech TECH_INFANTRY`, and `mapmode terrain`/`realms` work.
- [ ] Unknown console commands are rejected without changing gameplay.
- [ ] Time advancement, right-click owner navigation, and other map modes still behave as before.

#### Remaining work (not claimed as complete)1. Parse and apply date-aware technology histories and active county technology levels (history/local province state), then display authentic progress without inventing defaults.
2. Implement technology point sources, spending/investment costs, selected focus and spread rules verified from executable/disassembly or original CK2 behavior.
3. Render original `tech_entry_level_window` strip, progression sprites, focus and map-mode buttons driven by verified actual levels; support hover descriptions of effects and unlock requirements.
4. Improve source selection and DLC/mod loading if user reports localized labels or GFX sources missing. Original window has extra conditional overlays that currently are not rendered.
5. Implement additional console actions only with verified effect semantics, argument checks and game-state mutation tests. Current console is intentionally limited and has no Ironman mode, so commands that change game state are not enabled.
6. If you need a more exact CK2 console, trace command names/argument semantics from `FreeCK2game.exe` and CK2's scripts; the existing `docs/reverse-engineering/CONSOLE-FINDINGS.md` is a research note, not proof all commands exist in OpenCK2.

#### C++ deferralNo work in this increment requires C++. For large future simulation/technology effect systems, evaluate C++ only after the existing C data contracts, serialization and tests are stable. Do **not** convert these functioning C modules just for symmetry with the CK2 original source.

---

<a id="source-notes-c-core-progress-md"></a>
## Source note: `notes/C_CORE_PROGRESS.md`

> Preserved from the prior source archive. Original SHA-256: `382c315a71702b9bf33c05f4a69d3123d79b2a44a229a717947c37cf2f626084`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 C-first implementation — Increment 1
Baseline: `OpenCK2_character_icons_extended.zip` (SHA-256 `1bdf89edea06742e52ffa095215d6cc00d09963b5561c99b57c6919f0e0face0`).

**Scope:** The first C implementation increment against the source-aware UI/map plan. It is not a complete playable CK2 implementation, and the changes have **not** been verified by launching OpenCK2 against a licensed installation. The project source remains C17. The original loose files are consumed from the user's existing CK2 installation; no CK2 assets have been packaged here.

#### Implemented / changed
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

#### Tests run
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

#### Known limitations and outstanding UI work
- Map modes other than Terrain and Realms remain deliberately disabled. Their icons appear when their GUI/GFX and texture assets load, with a not-yet-implemented tooltip. There is no simulation or data-driven coloring for Religion, Culture, de jure, etc.
- Map-mode widget **positions are parsed** using the existing `frontend_gui_rect()` function. Its handling of nested CK2 orientations and the minimap's expanded/collapsed layout still requires an actual game comparison. No fake corrected hardcoded coordinates were introduced.
- Province tooltip is a safe first functional iteration using the existing basic tooltip renderer. Original skin, localization, dynamic weather, supply/defender bonuses, modifier-key interaction menus and caching remain unresolved.
- County holder may not always equal capital holding holder in vanilla CK2. The county-holder choice is explicit, not assumed to be CK2-exact. Owners and realm state use bookmark history (`start_date`) until live state simulation exists.
- The direct-vassals algorithm uses title history's explicit `liege` key, or `parent` for baronies, and deduplicates the resulting holders. This improves the previous title-only/primary-title view but is not a proof of exact CK2 allegiance, tax, opinion or sorting semantics.
- The screenshot's reported blank HUD element was **not identified with sufficient certainty**. No arbitrary icon was deleted. Republic/Nomad/Back remain separate GUI controls requiring capability/visibility checks.
- Character screen still lacks authentic Tax/Opinion values, original list sorting and fully functional tabs. Those must not be populated with unrelated treasury or invented opinion values.
- User's original executable debug-string research identifies promising methods, but **this increment did not perform fresh disassembly**. It implements bounded UI and data functions against the available C source and loose GUI names, not unverified decompiled algorithms.

#### Next C increments (in priority order)
1. Obtain SDL development dependencies, original game data mount and the complete current repository, then build and run at consistent logical resolution; run screenshot A/B checks at the same date/province/character.
2. Implement GUI hitbox/anchor diagnostics for `main.gui` to confirm Society and all mode icons' actual positions; identify the reported blank element before hiding it.
3. Add character Vassals list sort controls, true row columns and scrollbar from the loose GUI; preserve separate unknown flags for tax and opinion.
4. Add terrain/climate/religion/culture province history facts with date-aware caching and localization. Do not map a terrain texture pixel to a terrain type without evidence.
5. Generalize province-color overlay cache for data-backed Religion/Culture/de jure/Direct-vassal modes as their data becomes reliable. Keep original GUI id mapping isolated.
6. Extend gesture tests to SDL integration and owner/top-liege fixtures including missing history and input under active modals.

See `notes/CPP_DEFERRED.md` for tasks held for a possible future C++ boundary; none of those tasks were started in this increment.

---

<a id="source-notes-test-plan-md"></a>
## Source note: `notes/TEST_PLAN.md`

> Preserved from the prior source archive. Original SHA-256: `013a0ffd67a2b98996bad63091d50201e7c6f92588228ee8265d765ed412445d`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Manual runtime acceptance checklist (once SDL + CK2 assets are available)
- [ ] `make game` builds OpenCK2 on Linux; repeat a clean compile with warnings enabled.
- [ ] Confirm eight navigation icons appear in the original order, including Society, and do not overlap the wealth/date display. Compare against same-resolution reference.
- [ ] Identify the exact widget/source corresponding to the old blank tab; verify no invisible click target remains after any future fix.
- [ ] Confirm all eighteen campaign mode sprites appear at sensible positions with tooltips. Unsupported modes stay in the current mode rather than showing fake colors.
- [ ] Click Terrain and Realms icons and press `q`/`w`; verify renderer and selected icon agree. Pan and zoom while in both modes.
- [ ] Hover a landed province with county ruler different from independent ruler; verify province name/ID, county, owner and top liege match title history.
- [ ] Stationary right-click opens county holder, not top liege. Shift-right-click opens top liege. Right drag pans without opening a character.
- [ ] Right-click on character panel, HUD buttons, map-mode icons, buildings and wonder modal must not open a background province owner.
- [ ] Right-click on water or a province with missing holder must not crash; no fabricated owner tooltip data.
- [ ] Change inspected character: show direct vassals from more than one held title, no duplicate person IDs, stable order, correct list counts/scrolling.
- [ ] Compare historical ownership against 769, 867, 1066 and 1337 bookmark data. Do not represent campaign date advancement as historical ownership updates without implementation.
- [ ] Run `make check-map-core` and remaining existing repo tests when their missing source files are restored.

---

<a id="source-notes-cpp-deferred-md"></a>
## Source note: `notes/CPP_DEFERRED.md`

> Preserved from the prior source archive. Original SHA-256: `1cd5c39a5c88f0d73cfbdc8da9ce91d6013b447ffd00c08db3d7f96675d52a0e`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Possible C++ candidates — deferred (not prerequisites)
The current implementation remains **C17**. The items below are *candidates* for C++ if their complexity warrants it, not decisions to rewrite working C code. Each has a viable C design as well; do not migrate for syntax alone.

| Later subsystem | Why C++ could be helpful | Reason to defer |
|---|---|---|
| Fully general event/decision and scripted effect evaluator | Strongly typed node/variant tree, ownership-aware containers, multi-pass validation | First verify CK2 script syntax, trigger/effect behavior, and data contracts. The current C tokenizer and loose-file reader remain valuable. |
| Long-lived character/title/war relationship graph | Encapsulation and RAII for complex dynamically linked object lifetime | Implement and compare the historical snapshot/IDs in C before designing a mutable graph. |
| Campaign save games, stable object handles and versioned serialization | Schema evolution, typed serialization and managed ownership | No reliable complete gameplay-state model exists yet; locking in a format would be premature. |
| Multi-agent AI/economy/diplomacy planning | Large composable state/action objects and polymorphic strategy evaluation | Not part of this visual/interaction milestone; needs verified simulation mechanics first. |
| Highly dynamic mod hot reload/dependency graph | C++ containers and resource lifecycle helpers can simplify invalidation | Existing mounted assets and C parser work should first be proven correct for priority and cache invalidation. |

**Stay in C now:** sprite/GUI binding, hover/hit tests, map gesture input, ownership lookup, historical-file parsing, map-mode descriptors, SDL rendering adapters, utility caches and predictable data-array algorithms. These map naturally to small C modules and testable APIs.

If a later C++ component is approved, keep a narrow `extern "C"` C API boundary and avoid a whole-project rewrite. No language change is required for any remaining task in the immediate UI/map plan.

---

<a id="source-docs-qwen-144-hour-plan-md"></a>
## Source note: `docs/QWEN-144-HOUR-PLAN.md`

> Preserved from the prior source archive. Original SHA-256: `935710b1db9f4e75974f8d420f102c3b5d8442eb39bf97658c14ac4dbb9a2211`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 local AI work plan
This is a 144-hour work budget for Qwen 3.5 9B. The 18 parts have eight
hours each. Finish the acceptance check and checkpoint early whenever
possible. An eight-hour budget does not require an uninterrupted session.

#### Start and resume
1. Open `/media/london/EFFC-FDAD/OpenCK2`. Read `AGENTS.md`, the relevant
   row below, and its linked notes. Record the current commit and
   `git status --short` before editing. Preserve changes already present.
2. Work on one part at a time. Divide it into slices of roughly 30 to
   90 minutes. At the end of each slice, record the exact files changed,
   evidence, commands run, results, and next command in a local progress
   log at `/media/london/EFFC-FDAD/OpenCK2-Qwen-progress.md`. Keep screenshots,
   installed CK2 assets, saves, and raw REA output
   outside the repository.
3. On resume, read the last checkpoint and compare it with the current
   worktree. Continue from the first unfinished acceptance check. If a
   reference or an engine dependency is missing, record the missing
   evidence and move to the next independent part. Keep the blocked part
   visible in the log.
4. At the end of each part, run its focused check, `make game`, and
   `git diff --check`. Record existing failures separately from failures
   introduced by the change. Leave the worktree available for review.

Start with `docs/UI-TODO.md` and [construction evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-construction-md).
Use the [evidence conventions](EXE_RESEARCH.md#source-docs-reverse-engineering-readme-md) for CK2
observations, binary traces, and hypotheses.

#### Work queue
| Part | Hours | Work | Acceptance check |
| --- | ---: | --- | --- |
| 01 | 8 | Capture the build baseline and repair the construction test fixture. | Existing queue tests still run. Insufficient gold and a second pending settlement leave treasury and queue unchanged. `make check-construction` passes. |
| 02 | 8 | Add focused character-history fixtures for traits, claims, and resource assignments. | The fixture proves repeated keys, dated changes, future-date exclusion, claim removal, and explicit zero versus missing values. |
| 03 | 8 | Verify claims display against Edward III in 1337 and one authored weak claim. | Record CK2 and OpenCK2 shields and dates. Fix clipping, source ordering, or strong/weak presentation where verified. |
| 04 | 8 | Verify portrait navigation and the consort heading. | Open Philippa from Edward, return to Edward, open a family member, and inspect a vassal. Player treasury and time stay unchanged. |
| 05 | 8 | Verify trait ordering and attribution. | A fixture with two bonuses and a penalty shows each named contribution and moves the education icon first. Validate signed values and unknown base handling. |
| 06 | 8 | Compare tooltip geometry with vanilla. | Wealth, Demesne, Traits, Modifiers, Claims, and Martial tooltips stay readable and within the viewport at two resolutions. |
| 07 | 8 | Research generated starting prestige and piety. | Capture king, emperor, and unlanded cases at one date with documented DLC. Locate a loose-file or binary source for any rule proposed for implementation. |
| 08 | 8 | Bind verified resource initialization. | After Part 07, reproduce one authored and one generated case. Add a focused regression check and keep missing values distinguishable. |
| 09 | 8 | Research demesne, vassal-limit, and realm-size counting. | Record the count inputs and one vanilla comparison for each. Identify where OpenCK2 lacks title, holding, law, or government state. |
| 10 | 8 | Inspect vanilla coronations and their trait effects. | Record a scripted priest, bishop, or pope path, DLC requirements, and the resulting trait. Implement only a verified, testable slice. |
| 11 | 8 | Inventory character modifiers and opinion inputs. | Trace one authored modifier with its source and duration. Record the input components of one vanilla opinion tooltip before proposing totals. |
| 12 | 8 | Decode one authored dynasty coat of arms. | Use an existing dynasty, original atlas files, and a vanilla capture. Record the palette, atlas frame, emblem, and shield-mask evidence. |
| 13 | 8 | Render the verified dynasty-arm case. | After Part 12, display the result in the CK2 shield slot. Add a regression fixture and record which atlas or religion variants remain unsupported. |
| 14 | 8 | Verify one historical bloodline family. | Trace one founder and named descendants in vanilla. Record DLC, lineage direction, and any exceptions before coding membership. |
| 15 | 8 | Add one bloodline view path. | After Part 14, use the original panel and symbols for verified memberships. Confirm an eligible and an ineligible character. |
| 16 | 8 | Compare two historical Great Works. | Verify dates, stage, damage, and completed upgrades for Hagia Sophia and another authored wonder. Add a fixture for repeated upgrade keys. |
| 17 | 8 | Verify one settlement construction cost and duration. | Compare Middlesex in vanilla and OpenCK2. Trace a specific modifier input and record the fixed-point calculation before changing the price. |
| 18 | 8 | Verify integration and prepare handoff. | Run the relevant Make checks, smoke-test launcher and campaign, and update the UI TODO with verified results, open gaps, and the next smallest task. |
| **Total** | **144** | | |

#### Checkpoint template
```text
Part / slice:
Base commit and pre-existing changes:
Status: in progress / verified / blocked
Files changed:
Evidence and comparison scenario:
Checks and results:
Remaining risks or gaps:
Next exact command or edit:
```

#### Paste into Qwen
> Work through docs/QWEN-144-HOUR-PLAN.md one part at a time. Start with the
> first unfinished part. Read AGENTS.md and preserve existing worktree
> changes. Keep a local checkpoint after each small slice so I can stop
> and resume you. Keep original CK2 files and raw captures outside the
> repository. Report verified results separately from assumptions and
> blockers. Do not push changes. Stop after the current checkpoint if I
> ask to pause.

---

<a id="source-docs-qwen-progress-review-md"></a>
## Source note: `docs/QWEN_PROGRESS_REVIEW.md`

> Preserved from the prior source archive. Original SHA-256: `0aaba892afcb17a7a6879e97923f0fbb7f96da5b491424ff07abad12b5a3024e`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Review of `OpenCK2-Qwen-progress.md`
The uploaded file is a very short local Qwen checkpoint. It refers to base commit `ce428d0`, says it deleted `tests/construction.c`'s “Regression 2” assertions (lines 68–79), and proposes rerunning `make check-construction`.

**Finding:** Both `OpenCK2.zip` and the more recent `OpenCK2_Economy_History_C.zip` still include the “Regression 2” block and its assertions. The checkpoint is therefore **not an accurate record of the supplied current source**, and is not a pending instruction to delete those checks. We have preserved the regression tests unchanged in this increment.

**Recommendation:** The file is not needed by the compiler or runtime. Archive it for provenance, but do not keep it as the primary project TODO. If the regression fails on your current Linux build, capture the exact failing assertion and fix the code or fixture intentionally. Removing tests merely to make the suite pass is not a verified bug fix.

**Verification limitation:** This environment cannot run `make check-construction` because SDL2 development packages are absent. The test must be run on your Linux computer before concluding the regression is resolved. `make check-technology-console`, `check-map-core`, `check-economy`, and `check-ck2-map-data` pass here.

---

<a id="source-tools-readme-md"></a>
## Source note: `tools/README.md`

> Preserved from the prior source archive. Original SHA-256: `2809ef23249c72eb80185d3b901b97c82a25ce0cd242aa3db191e38ec12cb72f`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 developer tools
Tools are standalone research and verification programs. Build the game and
launchers through the root [Makefile](../Makefile). Keep local reference files,
REA projects, scratch scripts, and generated comparisons in ignored storage.

#### Title history probe
[`title-history.c`](../tools/title-history.c) reports the last dated holder assignment
at or before the requested date in one installed CK2 title-history file.

```sh
make title-history
CK2="${CK2_DIR:-$HOME/.steam/steam/steamapps/common/Crusader Kings II}"
./bin/title-history "$CK2/history/titles/c_york.txt" 1066.9.15
```

The pinned reference reports `holder=5660 effective=1066.1.1`. This probe
reads a single file. For runtime history, DLC and mod overlays, title-name
resets, and hierarchy selection, see [the starting-era notes](EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md).

#### Holding audit
`make holding-audit` builds a probe that lists the history file and
active holdings for every mapped province at a requested date.

```sh
./bin/holding-audit "$CK2" 769.1.1
```

The [province and holding audit](EXE_RESEARCH.md#source-docs-reverse-engineering-province-holdings-md)
describes the columns, map-wide comparison, and duplicate-file fix.

---

---

## 2026-11-12 — HUD, heraldry and settlement visuals (C-only increment)

**Reference:** user-supplied screenshots of OpenCK2 at the 1337 bookmark versus Crusader Kings II. **Implementation language:** C17. The original `interface/*.gui`/`*.gfx` in the user's CK2 installation remains authoritative; no retail assets are embedded in this project.

### Changed source and observed problems

| Source | Change | Status / remaining difference |
|---|---|---|
| `src/game/character_view.c/.h` | Bind original `domestic_dynastic.gui` `dynastic_shield` rectangle, render a separate shield from the main character title shield; use original `character_shield` anchor with the large mask rather than force a 55×62 offset. | **Visible fallback only:** dynastic coat of arms currently uses the ruler's primary title flag when present. This is NOT a reconstructed dynamic CK2 dynasty CoA. Requires `common/dynasties` pattern/template/color/texture generator and tests before parity. |
| `src/game/era.c/.h` | Add large CK2 title-shield mask path (`shield_big_mask.tga`, pagan variant), retaining a medium-mask fallback if large texture is missing. | Title arms still depend on `gfx/flags/<title>.tga`; many CK2 titles use procedural CoAs, not a file. |
| `src/game/main.c` | Add the current ruler's actual primary title shield to the topbar using `main.gui` `topbar_ruler_shield` coordinates instead of a hardcoded York flag. Also render the selected county's title shield in `province.gui` `pv_county_shield` coordinates. Cache selected titles and free on changes. | Verified source integration, not game-rendered. Requires compare at 769/867/1066/1337. |
| `src/game/main.c` | Group the temporary Next Month and Main Menu buttons at the bottom center; remove the redundant `Realms / Terrain` toggle from `campaign_buttons`. | The existing 18 map-mode icons in the original minimap remain the normal map-mode controls. Buttons are OpenCK2 development UI, not recovered vanilla CK2 widgets. |
| `src/game/campaign_hud.c/.h` | Render the three original `topbar_extension_*` backgrounds **before** character views; retain society icon source-defined position. | Test extension composition and overlap at supported scales/resolutions; some extensions may be conditional in original CK2. |
| `src/game/campaign_hud.c/.h`, `src/game/setup_toolbar.c` | Load `alerts.gui` and `alerts.gfx`, including `alerticon_startposition`, `alerticon_offset`, and the original 69-frame `GFX_alerticons` sprite. Draw eight grayed, noninteractive placeholders; suppress map right-clicks underneath them; descriptive hover note. | **Not actual alerts**: original eligibility, ordering, priorities, and dynamic count are not implemented. Arbitrary sheet frames must not be described as gameplay alerts. |
| `src/game/main.c`, `src/game/setup_toolbar.c` | Bind the original `province.gfx` culture variants of castle, city, tribal, temple (western-faith fallback), province settlement frame, and optional settlement-owner frame. Do not recolor originals arbitrarily. | Temple graphic depends on religion graphical culture, not yet determined; original `common/religions` resolver required. This concerns the province holdings panel. |
| `src/game/main.c` | Replace zoomed map square holding thumbnails with cached **county title shields** wherever `gfx/flags/c_*.tga` exists. Use an LRU of 192 entries; keep old art if no title flag exists. | Original CK2 uses placed 3D settlement meshes, occasional heraldic icons and conditional visibility. This is a closer 2D interim visual, not full map parity. |
| `src/game/assets.c/.h` | Remove hardcoded `gfx/flags/c_york.tga` load/fallback. | Prevents a bogus York shield when playing another ruler. |

### Evidence classification

- **Verified loose-file definition:** `domestic_dynastic.gui`: `character_shield`, `dynastic_shield`, `character_crown`; `main.gui`: `topbar_ruler_shield`, `topbar_ruler_crown`, `topbar_extension_0..2`; `province.gui`: `pv_county_shield`, `settlement_entry`, `settlement_placeholder`, `settlement_owner_frame`; `province.gfx`: settlement art families and variants; `alerts.gui`: `alerticon_window`, `alerticon_startposition` and `alerticon_offset`; `alerts.gfx`: `GFX_alerticons`, 69 frames.
- **Source verified:** old character shield was clamped to main crown x and hardcoded 55×62; campaign title background used a York flag; old map markers drew generic holding-art textures; campaign action table contained an extra Realms/Terrain button.
- **Provisional choice:** title-flag proxy for dynastic arms, eight grayed alert icons, 192-entry LRU, and development-button positions. These are documented replacements until CK2 behavior is verified.
- **Not newly verified:** any executable function addresses or CK2's dynamic alert/coat-of-arms algorithms. Preserve existing `docs/EXE_RESEARCH.md` without extrapolating from filenames.

### Checks

- Passed: `make check-map-core check-ck2-map-data check-economy check-technology-console CFLAGS='-O2 -Werror'` (seven programs including map, economy, government, styles, positions, finance holdings, and technology/console).
- Passed: targeted C17 syntax checks for `campaign_hud.c`, `character_view.c`, `era.c`, `setup_toolbar.c`, and `main.c` using **temporary incomplete SDL stubs**. This catches C syntax errors but is **not** a real SDL build and does not validate actual SDL types, links or renderer behavior.
- Verified: all new sprite and widget identifiers exist in the supplied GUI/GFX files; these files are definitions only and don't include the required texture bytes.
- Not run: full `make game` with installed SDL2, SDL2_image, SDL2_mixer, zlib and mounted CK2 assets, or an in-game comparison. Linux user should repeat the build.

### Manual acceptance checklist

- [ ] `make -B game CFLAGS='-O2 -g -Werror'` completes.
- [ ] As King Edward in 1337: topbar, big title arms, character dynasty slot and left province shield are visible, correctly positioned, and do not cover the dynasty/character labels. Treat the dynasty-flag proxy as a known limitation.
- [ ] Society extension background covers its icon without drawing over the character panel or date.
- [ ] No second Realms/Terrain button appears. Terrain/Realms icons and `q`/`w` still work.
- [ ] Next Month and Main Menu are adjacent and clickable; no map click leaks through them.
- [ ] County map shields appear where title flags exist, with no massive stutter when panning and zooming; missing or generated flags gracefully use the existing fallback. Map holding name and owner hover still work.
- [ ] Province settlement art uses different graphical culture variants where the game provides them; compare a western castle and a Muslim castle at the same zoom. Temple faith-dependent variants are still incomplete.
- [ ] Eight grayed preview alerts render at the original alerts strip location and block map right-clicks. Their visual frames must not be interpreted as real conditions, and they don't support dismissal yet.
- [ ] Confirm no regression to character tabs, Vassals, time advance, government tooltip, construction or the technology screen.

### Next work (still C-first)

1. **Dynamic dynasty CoA generator.** Parse `common/dynasties` `coat_of_arms` scripts plus the original palettes, layers, and patterns. Replace the temporary title flag, with explicit fixtures for Plantagenet and several non-Western dynasties.
2. **Alert eligibility/priority.** Recover event conditions and data dependencies from CK2 executable evidence, then render only earned alerts rather than the eight placeholders.
3. **True settlement meshes.** Connect `map/positions.txt`, `settlements.gfx`, graphical-culture actors, terrain transforms, and projected sprite/mesh LOD. Current county shields are not a 3D substitute.
4. **Generated title shields.** Cover county/title/dynasty arms without prebuilt flag files and compare crown overlays and sizes.
5. **Province panel faith-specific temple art.** Resolve religion graphical culture from authored data and use `GFX_settlement_temple_<culture>_<faith>` variants rather than the documented western fallback.

## 2026-11-12: Dynasty coat-of-arms reconstruction — C17 increment

**Implementation (not 1:1 shader parity):** The character screen's `dynastic_shield` now attempts to render dynasty arms from CK2's mounted `common/dynasties/*.txt`, `interface/coat_of_arms/coats_of_arms.txt`, `interface/coats_of_arms.gfx`, and `common/religions/*.txt`. `src/game/dynasty_arms_core.c/.h` parses authored `data={...}` and expanded `template/layer` forms, graphical religion styles, texture sheets, color palettes, and the template masks. `src/game/dynasty_arms.c/.h` selects the authored atlas frames (including predefined full-color dynasty designs), samples the masks/layers, and composites a software RGBA shield into an SDL texture. `character_view.c/.h` now uses that texture for the separate dynasty shield while preserving the county/kingdom **title flag** for the title shield.

**Concrete data checks:** Original supplied CK2 files yielded 8,083 dynasty records, 1,101 explicitly authored arms (of which 126 use multiple layers), nine style groups and 17 `CoatOfArmsType` templates. Dynasty 106, Plantagenet, defines `data={0 3 0 34 0 0 0}`: template 0, texture index 3, frame 34 from the Christian style's `dynasties.tga` atlas. This is not the title flag for `k_england`. These counts apply to the supplied `00_dynasties.txt`/base interface definitions, not an exhaustive DLC/mod install.

**Important differences from original CK2:** The `gfx/FX/coa_shield.lua` shader/effect was referenced by the original `coats_of_arms.gfx` but its source and the actual `gfx/coats_of_arms` atlas pixels were not supplied in the investigation, so sampled tint weights, frame blending, mask clipping and resolution remain approximations requiring in-game screenshot comparison. Emblem sprite definitions are parsed but **emblem placement is not implemented** (87 authored layers reference nonzero emblems in the inspected file). The game's generated dynasty design RNG, palette exclusion constraints, and special cases have not been recovered. For dynasties with no authored arms, OpenCK2 uses a stable, deterministic *approximate* design selected from data-defined templates and random-allowed atlases, labeled as such in the tooltip. Missing assets retain the earlier title-flag fallback, not a silently invented coat of arms.

**Validation:** `make check-dynasty-arms` passes both synthetic fixtures and original supplied CK2 data (when `CK2_DIR` is set). AddressSanitizer and UndefinedBehaviorSanitizer passed on those parser fixtures and original file. Existing map-core, economy-core, technology-console, government-core, settlement-style and map-position tests passed. `dynasty_arms.c` and `character_view.c` passed targeted C17 syntax checks with temporary SDL type stubs; only a pre-existing unrelated missing-braces warning was observed. **No full SDL game compilation or in-game rendering comparison occurred in this environment.**

**Recommended next verification on Linux:**

1. `make -B game CFLAGS='-O2 -g -Werror'` and `make check-dynasty-arms` (optionally `CK2_DIR='/path/to/Crusader Kings II'`).
2. Open Edward III of England in 1337. Compare *both* the dynasty shield and the England title flag against the original CK2 screenshot. The dynasty icon should resolve through Plantagenet ID 106, not the `k_england` title flag.
3. Check a dynasty with explicit multi-layer arms and one with no authored CoA; compare visual details and tooltip provenance separately.
4. Verify missing atlas assets do not crash and retain fallback. Investigate genuine TGA alpha/CoA shader matching next.

The underlying code remains C17 and uses the mounted CK2 asset search path; no proprietary graphics are embedded in the ZIP.


## 2026-11-12: C17 localisation and three genealogy interfaces

### Implemented

- `src/game/localization_core.c/.h`: shared indexed localisation catalog. Reads CK2 `localisation/*.csv` from the game's mounted base/DLC/mod data through the existing asset resolver; supports English, French, German, Spanish with English fallback, semicolon CSV quoting, basic CK2 color-control stripping and replacement of a single `$VARIABLE$`. No CK2 data files are overwritten.
- `src/game/ck2_script.c`: `ck2_translate()` now uses the shared catalog rather than opening/searching every CSV for each label. `ck2_localize`, `ck2_locale_reload`, and `ck2_locale_format` support new localized controls.
- `src/game/settings.c`: Language row is interactive; changes apply on **Apply**, persist in the user's SDL preference directory and reload selected language. Settings categories, option labels with confirmed CK2 keys, and some character stat tooltips are translated. Previously cached menu/tree views are discarded on change.
- `src/game/tree_history.c/.h`: date-aware source file parser for character-history dynasty, parentage, birth/death and name.
- `src/game/tree_view.c/.h`: modal Dynasty Tree, Family Tree and Realm Tree, launched from character-screen shield/name and existing tree buttons, with clickable people and Escape/close navigation. Dynasty uses CK2 coat of arms, authored character history and localized house/member labels; Family uses known parents, grandparents, children and spouse; Realm uses direct vassals from title history. Graphs have a capped visible node count and do not invent unknown links.
- `src/game/setup_toolbar.c` includes `interface/dynasty_tree.gfx` in sprite lookup. `Makefile` builds the new modules.

### Tested (standalone C source tests only)

`make check-localization-core check-tree-history check-dynasty-arms check-technology-console check-map-core check-economy check-governments-core check-map-positions check-settlement-styles CFLAGS='-O2 -Werror'` — passing here; new tests also ran with address and undefined-behavior sanitizers. `tests/localization-core.c` includes original CSV load, language switching and fallback; `tests/tree-history.c` covers synthetic dated history. Changed SDL-dependent sources received syntax checks with temporary SDL header stubs. **Not tested** with real SDL development headers, a full game binary, or CK2 screenshot comparison in this sandbox.

### Known limitations / next priorities

1. The localisation pass is **not complete**. Several custom gameplay/debug messages, tooltips and development controls still use English prose because the original game has no direct localization key for that custom behavior, or the corresponding key has not been mapped. Audit per-screen and map appropriate original CSV keys; do not falsely localize a string by assigning an unrelated CK2 key.
2. Settings currently supports four language columns, not every CK2-translated language. Glyph rendering is based on the existing byte-oriented font atlas; accented glyphs, text length and encoding need native Linux/Windows/macOS visual checks. CK2 formatting tokens beyond section-sign colors and a single `$TOKEN$` are not fully evaluated.
3. Genealogy windows are **functional first-pass graphs**, not an exact reproduction of CK2's branch collapsing, dynamic layout, search, filters, full dynasty prestige calculation, scroll/zoom, or random/modified genealogy data. Each tree is bounded to 28 visible nodes; historical counts may include records with incomplete birth/death metadata.
4. Realm genealogy reflects loaded historical title relationships, not a live changing campaign-state model. The Family graph shows only known relations. Tree backgrounds are resolved from original `dynasty_tree.gui/.gfx`, but runtime visual parity is still unverified.
5. Retest language changes when opening/closing game screens and on restarted launches. Test character with recorded family, dynasty 106 and Palaiologos, different bookmark dates, and multi-tier realm vassals. Confirm no stale localization pointers after Apply.

### Suggested local validation

```sh
make -B game CFLAGS='-O2 -g -Werror'
make check-localization-core check-tree-history check-dynasty-arms check-map-core CFLAGS='-O2 -Werror'
```
Open Settings > Game > Language, select French/German/Spanish, Apply, and reopen Settings; restart to verify persistence. Open a landed character: click family tree icon, realm tree icon, and dynasty shield/name; click a profile and return to the character screen. Compare CK2 tree screenshots at the same date/character and record differences.

---

## C-only genealogy layout increment — 12 November 2026

Baseline: `OpenCK2_Localization_Trees_C.zip` (the latest project ZIP supplied in this conversation at the start of this increment). The game logic and asset formats remain C17; no C++ conversion was made.

### Verified source issues corrected

- `tree_view.c` previously discarded every node with a row number >= 6, silently hiding many direct vassals, children, and other relatives. Its fixed 28-node budget and non-scrollable layout also made large historical groups inaccessible.
- The old dynasty tree rendered an arbitrary sample of same-dynasty people as a disconnected peer list; its ancestry lines did not reliably follow the recorded parent IDs. Family's loaded sibling data was not displayed, and spouse was treated like a parent edge.
- The modal previously rendered every node directly over a non-clipped window and drew the same generic dark card regardless of the original CK2 tree-member artwork.

### Implementation

- Added `src/game/tree_layout_core.c/.h` (SDL-independent C layout and clamped scrolling), with `tests/tree-layout.c` and a `make check-tree-layout` target.
- Increased the per-view bounded node budget to 64. Nodes beyond a scrollable viewport are reachable by mouse wheel / Up/Down; Shift+wheel or Left/Right scrolls horizontally; Home resets the position. This is *not* a claim that all dynasty members are loaded or simulated.
- Rebuilt family ancestry branches from the actual loaded father/mother/grandparent IDs and included siblings; spouse now uses a separate relationship link. Dynasty branches include only recorded same-dynasty parents/grandparents, children, siblings, and grandchildren. Unrelated members are no longer displayed as though they belonged to the focused genealogy.
- Extended realm tree to show up to 18 direct vassals and bounded second-level relationships; the `total` count may exceed the visible subset. The title data continues to come from `realm_direct_vassals()`.
- Draws relationship connectors as orthogonal branches, clips the graph inside its window, and binds original `GFX_DynaTree_Member` / `GFX_realmtree_title_bg` if installed. A flat rectangle is retained as a missing-asset fallback. The root's border is distinguished. The original GUI background, close control, banner and dynasty shield remain in use.
- Preserved the existing localised tree headings and status labels; no original game text was replaced with new English-only labels.

### Validation

- `make check-tree-layout check-tree-history check-localization-core check-map-core check-technology-console check-dynasty-arms check-economy check-ck2-map-data CFLAGS='-O2 -Wall -Wextra -Wpedantic -Werror'` passed in the build container.
- AddressSanitizer/UndefinedBehaviorSanitizer checks passed for the standalone tree-layout and tree-history tests.
- `tree_view.c` passed a `-std=c17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only` check using a *temporary SDL stub header*. There was no full SDL game build or live visual comparison in this environment; the stub does not prove full API/link compatibility.

### Manual acceptance checklist for Linux Mint

- Run `make -B game CFLAGS='-O2 -g -Werror'` and `make check-tree-layout check-tree-history` from the extracted project directory.
- In 1066/1337, open Family Tree on a character with many siblings/children. Verify the family nodes are accessible by scrolling and that parents and grandparents connect only to recorded relatives.
- Open Realm Tree on a large ruler (e.g. a Byzantine emperor); scroll down through direct vassals past row 6 and select a lower row. Verify character navigation and Escape/close still work.
- Open Dynasty Tree on a Palaiologos, compare the background, coat of arms, names, edges and portraits to CK2. Check language switching still affects the existing localised headings.
- Confirm the original member-card textures are present in the mounted game data; if absent, the flat fallback should render and scrolling must still work.

### Known limitations and deferred work

- These are bounded *historical* graphs, not full CK2 real-time genealogy, dynasty renown/prestige calculation, blood relationships, adoptees, branch expansion, drag-to-pan, zoom, filtering, founder/head selection, or a verified 1:1 original layout algorithm.
- Some relationships in character history are absent or inconsistent, and the original game may generate additional characters/events during play. The UI displays only the facts loaded in OpenCK2; it must not invent missing parentage.
- Family nodes and the realm tree still use an approximate placement algorithm, although geometry and scroll behavior are now centralized and independently tested. The original executable's GUI cross-references are recorded in `docs/EXE_RESEARCH.md`; those references do **not** establish exact original branch-layout code.
- A reusable, simulation-wide live genealogical graph and indexed historical data store may eventually merit C++ containers/ownership, but the current incremental rendering and bounded history queries are deliberately kept in C.

### Title prestige and combat modifiers — C increment (2026-11-12)

- Added `title_prestige.c/.h`: a rank-validated, testable ledger of held and direct-vassal title contributions. Rates are supplied by the original `defines.lua` loader; no fixed per-rank values are embedded in this module. `economy_snapshot.title_prestige` exposes rank counts and their monthly contributions for later UI tooltips. This is **not** full CK2 title gameplay: no succession, grant/revoke, de jure drift, claims, live liege transfers or monthly recalculation after ownership changes.
- Character view now sums `combat_rating` from loaded `common/traits/*.txt` and displays the *known trait modifier* with a sign. This is **not total personal combat skill**: base values and scripted/temporary modifiers remain unimplemented. The placeholder is retained if no combat-rating data is found.
- The old GCC `-Wmissing-braces` initializer in `character_view.c` is corrected.
- `make check-title-prestige`, `make check-economy-construction` and `make check-economy` passed in the isolated environment. SDL headers are unavailable here, so full game build and UI appearance are not verified.
- **Next priorities:** proper character modifier aggregation (learning, diplomacy, piety and combat), piety from traits/buildings/religious modifiers, tooltip breakdown using localisation keys, live title ownership mutation and monthly ledger refresh. The original executable's `title.cpp` path reference and CK2 screenshots provide leads, **not recovered source code or proof of original formulas**.

## Historical province-panel notes (merged from prior increment)



- Province panel is only rendered when a province is selected. The map, ruler HUD, minimap, and map-mode controls remain visible when it is closed.
- The close control is read from `interface/province.gui` (`province/button_close`), using `GFX_main_close_button` from `interface/generalstuff.gfx` where available.
- Escape closes the province panel after higher-priority modal views have been dismissed. Clicking an unowned sea tile also clears the selection.
- Clearing the selected province resets the holdings snapshot and highlighted province, preventing stale data from leaking into the next selection.
- The holding renderer now tolerates missing culture-specific settlement artwork without dereferencing a null texture. It preserves the original GUI frame and its original `capital_entry`/`settlement_entry` offsets.

### Original forward-looking backlog (some items addressed above)

- Original holding owner/controller/realm heraldic shields for each barony; these are separate widgets in `province.gui` and require barony-level title/holder data.
- Correct original province-panel text columns and icon data (levies, garrison, income, supply, revolts) instead of the current development text overlay.
- Religion-specific temple artwork and precise per-holding culture selection (the current renderer uses province graphical culture).
- The original GUI panel extensions, settlement progress bars and building level display.
- Full graphical comparison at the same resolution, selection and bookmark; no SDL build is available in this environment.

### Original Linux validation

```sh
make -B game CFLAGS='-O2 -g -Werror'
make check-map-core check-economy-construction
```

1. Select a province; confirm its holdings panel opens.
2. Click its close button; the map should remain visible without the province panel.
3. Select another province; the new holdings and culture/religion must load.
4. Press Escape once with the province panel open; it should close without leaving the campaign.
5. Click the sea and confirm that the panel disappears.
6. Verify that the empty slots retain their original frames and clicking an occupied slot still opens construction.

## C17 increment: political borders, marriage and runtime children (12 Nov 2026)

**Scope:** This is a bounded foundation, not a claim that the original CK2 diplomacy,
family simulation, or map shaders have been recovered. The files under `common/`,
`history/`, and `interface/` remain external installed game data; OpenCK2 does not
modify them.

### Political map

- `src/game/political_map_core.c/.h` classifies **independent realm boundaries by
  independent-title identity**, not displayed RGB (different realms can have
  identical authored colors). It distinguishes county-internal boundaries.
- `src/game/realms.c` uses that classification to outline the base political
  map. This is an approximate CPU raster, not the original Clausewitz shader.
- `realm_direct_vassal_group()` follows authored de-facto lieges and higher
  personally held titles for the Direct Vassals overlay. Missing historical
  chains return unknown rather than fabricated political membership.
- The map is still a **bookmark-era snapshot**. Marriages do not alter title
  history or political borders without succession/title-transfer mechanics.

### Family simulation

- `src/game/family_gameplay.c/.h` holds mutable spouse links, marriage types,
  due dates and newly born characters separately from historical profiles.
- Eligibility for normal and matrilineal marriages checks known ages,
  deceased status, existing spouse relationships and immediate kinship;
  CK2's complete script/government/religion constraints and AI acceptance are
  not yet evaluated. Original character history remains read-only.
- Minimum male/female marriage age, maximum childbearing age and post-birth
  waiting period are read from installed `common/defines.lua`; absent rules
  disable the corresponding actions rather than fabricating values.
- `conceive` is **opt-in developer functionality**, with a provisional
  280-day pregnancy duration; it does not represent the original CK2 fertility
  algorithm, automatic conceptions, stillbirths or genetic inheritance.
- New children receive runtime IDs and parental links. Normal marriage uses
  paternal dynasty, `marrymatri` maternal dynasty. Cultural names are chosen
  deterministically from original `common/cultures/*.txt` `male_names` and
  `female_names` lists (`src/game/family_names_core.c`). Name weighting and
  historical naming events are not yet matched to the executable.
- The character view can merge runtime children into child portrait slots.
  The historical Dynasty/Family tree data parser, succession mechanics, and
  persistent saves do **not yet** include generated people.
- Both landed and unlanded marriage candidates can be retrieved by ID from
  the mounted character-history source. Unknown candidates are rejected.

### Developer console actions

- `family` — displays the playable ruler's spouse ID and created-child count.
- `marry <character-id>` — attempt a normal marriage. Uses actual history
  profiles and refuses known invalid relationships.
- `marrymatri <character-id>` — attempt a matrilineal marriage.
- `conceive` — explicitly schedule a pregnancy for the playable ruler and
  recorded spouse (importing a one-sided authored marriage when possible).
- `divorce` — dissolve an **active runtime** marriage. Historical CK2 divorce
  events and marriages are not rewritten.
- Advance the campaign calendar and inspect your ruler's Family tab after
  a due date. Pregnancy and newborn console messages are informational only.

### Tests and Linux verification

```sh
make -B game CFLAGS='-O2 -g -Werror'
make check-political-core check-family-gameplay check-family-names \
     check-map-core check-map-themes-core check-tree-history
```

New C unit tests passed under `-Wall -Wextra -Wpedantic -Werror` and the
Address/UndefinedBehavior sanitizers. Tests of the original culture-name file
also passed. Changed SDL-dependent translation units passed **syntax-only**
checks with temporary stubs; **a real linked SDL build and visual verification
were not possible in this environment**. On Linux, check political borders at
zoom changes, direct vassal categories, historical marriages, console messages,
births after campaign time advances and generated names/children in Family.

**Open items:** accurate political map texture/shader, naval/coast boundaries,
user-selected liege political modes, all title-history mutations, CK2 fertility
rates/traits/dynasty prestige, betrothal/concubinage, extended kinship checks,
inheritance, succession, spouse/candidate UI, runtime genealogy integration,
save/load, and AI family behavior. None is claimed complete here.

## Console-fired CK2 events (C17, 12 November 2026)

Implemented `event <id> [target id]` with on-demand lookup in the mounted
`events/*.txt` files.  It accepts numeric IDs (for example `900`) and
namespaced IDs (for example `WoL.1`).  Script text is read using the existing
mounted CK2/DLC/mod content lookup.  The game does **not** bundle the event
files; a valid CK2 installation with `events/` is required.

- `event 900` fires a matched event for the playable ruler if it is a
  `character_event`, or the currently selected province if it is a
  `province_event`.  `event <id> <province id>` also targets a different
  known province.  Character IDs other than the current player are rejected
  until their runtime stat/effect state exists.
- The event modal displays localized title/description/option keys using the
  already loaded CK2 localization catalog.  Click an option, press a number
  key 1–8, or open the console and use `eventchoose <number>`.  Escape or
  `eventclose` dismisses the modal without applying option effects.
- **Developer force-fire semantics:** event triggers, scheduling/MTTH, scoped
  descriptions and option eligibility are *not evaluated*.  This is an
  intentional partial implementation, not a recovered CK2 event evaluator.
- Flat numeric `wealth`, `prestige`, and `piety` option effects update the
  playable character's campaign resources.  Flat `religion` and `culture`
  effects are validated using the existing conversion catalog and written
  to mutable campaign overrides for the correct character/province scope.
  Direct numeric effects in province scope are skipped because their intended
  character target cannot safely be inferred.
- Other scripted effects (`if`, `random`, `ROOT`, `FROM`, character/province
  scope switches, delayed `character_event`/`province_event` chains, etc.)
  are **not** executed.  The console reports skipped unsupported effects and
  any unexecuted `immediate` block.  A capped maximum of eight options is
  displayed, with a warning for additional options.
- The initial modal uses a native OpenCK2 frame and CK2 localization; it is
  **not** CK2's original event shader, picture renderer or exact window skin.
- In absence of the user's actual game event files, tests use small synthetic
  scripts.  `make check-events` and `make check-technology-console` test the
  SDL-independent parsers and console commands; on-device SDL rendering and
  execution of real DLC/mod event scripts remain to be verified.

Follow-up: record active event scope/targets, implement CK2's trigger/effect
interpreter and persistent event state, render authored event pictures, and
support full option counts and scripted chains.  Prefer incremental verified
behavior over inventing event results.

## 2026-11-12 — Freestyle C17 increment: Bloodlines, Great Works, Next Song

Source baseline: `OpenCK2_Character_Warfare_C.zip`. All three features use the
original CK2 mounted loose assets; original game files are not modified or
shipped. The new changes are integrated into the campaign renderer and input
handlers, with cleanup and bounded SDL-free regression tests.

- **Bloodlines browser:** `src/game/bloodline_view.c/.h` uses the original
  `interface/bloodlines.gui` positions and `interface/bloodlines.gfx` symbols.
  Character-screen `button_bloodlines` now opens a scrollable six-entry browser
  of definitions from `common/bloodlines`. The button no longer does nothing.
  This is **a definitions catalog, not a membership screen**. Historical
  bloodline founders, membership and inherited modifiers require real game-state
  reconstruction; the browser does not fake them.
- **Great Works:** The existing `wonder_view` uses real dated province history.
  The character-screen Great Works button can open the first recorded built work,
  and the original `next_wonder_button` / `previous_wonder_button` move between
  historical built works at the selected bookmark date. The index is built once
  on first use and resets for a new campaign. It does not include future or
  player-built wonders, nor does it modify construction/gameplay rules.
- **Next Song:** The original `main/menu_area/minimap_nextsong` GUI widget
  and `GFX_minimap_nextsong` sprite are bound in `campaign_hud`. Clicking it
  skips to the next available `music/*.ogg`, `.mp3` or `.flac` file found via
  the mounted asset system. Missing or unsupported audio is ignored safely;
  the current track is kept if a candidate cannot be loaded. Playlist names
  are freed with the music subsystem.
- **Modal correctness:** Bloodlines and Great Works capture input while open;
  clicks no longer activate the map behind the windows. ESC closes the views.
- **SDL-free tests:** `make check-music-playlist check-bloodline-browser
  check-wonder-navigation`. `make check-bloodlines-core` parsed **247** authored bloodline definitions from the
  four supplied source files and passed AddressSanitizer/UBSan. Existing campaign-core regressions also remain
  part of the test plan. Full SDL compilation and in-game appearance need
  confirmation on the user's CK2 installation.

Outstanding work: recover historical bloodline instantiation and membership,
add founder portraits and living-member browsing, support Great Work construction
state/upgrade effects, implement context-sensitive music playlist rules, and
compare the actual modal sizes against CK2 at the same display resolution.

## 2026-11-13 — Core development: native OpenCK2 campaign checkpoints (partial)

Source baseline: `OpenCK2_Byzantine_Election_Scripts_C.zip`.  This increment
adds a **versioned OpenCK2-native checkpoint** for the mutable systems that
already exist, not a CK2 save parser or a claim of CK2 save interoperability.
The implementation is in `src/game/checkpoint_core.c/.h`, with stable-ID
export/restore interfaces in `construction.c` and `realms.c`.

**Developer-console usage (in an active campaign):**

```
savegame
savegame byz_test
loadgame byz_test
loadgame
```

`quicksave` and `quickload` are aliases for the corresponding `quick` slot.
Names must be 1–32 ASCII letters/digits, underscores or hyphens; arbitrary
paths are rejected. The checkpoint is saved under SDL's user preference folder
for `OpenCK2/OpenCK2/`, with an `.ocks` extension. `savegame` overwrites a
named slot via a neighbor `.tmp` file and `rename` **after** a successful
write. On Linux/Unix filesystems this prevents partial saves from replacing
an existing valid file, but persistence after sudden power loss and Windows
rename semantics still require verification; no fsync guarantee is claimed.

Version 1 checkpoint stores the start-bookmark date and original ruler ID,
current playable ruler ID and home province, campaign date/resources and
rates, runtime family changes (including births/deaths/pregnancies/traits),
religion/culture overrides and missions, wars, armies/paths and occupations,
levy-raising source records, selected laws, building projects and new
settlements, and title-owner changes since the initial bookmark. Names and
keys are length-prefixed, all primitive values have explicit encoding, limits
bound memory/file usage, and an FNV-1a-64 trailer detects accidental damage.
This checksum is **not cryptographic** and does not authenticate saves.

Restoring is limited to an **already-open campaign from the same bookmark and
starting ruler**. The loader rejects invalid/corrupted/unrecognized versions,
unknown law/building IDs, unknown titles/heirs, missing provinces, unsupported
conversion keys and invalid marching paths. It decodes state into temporary
allocations before touching gameplay state. The current game is paused after
loading. The main-menu `Load Game` entry is not connected to this new format.
To load after restarting OpenCK2, start a new campaign at the **same bookmark
and with the same ruler** and then use the console `loadgame [slot]` command.

**Known limitations:** this is an intentionally partial *developer snapshot*.
It does not serialize unhandled event windows, dynamically generated event
chains, UI state, mod content hashes, CK2 native saves, CK2 Ironman, the full
historical world, government voting offices, or extra gameplay systems that
remain unimplemented. The current economic snapshot is recalculated from the
restored owner/building/law state and the mounted CK2 history. A failed
memory allocation during the late live title/ruler restoration phase may
still require restarting the campaign (reported explicitly). Restore under
changed mods is not guaranteed just because the known identifiers still
exist. No CK2 reference executable comparison was performed.

**Tests:** `make check-checkpoint-core check-construction-checkpoint
check-realm-inheritance check-technology-console CFLAGS='-O2 -g -Werror'`.
The new checkpoint round-trip covers all persisted component categories and
checks truncation, tampering, unsupported version handling, and preservation
of an existing save when a new write fails. Construction tests explicitly
reorder the CK2 building catalog and reject mismatched keys. Title tests
check deltas and restoration against bookmark owners. AddressSanitizer and
UndefinedBehaviorSanitizer were run on the checkpoint codec. Full SDL game
compilation, actual in-game save/load, and save/reload across process
restarts must be checked on Linux Mint with the user's CK2 installation.

**Next:** a controlled headless campaign-world state with explicit owner and
reference lifetimes, a complete content/mod fingerprint, fully transactional
restore, a main-menu Load Game workflow, and CK2 save-format research. Treat
these as separate milestones; don't equate `*.ocks` with `*.ck2`.

## 2026-11-13 — Government types: nomads, republics, theocracies (partial)

Baseline: `OpenCK2_Core_Checkpoints_C.zip`. The government loader already read
original `common/governments/*.txt`; this increment uses more of those
rules in C17, but is **not** the full CK2 government script engine.

- Province history already contains dated holdings. `map_settlements_build`
  now records **every independently verified barony holder** and its holding
  type (not merely the visible capital). No inferred county-holder fallback
  is treated as proof of personal holding ownership. The original character
  `government` history key wins over all derived classifications.
- At bookmark start, directly held NOMAD, CITY and TEMPLE holdings can provide
  provisional `nomadic_government`, `republic_government` and
  `theocracy_government` assignments, respectively. For a mixed personal
  demesne, prefer a verified primary barony, then an unambiguous primary-title
  capital; otherwise refuse to classify. This is **not** evaluation of CK2's
  whole `potential` expression, and the UI/console identifies the inference.
  A city does **not** prove merchant-republic government: `is_patrician`,
  `higher_real_tier_than`, port access, and previous lieges are independent
  requirements. An explicitly authored merchant republic type is preserved.
- Religion-group membership and male/female temple holder permissions are
  read from `common/religions` in a bounded group index. Unknown religion,
  Muslim religion, and disallowed holder sex prevent *provisional ordinary*
  theocracy assignment. Original government-religion contradictions or
  missing titles are not silently reconciled.
- `common/governments` now also parses `no_vassal_tax`,
  `vassal_call_to_arms`, `forced_contract`, `landed_inheritance`,
  `have_gender_laws`, and several authored construction/creation permissions.
  The first consumer of `no_vassal_tax` excludes nomad->nomad tax transfers
  in the existing **partial** direct-vassal tax model. The nominee's real
  religion-tax, opinion and obligation effects remain unresolved.
- A government with authored `can_build_holdings=no` cannot found settlements
  through the existing build window. Merchant republics and theocracies are
  prevented from using OpenCK2's **ordinary hereditary player succession**;
  their election/appointment systems remain missing and titles stay unchanged
  on a death rather than being inherited incorrectly. Explicit prohibition of
  gender laws is enforced for developer law selection, as are unsupported
  succession-law overrides for merchant/temple rulers. There is **no** nominal
  substitute election or clerical appointment.
- `government [landed character ID]` reports assigned type, provenance,
  preferred holding mask, construction permission, hereditary-policy status,
  gender-law permission and government-rule tax-exemption count. The
  Governments map overlay uses authored CK2 colors for resolved assignments.

**Verified scope:** `make check-nomad-republic-theocracy check-governments-core
check-byzantine-government`, optionally with paths to CK2's original
`common/governments/*.txt` in `CK2_GOVERNMENT_TEST_FILES` and original
`00_religions.txt` in `CK2_RELIGION_TEST_FILE`. The supplied files load 12
original government definitions and 55 original religion-group entries.
Synthetic tests exercise ownership ambiguity, ordinary versus merchant
republics, explicit government rules and religious temple restrictions.
SDL-independent suites passed with `-Werror`. Full SDL build, government
portraits, faction behavior and comparison against CK2 running at the same
bookmark are still needed.

**Remaining for 1:1:** nomadic population/manpower, clans and clan vassal
contracts, nomadic holding rules and faction mechanics; merchant trade posts,
family palaces, patrician houses and elections, coastal/port conditions,
republic councils and city tax rights; clerical appointments, papal and temple
succession, investiture, bishops' tithe and investiture; full potential
triggers and DLC/mod precedence; reclassification after conversion, law or
capital changes; AI, user interaction and save/load integration for all new
systems. Build these from original scripts and reference scenarios, not
hardcoded generic ratios.


## 2026-11-13 — Nomadic clans, merchant republics and clerical appointment core (partial)

**Baseline:** `OpenCK2_Nomadic_Republic_Theocracy_C.zip`. Source stays C17.
The new SDL-independent `government_mechanics_core.c/.h` reads numerical
constants from `common/defines.lua` and trade-post building definitions from
`common/buildings/00_tradepost.txt` / `01_tradepost.txt`. No CK2 Lua is executed.
Unknown or missing required numerical definitions disable those actions rather
than using substituted values. Game and mod files remain read-only.

**Nomads:** `clanregister <ruler_id> <verified_owned_counties> <population_capacity>`
requires a recognized nomadic ruler and verified domain. `clans` inspects
stored population, capacity and manpower; `clansplit <head> <new_head>
<county_count> <population>` creates another runtime clan after capacity and
population validation, but **does not transfer real county titles**. Clan
population and manpower update on a provisional monthly tick using sourced
`NNomad` multipliers. Manually supplied capacity is NOT automatically derived
from the CK2 map, empty holdings or clan land demands. A modeled population
component contributes to the ruler's tax estimate where known.

**Merchant republics:** `patricianadd <ruler_id>` requires a verified merchant
republic government record. `patricians`, `campaignfund <gold>` and
`dogeforecast` maintain family fund balances and report the **base Respect
subtotal** from authored age, prestige and campaign-fund factors, when the
inputs are known. It does not run the complete CK2 election, any random
factor, family palaces, or a merchant republic AI. No automatically elected
Doge is claimed or transferred.

**Trade posts:** `tradeposts` reports projects. `tradebuild <province_id>
<tr_port_N|tr_enclave_N>` starts a source-defined project for the player's
registered patrician family when the province has an **authored port anchor**
in original `map/positions.txt`, the building exists, the CK2 upgrade chain is
satisfied and treasury covers its CK2 cost. Original `build_time`, `gold_cost`,
`tradevalue` and `tax_income` are parsed separately: only authored annual tax
is folded into monthly cash flow (`annual / 12`), not trade value. Build
prerequisites beyond the implemented upgrade chain, including `potential`,
`trigger`, coastal checks beyond the port-anchor proxy, dynamic distances,
Saharan trade routes and trade zones, are not yet evaluated. Do not present
this as universal CK2 trade-post construction eligibility.

**Theocracy:** `bishopnominate <b_title> <character_id>` checks a verified
player-controlled temple barony, known living adult candidate and original
religious temple-holder restrictions. It requires selected
`investiture_law_0` (Papal Investiture) or `investiture_law_1` (Free
Investiture) from the existing law state. `bishoplist` shows queued
nominees. `bishopapprove <b_title>` is a **developer-only explicit approval**:
when the current bishop is dead or absent and the title transfer succeeds, it
marks approval and transfers the temple title. It is not actual papal consent
or automatic bishops' succession. Failed transfer retains the nomination for
another attempt.

**Persistence:** checkpoint wire format v2 writes and reads bounded clan,
family, trade-post and nomination records. V1 records remain readable with an
empty new-government state. Reload rejects missing building definitions,
unknown title/ruler relationships and malformed state before ordinary restore.
This is an OpenCK2 developer save (`*.ocks`), NOT a CK2 native save; the
existing late-stage restore limitation still applies.

**Verification:**

```sh
make check-government-mechanics check-checkpoint-core check-map-positions \
    check-nomad-republic-theocracy CFLAGS='-O2 -g -Werror'
make check-government-mechanics \
    CK2_DEFINES_TEST_FILE=/path/to/CK2/common/defines.lua \
    CK2_TRADE_POST_TEST_FILE=/path/to/CK2/common/buildings/00_tradepost.txt
```

Original script loading, SDL-independent regression tests and sanitizers pass.
The complete SDL build and actual UI/console behavior have not yet been
verified in this environment. The next reference probes should compare one
known nomad-clan growth tick, a real patrician Doge election after campaign
funding, the port potential conditions at a coastal trade post, and a vacant
bishop appointment under both investiture laws in matching CK2 bookmarks.
