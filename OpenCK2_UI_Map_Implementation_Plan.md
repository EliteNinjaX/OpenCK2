# OpenCK2: province interaction, HUD layout, Vassals and map modes

Planning baseline: 12 November 2026. **Planning and investigation only; no project code has been modified.**

## 1. Baseline and evidence rules

The newest OpenCK2 source archive returned by the available file search is **OpenCK2_character_icons_extended.zip**, created 12 November 2026 at 05:24 UTC (11 November, 23:24 America/Chicago). This plan uses its actual contents, not the earlier source snapshot. It contains 56 files, including `Makefile`, `src/game/bloodline.c/.h`, and the extended character icon bindings. The archive's included `ICON_FIX_REPORT.md` still describes an earlier ten-icon pass; the actual source has sixteen extra bindings and takes precedence over that report.

Original assets examined: `interface clone.zip`, extracted under `interface/`. These are GUI/GFX definitions, not a complete runtime texture or gameplay-data installation. Original screenshot examined: `image(20261112-060301).png`, showing the Vassals tab and Nordgau tooltip on 1 January 1337. It is a reference screenshot, not a runtime verification of this source archive.

Original binary examined: `FreeCK2game.exe`, 64-bit PE, preferred image base `0x140000000`. The executable was not launched. Earlier bloodline/government research is background evidence; fresh GUI/tooltip cross-reference work below is more directly relevant.

| Input | SHA-256 |
|---|---|
| OpenCK2_character_icons_extended.zip | `1bdf89edea06742e52ffa095215d6cc00d09963b5561c99b57c6919f0e0face0` |
| interface clone.zip | `bac2f67332ae91bcca729d05ffedbe9a761e97565fe5195ea92055612c7f412b` |
| FreeCK2game.exe | `a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d` |

Evidence labels throughout:

- **[S] Verified source:** directly observed in the latest C source.
- **[L] Verified loose-file definition:** widget, sprite, layout or localization identifier present in the original interface archive. This does not prove a runtime visibility rule or algorithm.
- **[B] Verified binary observation:** instruction/string cross-reference or PE function boundary. Descriptive function names are inferred, not recovered symbols.
- **[V] Reference image:** visible behavior/text in the supplied original screenshot.
- **[P] Proposed implementation:** an engineering choice for OpenCK2, not a recovered CK2 rule.
- **[U] Unknown:** requires actual gameplay files, original-game comparison, or additional disassembly.

Confidence is high for directly inspected identifiers and source control flow; medium for inferred subsystem roles; low for exact original algorithms that have not been traced. “Available” means available to this investigation, not necessarily the latest working copy on the user's PC.

## 2. Main findings and scope

1. **Right-click navigation is blocked by existing drag handling. [S]** `handle_event()` records both middle and right button down as map drags, then consumes the matching button release before any navigation logic. A stationary right-click currently never reaches an owner-opening path.
2. **The county holder and independent ruler are already separate concepts. [S]** `realm_county_info(..., false, ...)` resolves the county; the `true` variant resolves the independent realm. `realm_province_ruler()` uses the latter. Using that function blindly would open the emperor instead of a count in a vassal province.
3. **A basic province tooltip already exists, but only in setup. [S]** `setup_tooltip()` resolves county holder and independent realm. Campaign drawing has HUD/character tooltips, not the requested province tooltip.
4. **Society is a top HUD icon, not one of the six character-detail tabs. [S,L]** The current HUD draws eight raw textures in a fixed loop. The GUI defines Society next to Religion. The source already assigns it the expected basic x-slot, so the reported visible separation is not yet explained by an obvious extra index. Background extensions, sprite cropping and actual runtime asset variants must be checked before shifting it.
5. **The “blank tab left of wealth” is not conclusively identified. [U]** There is no separate republic/nomad entry in the current eight-icon draw loop. The original GUI defines optional Republic/Nomad controls, a Back control and decorative extension pieces. They are diagnostic candidates, not a verified identification of the unwanted element.
6. **Vassal selection and wheel scrolling already exist. [S]** The list is incomplete, not absent: it shows a limited set of portraits/shields/combined labels. It lacks original sort controls, tax/opinion columns and visible scrollbar binding. Its data query is title-based and can omit or duplicate character vassals.
7. **There are 18 explicit campaign map-mode buttons in `main.gui`. [L]** This is different from the setup screen's smaller set. Only setup Terrain/Realms currently switch behavior. Campaign mode buttons and a shared mode state remain to be built.
8. **Latest archive is not a cumulative merge of every earlier patch. [S]** It contains `bloodline.c`, but not the earlier `definitions.c/.h` catalog or an explicit government key in `ruler_profile`. Do not design the government map mode as if those earlier changes are present.

**Difficulty:** HUD alignment is a bounded task; shared input/hover/owner handling is medium complexity; full CK2-equivalent Vassal economics/opinion and all dynamic map modes are large subsystem work. Implement these as separate reviewable stages. No single “add all map modes” change should imply the simulation behind them already exists.

## 3. Exact OpenCK2 code map

Paths and line anchors are relative to the latest archive root. They are navigation aids for this snapshot, not permanent API positions.

| File / existing entry points | Current responsibility | Planned work |
|---|---|---|
| `src/game/main.c`: `struct menu` (~129), `campaign_map_area()` (99), `handle_event()` (2384) | UI state, fixed HUD exclusion rectangles, input routing and right/middle drags | Shared map hit result, UI occlusion, click/drag discrimination, hover timing and owner navigation |
| `main.c`: `draw()` (2022), `draw_campaign()` (1195), `setup_tooltip()` (1641) | Map/HUD order; setup-only province tooltip; raw HUD tab drawing | Resolve one tooltip target; render campaign province tooltip last; replace raw navigation loop with GUI-bound widgets |
| `main.c`: `inspect_campaign_character()` (1801), `campaign_character_click()` (1825) | Existing character-window opening, family/vassal navigation and two active tabs | Reuse opening path for province owner; keep target-ID validation; make current-date refresh explicit |
| `main.c`: `load_campaign_topbar()` (1064), `draw_campaign_topbar()` (1134), `hit_test()` (2116) | Partial GUI binding for resource/date/speed controls | Extend HUD binding lifecycle to navigation/conditional elements; eliminate stale hitboxes when widgets are hidden |
| `main.c`: `select_setup_mode()` (1929), `setup_select()` (1938), `move_map()` (1744), `load_realms()` (1297) | Terrain/Realms toggle and camera/history rebuilds | Introduce shared mode selection; invalidate overlays on camera/date/state changes; keep setup-only controls separate |
| `src/game/map_view.c`: `map_view_pick()` (896), `map_view_build()` (801), `map_view_move()` (858), `map_view_animate()` (698) | Projected screen-to-world samples, terrain/water rendering, camera | Reuse samples for picking and thematic overlays; do not replace camera math with a flat screen-to-bitmap approximation |
| `src/game/campaign.c/.h`: `province_map_load()`, `province_at()` (234), `province_history_read()` (175), `struct province` | RGB-to-province lookup, IDs/names, history paths | Add or accompany with date-aware province facts; preserve distinction between province ID and array index |
| `src/game/realms.c/.h`: `history_load()` (523), `read_history()` (136), `independent()` (218), `realm_county_info()` (449), `realm_province_ruler()` (504), `realm_ruler()` | Landed titles, dated holder/liege history, county/independent mapping, ruler lookup | Expose explicit owner/top-liege access; add character-based direct-vassal query; retain separate de jure and de facto relations |
| `realms.c`: `realm_primary_title()`, `realm_held_titles()`, `realm_vassal_titles()` (483), `realm_map_build()` (582), `realm_map_draw()` | Highest-rank title selection, title-based list, realm overlay/labels | Preserve setup callers; introduce new character-vassal query rather than silently changing old pagination contract; generalize overlay coloring |
| `src/game/character_view.c/.h`: `character_view_load()` (19), `character_view_vassals()` (351), `character_view_pick()`, `character_view_draw()`, `character_view_free()` | Character widgets, list art, row hit tests/rendering, cleanup | Original row columns/sorts/counts/scrollbar; shared row geometry; explicit selected-tab state as needed |
| `character_view.c`: `panel_tooltip()` (442) | Wrapping/clamping of a flat-color tooltip with plain text | Reuse layout initially; separate reusable tooltip rendering from character-specific code if needed; original skin and rich text later |
| `src/game/frontend_gui.c/.h`: `frontend_gui_load_file()`, `frontend_gui_find()` (63), `frontend_gui_rect()` (84) | GUI tree, direct-child lookup, recursive position/orientation accumulation | Use named parents; validate nested anchors and scaling; support missing layout semantics only when required by inspected widgets |
| `src/game/setup_toolbar.c/.h`: `load_sprite_definition()` (90), `load_widget()` (142), `setup_widget_load_node()` (175), `load_scrollbar()` (274), `setup_widget_draw()` | GFX resolution, frames, shortcuts, existing reusable scrollbar parts | Extend/correct sprite lookup scope, GUI scale, selected state and reusable scrollbar support without copying coordinates |
| `src/game/assets.c/.h`: `menu_assets_load()`, HUD texture bindings (~238–265), texture teardown | Eight raw HUD tab textures, minimap/province backgrounds and fonts | Replace duplicated raw widget loading incrementally; ensure exactly one owner per texture; maintain graceful missing-asset diagnostics |
| `src/game/ck2_script.c/.h`: `token()`, `field()`, `find()`, `load_from_directory()`, `ck2_translate()` | Loose-file parsing and localization | Reuse parser and mounted content resolution; add only supported date/format semantics; preserve unknown values |
| `src/open_launcher/asset_paths.c/.h`: `ck2_asset_path*()`, `ck2_list_assets()` | Mounted base/DLC/mod asset lookup and priority | Keep this as the file access route; no absolute installation paths or copies of proprietary assets |
| `Makefile` | C17 build, SDL2/image/mixer/zlib, named checks | Add proposed modules and tests when implementation starts; restore missing test sources before claiming those checks pass |

### Baseline limitations to resolve before implementation

- `Makefile` exists in this archive, unlike the earlier source-only upload. It references `tests/*.c` and `tools/*.c` that are **not included** in this 56-file archive. A target name alone is not a runnable test suite. [S]
- SDL2/SDL2_image/SDL2_mixer dependency discovery fails in the current environment. No full build or game launch is claimed. [S]
- Rendering/history frequently uses `menu->start_date` while the campaign calendar advances in `menu->campaign.date`. This plan must define whether the first milestone is a bookmark snapshot or live-date state. Do not quietly claim live ownership/weather by displaying a newer date over old data. [S,P]
- `province_at()` returns land provinces only; water/ocean hover needs a separate lookup policy if desired. [S]

## 4. Original interface definitions to reuse

### 4.1 Society, conditional topbar controls and the unidentified blank area

`interface/main.gui` has a `main` parent at `(0,-1)`. Its direct children include:

| GUI node | Local position | Sprite / details | Meaning and confidence |
|---|---|---|---|
| `icon_topbar_court` | 146,15 | `GFX_topbar_folder_court`; F3 | Court navigation [L] |
| `icon_topbar_laws` | 187,15 | `GFX_topbar_folder_laws`; F4 | Laws navigation [L] |
| `icon_topbar_technology` | 228,15 | `GFX_topbar_folder_tech`; F5 | Technology navigation [L] |
| `icon_topbar_military` | 269,15 | `GFX_topbar_folder_military`; F6 | Military navigation [L] |
| `icon_topbar_plots` | 310,15 | `GFX_topbar_folder_plots`; F7 | Intrigue navigation [L] |
| `icon_topbar_factions` | 351,15 | `GFX_topbar_folder_factions`; F8 | Factions navigation [L] |
| `icon_topbar_religion` | 392,15 | `GFX_topbar_folder_religion`; F9 | Religion navigation [L] |
| `icon_topbar_societies` | 433,15 | `GFX_topbar_folder_societies`; F11 is commented out | Society navigation [L]; do not enable the commented shortcut accidentally |
| `icon_topbar_republic` / `icon_topbar_nomad` | both 474,15 | Government-specific sprites; F10 commented out | Mutually overlapping alternatives [L]; runtime conditions need verification [U] |
| `icon_topbar_back` | 487,14 | `GFX_main_back_button`; shortcut b | Back control, not a wealth statistic [L] |
| `icon_topbar_back_nomad_rep_frame` | 505,30 | `GFX_back_button_rep_nomad` | Decorative/state-dependent frame candidate [L,U] |

Additional named positions: `extensions_begin=(433,15)`, `extensions_step=(40,0)`, `nomad_rep_back_button_offset=(28,26)`; backgrounds `topbar_extension_0`, `_1`, `_2`. [L] Their exact runtime inclusion/order is not established just by being present.

Current `draw_campaign()` draws eight textures at `(146 + i*41, 15)`, with a fixed `32×36` source crop and destination. It neither composes those background extensions nor derives each icon's dimensions from its GFX frames. This bypass is verified; the precise reason for the user's visible Society separation remains unverified. [S,U]

**Important GFX dependency:** Society's sprite is defined in `interface/domestic_societies.gfx`, with texture `gfx/interface/topbar_folder_societies.dds` and two frames. `setup_toolbar.c`'s fixed `gfx_files[]` currently includes main/frontend/alternate_start/generalstuff/domestic_dynastic/wonder, **not domestic_societies.gfx**. Therefore replacing the raw Society texture with a call to the current generic node loader alone would fail to resolve it. Extend the explicit GFX sources or introduce a cached mounted GFX index first. [S,L,P]

**Blank-control investigation procedure [P]:** record each drawn widget's GUI name, destination rectangle, selected source frame and visibility reason; capture the failing OpenCK2 HUD; temporarily inspect one suspect layer at a time; distinguish an actual button from decoration baked into a background. Match the offending rectangle to a source draw operation before removal. If it is an unsupported Republic/Nomad control, hide its control and hitbox under an explicit capability rule. If it is decoration, correct background composition. If it is Back, retain valid navigation semantics and remove only unintended empty presentation. Do not remove wealth/prestige/piety or the Family/Relations tabs by spatial guess.

### 4.2 Character tabs and Vassals

`interface/domestic_dynastic.gui`, parent `dynastic_view`:

| Visible tab | Button | Label | Local x,y |
|---|---|---|---|
| Family | `tab_1` | `family_text` | 11,349 |
| Relations | `tab_6` | `relations_text` | 99,349 |
| Vassals | `tab_2` | `vassals_text` | 187,349 |
| Court | `tab_3` | `courtiers_text` | 275,349 |
| Pacts | `tab_4` | `pacts_text` | 363,349 |
| Abroad | `tab_5` | `abroad_text` | 451,349 |

All six use `GFX_charview_tab` and GUI `scale=0.83`. Existing source binds them in this order, but its generic sprite loader does not apply general GUI scale. Check rendered size and hitbox width together. Only Family and Vassals have implemented selection paths; the other labels should not pretend their lists exist. Society does not belong in this table. [S,L]

Relevant original Vassals controls [L]:

- Sort buttons: `sort_tier` (39,381, `GFX_sort_88`, text `TIER`), `sort_name` (126,381, `GFX_sort_188`, `CHAR_NAME`), `sort_tax` (312,381, `GFX_sort_88`, `CHAR_TAX`), `sort_opinion` (398,381, `GFX_sort_112`, `OPINION`).
- `vassal_listbox`: position `(22,410)`, size `491×244`, spacing 1, border `10×10`, scrollbar `standardlistbox_slider`.
- Root template `vassal_entry`: `484×61`; `charview_list_bg`, `charview_list_overlay`, `portrait`, `portrait_frame`, `shield`, `name`, `title`, `relation_icon_strip`, `tax_who_icon`, `tax_bg`, `tax`, `opinion_bg`, `opinion`, and church-specific `pope_opinion*` elements.
- `portrait` exposes `CHARACTER_TOOLTIP` and `CHARACTER_TOOLTIP_DELAYED` localization keys.
- `interface/EU3_core.gui` contains the reusable scrollbar definition; associated sprites require GFX resolution too.

These define appearance and binding names. Tax amount/direction, opinion meaning, default ordering, count semantics and conditional overlays require gameplay data and original-game checks. Do not use the campaign's simplified monthly treasury income as a vassal tax value. [U,P]

### 4.3 Tooltip skin and data

The supplied `interface/EU3_core.gfx` defines a **textSpriteType** named `ToolTip`, using `gfx/interface/tooltip_bg.tga`. `assets.c` already loads a tooltip DDS for the setup tooltip. The existing `panel_tooltip()` instead draws a flat fill and border. A generic sprite loader that searches only `spriteTypes`/`spriteType` is not automatically sufficient for `textSpriteType`. [L,S]

The screenshot shows a province/owner heading, realm line, terrain, static climate, current winter, military information/defender bonus, supply limit, and modifier-key commands. [V] No complete dedicated dynamic province-tooltip layout was identified in the inspected GUI files. Do not invent a missing `province_tooltip.gui` or claim all tooltip content is authored in one loose window. The executable builds at least part of this text dynamically. [B,U]

### 4.4 Original campaign map-mode inventory

All following buttons are direct children of `main/minimap/mapmodes` in `interface/main.gui`. Every sprite name is `GFX_` followed by the exact button name; definitions are in `interface/main.gfx`. They use the corresponding `gfx/interface/mapmode_*.tga` texture paths and frame metadata; the `.gui` also supplies instant and delayed tooltip keys. Texture bytes are not in the interface ZIP.

| Exact button suffix after `mapmode_` | Key | Intended mode | Proposed first functional stage / missing dependencies |
|---|---|---|---|
| `terrain` | q | Terrain | Existing base renderer; expose shared selection state first |
| `realms` | w | Realms | Existing independent-realm mapping; retain existing colors as baseline |
| `diplo` | e | Diplomacy | Later: reference ruler, wars, alliances, truces, relation categories |
| `religion` | r | Religion | Province religion history and religion definition colors; not ruler religion |
| `culture` | t | Culture | Province culture history and culture definition colors; not ruler culture |
| `economy` | u | Economy | Later: actual provincial income/tax metric and verified normalization |
| `duchies` | i | De jure duchies candidate | Landed-title parent chain; confirm actual CK2 mode semantics/overrides |
| `kingdoms` | o | De jure kingdoms candidate | Same, plus dated de jure changes if supported |
| `dejureempires` | p | De jure empires | Same, with empire ancestor and drift/history handling |
| `revoltrisk` | a | Revolt risk | Later: actual revolt risk/modifier calculation |
| `dynasties` | s | Dynasties | Holder identity/dynasty data; verify county holder vs realm ruler basis and palette |
| `opinions` | d | Opinions | Later: opinion engine, reference character and display range |
| `directvassals` | f | Direct vassals under independent ruler | Character liege graph plus selected/reference realm semantics |
| `trade_zones` | g | Trade zones | Later: trade posts, merchant republic ownership, zone connectivity |
| `governments` | h | Governments | Later: government definitions, assignment and authored colors; no generic feudal fallback |
| `coalitions` | j | Coalitions/defensive pacts | Later: pact membership, targets and display rules |
| `epidemics` | k | Epidemics | Later: disease state/spread/resistance; static climate is insufficient |
| `players` | l | Players | Session/player ownership model; no invented multiplayer state |

**Verified identifiers, proposed semantics:** button existence, shortcuts and localization are [L]; actual color algorithms and all conditional/DLC visibility rules remain [U] unless separately established. “De jure duchies/kingdoms” is a semantic interpretation supported by context, not a traced color function.

`toggle_mapmodes` and `CustomMapMode1` through `CustomMapMode5` are siblings under `minimap`, not children of `mapmodes`. Reproduce their layout separately. Custom slots are slots, not five additional recovered game mechanics; persistence/remapping/drag behavior needs tracing. `GFX_mapmode_counties` and `GFX_mapmode_totaltechlevel` also exist, but are **not extra campaign buttons in this inspected mapmodes group**. A sprite's existence is not proof of an active button. [L,U]

Setup remains distinct: `frontend.gui` includes `mapmode_defacto_counties`, `mapmode_defacto_duchies`, `mapmode_defacto_kingdoms`, Religion, Culture, Governments and Players, plus Terrain/Realms. Existing `setup_toolbar.h` does not bind every one of those GUI elements. Keep Shattered/Random and frontend ornaments out of the campaign mode enum. [S,L]

## 5. Executable research: concrete entry points and limits

### Method

Fresh analysis used PE exception/unwind records to locate function ranges and Capstone disassembly to find direct RIP-relative/immediate string references. The GUI-oriented scan found 102 selected strings and 388 references. A supplemental byte-pattern search found source-path and localization-reference candidates; the tooltip and key GUI functions were subsequently disassembled from their PE function starts. Coverage is static and incomplete: indirect pointers, virtual calls, stripped names and code without unwind entries may require additional work.

Addresses below are preferred VAs for the SHA-256 above; subtract `0x140000000` for an RVA. Rebase them in a debugger if ASLR is active. Range ends are exclusive. No recovered `.cpp` source body or matching PDB was obtained.

| Investigation target | Concrete function/range or reference | What is established / not established |
|---|---|---|
| Province tooltip composition | `0x140910520–0x1409178fc` | Same function references occupation, de jure information, diplomacy, trade, disease, terrain/climate/winter and hotkey labels; strong tooltip-builder candidate [B, high for text composition; exact original method name unknown] |
| Terrain line | `0x140916f19` → `TERRAIN`; `0x140916f20` → `TERRAIN_DESC` | Concrete format/localization connection; terrain-selection algorithm still untraced |
| Climate line | `0x140916f99` → `CLIMATE`; `0x140916fa0` → `CLIMATE_DESC` | Climate is separately formatted from terrain |
| Current winter | `0x140917004` reads object dword `+0xd98`; branches to `MILD` at `0x140917030`, `NORMAL` at `0x140917060`, `SEVERE` at `0x140917090`; `CURRENT_WINTER` at `0x1409170cc` | High confidence for this display mapping (1/2/3); not proof of winter generation, duration or zero/default meaning |
| Province command hints | `0x140917156` → `PROVINCE_HOTKEY_INFO` | Exact key to look up in localization; string presence does not implement the commands |
| Supply display candidates | `0x140867d87` in `0x140866bb0–0x14086c44d`; `0x140b9cf90` in `0x140b9c5f0–0x140b9d87f` | `SUPPLY_LIMIT` references; neither is claimed to be the requested province's complete supply formula |
| Campaign mode bindings | `0x140660310–0x1406610f9` | Repeated lookups of `minimap`, `mapmodes`, all 18 names, then CustomMapMode1–5; high-confidence GUI binding routine, not proof of coloring semantics |
| Topbar bindings | `0x14065f8d0–0x14065fdf6` | Society at `0x14065fd1f`, Back at `0x14065fd8d`, Republic at `0x14065fbe1`, Nomad at `0x14065fc4b`; good starting point for visibility/placement callers |
| Additional topbar layout/state | `0x14065fe00–0x140660301`, `0x140668af0–0x14066b165` | Additional Society references; investigate conditions and writes before assigning descriptive names |
| Vassal row template binding | `0x1404f67d0–0x1404f6941`, reference `0x1404f6817` → `vassal_entry` | Template connection is verified; row data semantics not decoded |
| Character view sort bindings | `0x1404fc2f0–0x1404fd076`, refs `0x1404fcbfe` → `sort_opinion`, `0x1404fcc57` → `sort_tax` | Concrete sort-control lookups; comparator direction/formulas unknown |

Source-path anchors recovered directly from the executable:

| Embedded filename | Example reference → containing function | Relevance and confidence |
|---|---|---|
| `map.cpp` | `0x1408133ee` → `0x140812c90–0x140813698` | Map subsystem diagnostic site; medium as behavior investigation starting point |
| `province.cpp` | `0x14090df2e` → `0x14090dde0–0x14090f333` | Province subsystem diagnostic site; not itself proof of input dispatch |
| `provincehistory.cpp` | `0x14094d10a` → `0x14094d0b0–0x14094d325` | Province history investigation anchor |
| `characterhistory.cpp` | `0x14028cd39` → `0x14028c6f0–0x14028e94e` | Historical identity/relationship anchor |
| `title.cpp` | `0x140a43b76` → `0x140a439f0–0x140a43cfa` | Title ownership/relations anchor |
| `titlehistory.cpp` | `0x140a8b44d` → `0x140a8b3f0–0x140a8b644` | Dated title-state anchor |
| `society_object.cpp` | `0x140a08033` → `0x140a07c00–0x140a0815e` | Society gameplay object; no evidence this is the topbar layout method |
| `character_opinion.cpp` | `0x14041ab70` → `0x14041a650–0x14041ac85` | Opinion-system diagnostic anchor; exact opinion equation unknown |
| `eu3graphicalmap.cpp` | `0x14062c8c4` → `0x14062c2d0–0x14062cab5` | Graphical map subsystem anchor |
| `pdxmap.cpp` | `0x1408d2305` → `0x1408d1c20–0x1408d2783` | Map subsystem anchor |

These filenames genuinely occur in the binary. Associating one filename with every requested feature would be a guess. `gamestate.cpp`, `government.cpp` and `governmentflavor.cpp` from earlier research remain useful supplementary targets, but government parsing is not equivalent to government assignment or map coloring.

### Next disassembly questions, in useful order

1. Trace callers and mode branches in `0x140910520`; find owner, realm-name, defender bonus and supply-value producers. Resolve `PROVINCE_HOTKEY_INFO` from the original localization before translating placeholders.
2. Follow callbacks registered by `0x140660310` and `0x14065f8d0`. Recover mode IDs, state-dependent visibility and extension offsets. Do not infer original numeric IDs from OpenCK2's setup enum.
3. Follow row construction and sort callback targets from `0x1404f67d0` and `0x1404fc2f0`. Determine tax units, opinion target/direction and stable ordering.
4. Investigate source-path anchors when relevant data flows reach them; do not spend time decompiling an entire file-sized region before implementing a bounded UI milestone.

## 6. Data contract and architecture to implement

All names in this section are **proposed APIs/types [P]**, unless explicitly called existing.

### Shared map hit and owner model

Introduce a `map_target` result containing `province_index`, `province_id`, county title key, `owner_id`, independent realm title key and `top_liege_id`, plus separate known/valid flags. Keep occupied/controller identity separate for later support. Do not store an ambiguous single `ruler` field.

Resolve via existing `map_view_pick()` → `province_at()` → pointer difference for **array index** → `realm_county_info(...false)` / `realm_county_info(...true)` → `realm_ruler()`. RGB-sorted province array indexes are not province IDs. Validate all transitions and handle missing county/holder/profile without dereferencing null.

Define owner navigation initially as **county holder**, based on the existing county mapping. [P] Verify province-capital holding owner versus county holder against actual CK2 data; the screenshot's emperor example does not distinguish them. Do not use independent ruler as a silent fallback. Shift-right-click targets the top liege when known. [V,P]

For the first milestone, identify the state as bookmark history. For live-date support, separate immutable history inputs from mutable campaign ownership/state and update the same read model consistently. Avoid reparsing all history files on every calendar day or mouse move.

### Input ownership

Route input to modal windows, character/building panels, HUD/mode controls, then map. Derive blocking rectangles from visible widgets/windows; the fixed `campaign_hud[]` does not accurately represent an open 500+-pixel character panel. Its current fixed left panel ends around x=348 while the character panel extends farther. [S,P]

Use a pending pointer gesture: button, down point, maximum movement, down target and modifiers. A small movement threshold (initially configurable, e.g. 4 logical pixels) is an OpenCK2 usability choice, not a recovered CK2 constant. Preserve right-drag pan once threshold is crossed; on stationary release dispatch owner navigation. Middle-button pan remains available. Cancel pending gestures on focus loss, screen transition and incompatible modal changes. Decide whether modifier state is captured on press or release and test it consistently.

Do not advertise Alt-right-click interactions until a real interaction menu exists. The reference screenshot documents Control-left-click sub-realms, Right-click owner, Shift-right-click top liege, Alt-right-click interaction and Alt+Shift-right-click top-liege interaction. Reproduce supported commands first and build hint text from actual supported commands.

### Province facts and tooltip model

Proposed `province_facts` cache: date/revision, terrain key, climate key, province religion/culture keys and known flags. Resolve static/dated information through original province history and relevant common/map definitions, with mounted DLC/mod overrides. Candidate files such as `map/terrain.txt` and `map/climate.txt` must be confirmed against the user's actual installation before being treated as authoritative paths. `common/religions` and `common/cultures` directories occur in the binary; their complete schemas/colors still require the loose files.

The renderer's `terrain.bmp` palette selection (`index_at(...) & 15`) is a visual tile operation, **not a verified province terrain classification**. Do not infer province terrain from its center pixel or assign a climate category based only on latitude. Read authored overrides first, then investigate any original fallback algorithm.

Separate static climate from current winter, and current winter from arbitrary month-based guesses. Unknown supply limit, tax/opinion, defense percentage, disease and weather should be omitted or clearly shown as unavailable. Supply is potentially viewer/army/context dependent; a single cached number per province may be insufficient.

Tooltip model should be structured lines/runs with localization keys, resolved values, optional section separators and supported action hints. First milestone can reuse `panel_tooltip()`'s wrapping/clamping; visual parity later uses `ToolTip` skin and the original font/text styling. Do not print raw CK2 formatting codes or invent supply values. Cache facts outside the draw loop; invalidate on province, date, content or relevant simulation revision.

### Character-based Vassal model

Keep `realm_vassal_titles()` for existing title-based setup callers. Add a separate direct-character-vassal query that considers all titles held by the selected character and resolves each candidate's actual liege relationship. Deduplicate by character ID, choose one display title per character, exclude self and deeper indirect vassals, and return a total count independent of page capacity.

Existing failure modes to fix: multiple held titles belonging to one vassal can yield duplicate rows; considering only `realm_primary_title()` can omit vassals under another held title; equality to one liege title key does not fully model character allegiance. These follow from source structure; the exact set of affected historical rulers has not been tested.

Proposed row state: character ID, display title, rank, name/title text, portrait/shield, optional tax, optional opinion, visibility flags and stable sort key. Unknown tax/opinion remain unknown, never zero. Sort Tier/Name first; activate Tax/Opinion sorting only when the model can distinguish known values. Preserve page position by stable row identity when practical and clamp after filtering/sort changes.

### Shared map-mode model

Proposed `map_modes.c/.h`: semantic mode enum/descriptor, supported-state flag, selected/reference character or realm, revision key and a province color resolver. Proposed `campaign_hud.c/.h` can own campaign navigation/mode widget binding. New module names are proposals, not files already in the archive.

Reuse `map_view.samples`/province geometry, existing realm color data and water rendering. Separate data mapping from projected overlay generation. Cache province-to-group/color values; rebuild only when their inputs change. Camera movement changes projection; it should not reload history unnecessarily. Preserve selection highlights, borders, realm labels and tooltip hit detection across mode switches. Keep de jure parentage distinct from current liege chains and ownership distinct from occupation/controller.

Original GUI/GFX governs placement, sprites, shortcut and tooltip metadata; engine code still needs semantic handlers. An explicit mapping from original widget IDs to OpenCK2 capabilities is appropriate. Hardcoding historical rulers, province IDs, colors or guessed mechanics is not.

## 7. Implementation stages and exit gates

### Stage 0 — Establish a reproducible baseline (P0)

**Change later:** build/test scaffolding only as necessary. **Inspect now:** Makefile and mounted content path.

- Recover missing existing tests/tools from the actual repository; obtain SDL dependencies and actual game-data root without redistributing those assets.
- Record enabled mods/DLC, asset-priority order, resolution/logical size and source revision. Capture baseline OpenCK2 HUD and a known province with county owner different from top liege.
- Verify whether earlier research modules were intentionally omitted from this archive; any merge must be explicit.
- Establish the first delivery as bookmark-correct; document live-simulation limitations.

**Exit gate:** baseline build launches or exact build blockers are recorded; relevant test files are actually present; a known owner/top-liege fixture and visual comparison setup are identified.

### Stage 1 — HUD binding audit and bounded layout corrections (P1, low/medium)

**Files:** `assets.c/.h`, `main.c` navigation drawing/topbar lifecycle, `setup_toolbar.c` GFX loading, `frontend_gui.c` only if required; optionally new `campaign_hud.c/.h`.

- Bind the eight actual navigation nodes from `main.gui`, including Society's correct GFX source and two-frame metadata.
- Respect parent offsets, GUI scale where used, selected/hover/disabled presentation, and missing-asset fallback. Avoid globally reinterpreting all anchors before a targeted fixture proves it necessary.
- Audit extension backgrounds and conditional Republic/Nomad/Back elements. Identify the blank element by draw record before hiding/removing it.
- Replace coarse HUD navigation hover/hit rectangles with visible widget geometry. Keep unsupported tabs visible with clear status; hiding the specifically unwanted blank control should not create an invisible clickable region.

**Exit gate:** all eight navigation icons appear in the original defined order; Society shares the row and expected background; blank target is positively identified and removed/conditioned; real wealth and other requested icons remain present. No gameplay screens need to be implemented merely to correct the row.

### Stage 2 — Shared picking and province-owner right-click (P1, medium)

**Files:** `main.c` input paths, `realms.c/.h`, `campaign.c/.h`; optional new `province_interaction.c/.h`.

- Implement one validated map-target query and use it for hover, clicks and mode context.
- Separate UI input from map input, and stationary right-click from drag.
- On owner action call `inspect_campaign_character()` with the county holder's actual profile. Open top liege only for the explicit modified action. Reopen/refresh correctly if the same ID already has an open view.
- Preserve left-click selection and right/middle panning; cancel on focus loss. Keep setup behavior unless intentionally changed and documented.

**Exit gate:** nested-vassal fixture opens count on right-click and independent ruler on Shift-right-click; dragging never opens a character; clicking panels never selects the map behind them.

### Stage 3 — Honest campaign province tooltips (P1/P2, medium)

**Files:** `main.c` hover/draw order, `campaign.c/.h` or new `province_facts.c/.h`, `ck2_script.c` as needed, shared tooltip renderer extracted from `character_view.c` only when useful.

- First add province/owner/realm lines and correct action hints from the same query used by navigation.
- Resolve terrain and static climate from verified loose data. Expose province religion/culture for upcoming map modes.
- Add hover delay/cancellation and one tooltip target per frame; no file I/O in render or per-frame motion handling.
- Then bind original tooltip skin and localization/rich text. Add winter/supply/defense only after state/formula verification; unavailable sections remain absent.

**Exit gate:** correct tooltip on unobscured land; no tooltip through UI, during drag, after window leave or over invalid pixels. Tooltip content never disagrees with the owner action. Unknown numeric military details are not fabricated.

### Stage 4 — Complete the structural Vassals list (P2, medium/high)

**Files:** `realms.c/.h`, `character_view.c/.h`, `main.c` click/wheel paths, reusable scrollbar code in `setup_toolbar.c/.h` or a small shared widget module.

- Add and test character-based direct-vassal enumeration; preserve existing setup function behavior.
- Bind original sort headers, row name/title/rank/shield columns, visible count and `standardlistbox_slider`.
- Use one function to compute row rectangles for drawing and picking, including template spacing/borders, scale and clipping. Remove unrelated fixed row offsets/dimensions only after mapping them to original definitions.
- Add Tier/Name sort directions and deterministic tie-breaks; complete portrait/row navigation and scrolling.
- Bind tax/opinion visuals with unknown placeholders. Implement real values and conditional church/relationship overlays in a subsequent separately validated change.

**Exit gate:** each direct character vassal appears once; total count and scrollbar match; sort/select/scroll remain consistent; row labels and shield refer to the same character/title; no invented tax/opinion.

### Stage 5 — Campaign map-mode controls and shared state (P2, medium)

**Files:** new proposed `map_modes.c/.h` and possibly `campaign_hud.c/.h`; `main.c`, `setup_toolbar.c`, `realms.c`, `Makefile`.

- Bind all 18 original mode icons, toggle and custom slots from their correct GUI parents; preserve frame metadata, shortcuts and tooltips.
- Implement shared Terrain/Realms selection first, with active visual state. Expose other modes as unavailable until their resolvers work; clicking an unavailable mode must not silently recolor the map as Realms or set a false active state.
- Build explicit setup-to-semantic-mode mapping without merging setup-only Shattered/Random controls into the map modes.
- Test mode state through pan, zoom, resize/logical-size changes and panel opening. Unimplemented custom slot remapping can remain disabled with a clear status.

**Exit gate:** Terrain and Realms switch in campaign via buttons/shortcuts; remaining original icons render consistently and report support status; no click-through or shortcut theft from text entry/modal windows.

### Stage 6 — Incremental data-backed thematic modes (P2/P3, medium/high)

Recommended order: province Religion → province Culture → de jure Duchies/Kingdoms/Empires → Dynasties → Direct Vassals. Reorder only if actual loose-file coverage favors another mode.

- Religion/Culture share province facts and use original definition colors with explicit missing-data behavior.
- De jure modes traverse authored parent hierarchy and separately handle dated changes; never substitute current liege chains.
- Dynasty colors/palette and holder basis require original comparison; do not pretend a deterministic engineering fallback palette is an original CK2 color rule.
- Direct Vassals requires the character liege model and an explicitly chosen reference realm.
- Keep cache/revision accounting per mode; add legends and mode-specific tooltip facts where data supports them.

**Exit gate per mode:** independently verified province group fixtures; correct unknown/unlanded/sea handling; unchanged geometry/selection; screenshot comparison at the same bookmark and content configuration. Do not wait until all six are coded to test the first.

### Stage 7 — Dynamic mechanics, visual parity and regression (P3)

Diplomacy, Economy, Revolt Risk, Opinions, Trade Zones, Governments, Coalitions, Epidemics and Players depend on systems absent or incomplete in this baseline. Implement prerequisites in separate features, then add each mode. Supply/winter/defense and exact tax/opinion are similarly separate milestones.

Compare original CK2 and OpenCK2 at matching bookmark/date, character/province, resolution, UI scale and DLC/mod state. Start with 769, 867 and 1066 where data is available; additionally use the 1337 reference for the shown list/tooltip. A screenshot at a different date or content configuration is not a reliable numeric oracle.

**Exit gate:** all supported modes have data tests and runtime validation; unsupported behavior is documented; no claim of complete CK2 equivalence without side-by-side evidence and edge-case coverage.

## 8. Acceptance-test matrix

These are planned tests, **not tests already run**. Prefer small synthetic history fixtures for data semantics and actual game assets for layout/render tests. Restore absent pre-existing test files before expanding the Makefile checks.

| ID | Scenario | Required result |
|---|---|---|
| B01 | Build exact archive/repository with dependencies | `make game` compiles/links; missing tests/tools identified separately; no success claim from `make -n` alone |
| B02 | Mounted DLC/mod replaces GUI, GFX or history | The runtime honors existing asset priority; plan-specific hardcoded paths do not bypass it |
| H01 | Standard HUD at baseline logical size | Court→Laws→Technology→Military→Plots→Factions→Religion→Society order; geometry follows GUI parent/child definitions |
| H02 | Society sprite binding | Resolves from domestic_societies.gfx, two frames; no missing sprite or forced 32×36 crop |
| H03 | Blank target | Draw record identifies its exact origin; unwanted element and hitbox absent; wealth/prestige/piety remain intact |
| H04 | Unsupported/missing optional HUD asset | Clear diagnostic/fallback; no crash, overlapping substitute or texture leak; remaining HUD loads |
| H05 | Conditional Republic/Nomad/Back configuration | No overlapping alternatives; intended visibility confirmed against original behavior before enabling |
| I01 | County held by C, vassal of D, under independent E | Right-click opens C; Shift-right-click opens E; neither changes the player character |
| I02 | Independent county | Owner and top-liege actions both target its holder consistently |
| I03 | Invalid sample, no county, no holder or missing profile | Safe no-op/known unavailable state; never arbitrary emperor or previous character |
| I04 | Right drag over threshold; middle drag; small jitter | Pan only for drag; stationary right-click navigates once; threshold behavior consistent in logical coordinates |
| I05 | Open character panel, buildings, modal, HUD or minimap above map | Visible UI consumes input; no map selection/navigation/hover through it |
| I06 | Focus loss/window leave/screen change midgesture | Gesture and tooltip canceled; no later accidental navigation |
| T01 | Province owner differs from top liege | Tooltip names both correctly and navigation uses the same IDs |
| T02 | Move between provinces, then stand still | Hover delay resets; cached text updates; no history reads in drawing or every mouse event |
| T03 | Map pan/zoom while pointer stationary | Hover target recomputed from new projection/revision; old province text not retained |
| T04 | Terrain override, dated culture/religion change, missing climate | Authored data and date applied; unknown fields omitted/marked; no center-pixel terrain guess |
| T05 | Every viewport corner; long localization | Tooltip remains within viewport, wraps, clips and restores renderer state; supported formatting renders visibly |
| T06 | No winter/supply/defense engine | No fabricated current winter or numeric supply/bonus; hints advertise only implemented actions |
| T07 | Game date advances | Either state truly updates or milestone explicitly remains bookmark-based; old data never presented as live state |
| V01 | One vassal with several held titles | One row per character; correct representative title; count not number of title records |
| V02 | Selected ruler holds multiple liege titles | Direct character vassals under all relevant held titles considered; indirect vassals and self excluded |
| V03 | 0, 1, exactly page size, and >page size vassals | Correct total, clipping, end-scroll clamp, empty state; scrollbar drag/arrows/page/wheel consistent |
| V04 | Tier/Name ascending/descending; tied names/ranks | Deterministic ordering, stable tie-break, matching header/active indication and row hitbox |
| V05 | Tax/opinion missing vs actual zero | Missing stays distinguishable from zero; unsupported sorts do not imply valid values |
| V06 | Click portrait, label or visible row; partly clipped row | Opens intended character only; no hit outside list clipping; no stale prior-page ID |
| V07 | Switch inspected ruler/date while scrolled | Rows/count/art refresh; offset clamped/reset; no stale family/vassal state or freed texture access |
| M01 | Inspect original campaign controls | All 18 names, shortcuts, tooltips and toggle/custom siblings bound from loose files; not confused with frontend de facto buttons |
| M02 | Terrain↔Realms through mouse and key | Consistent selected state, overlay, legend, tooltips and selected province; no unsupported mode pretends to work |
| M03 | Change mode then pan/zoom/open panel | Camera/picking/borders/water/highlight remain correct; supported mode persists; caches invalidate correctly |
| M04 | Religion/Culture fixture where ruler and province differ | Mode uses province identity and original definition color, not ruler identity |
| M05 | De jure county parent differs from de facto liege | De jure mode follows authored hierarchy; Realms follows independent realm; neither borrows the other's mapping |
| M06 | Dynasty/Direct Vassal reference changes | Correct target model and cache invalidation; no reuse of unrelated selected-player state |
| M07 | Unavailable dynamic mode or custom slot | Clear support status, no false active mode and no wrong map recoloring; no shortcut interception in text input |
| P01 | Repeated hover/open/sort/scroll/switch | Bounded cache/texture use; no I/O in hot draw loop; no sanitizer failures in testable data components |
| R01 | Side-by-side original comparison | Same date/ruler/province/mode/content/scale; record residual differences instead of asserting parity from appearance alone |

Build/test targets that can be reused **after restoring their missing sources**: `check-campaign`, `check-setup-toolbar`, `check-holdings`, `check-portraits`, `check-construction`. Proposed new targets: `check-province-interaction`, `check-province-facts`, `check-vassals`, `check-map-modes`, plus an SDL render/GUI fixture check. Names are proposals. Use ASan/UBSan for parser/model tests when supported; report any environmental restrictions rather than claiming unrun checks passed.

## 9. Remaining unknowns and required evidence

| Unknown | Why it matters | How to resolve |
|---|---|---|
| Exact unwanted blank element | Several controls/decorations are plausible; removal by guess could delete Back or a valid stat | Capture current OpenCK2 screen and draw-identity log, compare to GUI rectangles and background composition |
| Society's reported visual separation | Source already uses the expected basic x-slot | Inspect actual texture dimensions/frame cropping, enabled DLC/mod UI files, parent offsets and topbar extensions |
| County holder vs province-capital holding owner | “Owner” may differ in special configurations | Compare original right-click with county/capital-holder fixtures; trace original input callback |
| Tooltip modifier placeholders and current winter generation | Binary gives display keys/enum branches, not complete game state | Read localization key values and loose data; follow data producers in original tooltip function |
| Province terrain/climate fallback paths | Visual renderer's tile selection is not a gameplay classification | Inspect actual map/common files, history overrides and province parser call graph |
| Supply/defender formula and context | Numbers can depend on modifiers, armies, owner/viewer, weather and date | Follow original tooltip's numeric producers and compare controlled scenarios |
| Direct-vassal exceptions, rank/display title and default ordering | Multi-title characters and special governments complicate rows | Character/title liege fixtures, original list comparison and row callback tracing |
| Tax/opinion values, direction and units | Cannot safely reuse simplified treasury income or dummy zero | Recover value producers/localization; implement the corresponding simulation model first |
| Map-mode reference selection and legends | Direct Vassals, Diplomacy and Opinions need context | Trace callbacks/state changes and test original selection/modifier behavior |
| Exact colors and normalization of thematic modes | A plausible palette is not original equivalence | Inspect definition colors and original color resolver branches; record any engineering fallback explicitly |
| Custom slots, expanded minimap layout and persistence | GUI definitions establish slots, not customization semantics | Trace mode-binding callbacks and settings serialization; compare original UI |
| DLC/government visibility conditions | Overlapping controls do not encode all visibility logic | Trace topbar state callers and compare relevant governments/content configurations |
| Live-date state | Current code uses start_date in many readers | Choose explicit bookmark milestone, then introduce coherent mutable state/history application |
| Full repository/test coverage | Latest ZIP omits sources referenced by its Makefile | Use actual repository build files/tests; do not reconstruct absent tests from target names alone |

## 10. Prioritized TODO checklist

Check boxes remain open because this request authorizes planning, not implementation.

### P0 — Baseline and evidence

- [ ] BASE-01: Confirm `OpenCK2_character_icons_extended.zip` matches the intended working branch; record revision/content settings. **Stage 0; acceptance B01–B02.**
- [ ] BASE-02: Restore missing tests/tools and install/locate SDL build dependencies; run baseline build and existing checks where available. **Stage 0; B01.**
- [ ] BASE-03: Capture current OpenCK2 HUD and record draw identities for Society/blank element; identify exact offending layer. **Stage 0/1; H01–H03.**
- [ ] BASE-04: Create fixture distinguishing county owner, intermediate liege and independent ruler; explicitly choose bookmark-state milestone. **Stage 0; I01,T07.**
- [ ] BASE-05: Preserve legacy research/module differences as an explicit merge decision; do not assume government catalog exists. **Stage 0.**

### P1 — First usable interaction delivery

- [ ] HUD-01: Bind original eight topbar navigation widgets, add Society GFX source and correct sprite-frame geometry. **Stage 1; H01–H02,H04.**
- [ ] HUD-02: Correct background/conditional composition and remove only positively identified blank element and its hitbox. **Stage 1; H03,H05.**
- [ ] INPUT-01: Create one validated map target, preserving province index/ID and owner/top-liege separation. **Stage 2; I01–I03.**
- [ ] INPUT-02: Add visible-UI occlusion and click-versus-drag state; preserve left selection and right/middle panning. **Stage 2; I04–I06.**
- [ ] INPUT-03: Connect stationary owner right-click and Shift-right-click to existing character window opening. **Stage 2; I01–I03.**
- [ ] TIP-01: Add campaign province/owner/realm tooltip and supported action hints; single tooltip target with delay/cancellation. **Stage 3; T01–T03,T06.**

### P2 — Data and structural UI completeness

- [ ] TIP-02: Parse/cache province terrain, climate, religion and culture from confirmed loose data with date/provenance/known flags. **Stage 3; T04,T07.**
- [ ] TIP-03: Bind original tooltip skin, localized runs and viewport-safe layout; preserve unknown military fields. **Stage 3; T05–T06.**
- [ ] VAS-01: Add character-based direct-vassal enumeration across held titles; deduplicate and return total separately from visible page. **Stage 4; V01–V03.**
- [ ] VAS-02: Bind original headers and row name/title/rank/shield layout, count and visible scrollbar. **Stage 4; V03,V06–V07.**
- [ ] VAS-03: Add Tier/Name sorting with deterministic ties and shared draw/hit geometry; keep tax/opinion unknown when unsupported. **Stage 4; V04–V05.**
- [ ] MODE-01: Introduce semantic shared map-mode state and GUI bindings for all 18 campaign buttons, toggle and custom slots. **Stage 5; M01,M07.**
- [ ] MODE-02: Implement campaign Terrain/Realms switching and selection feedback; keep setup-only controls distinct. **Stage 5; M02–M03.**
- [ ] MODE-03: Add Religion/Culture from province facts and original color definitions. **Stage 6; M04. Depends TIP-02,MODE-01/02.**
- [ ] MODE-04: Add de jure Duchies/Kingdoms/Empires from authored hierarchy with explicit dated-change limits. **Stage 6; M05.**

### P3 — Dependent mechanics and parity

- [ ] RE-01: Follow original tooltip producers for owner/defense/supply and resolve PROVINCE_HOTKEY_INFO localization. **Section 5; T06/R01.**
- [ ] RE-02: Trace topbar visibility/layout callers and map-mode callbacks/reference context before enabling uncertain behavior. **Section 5; H05,M06–M07.**
- [ ] RE-03: Trace Vassal row/sort callbacks for tax/opinion semantics and special government cases. **Section 5; V05.**
- [ ] VAS-04: Implement real tax/opinion models, conditional row overlays and corresponding sorts. **Stage 7; V05,R01.**
- [ ] MODE-05: Add verified Dynasty and Direct Vassal context/palette behavior. **Stage 6; M06. Depends VAS-01.**
- [ ] MODE-06: Add each remaining dynamic map mode only after its data/simulation prerequisites exist. **Stage 7; M07,R01.**
- [ ] TIP-04: Add real current winter, defense and context-sensitive supply displays only after formulas/state are verified. **Stage 7; T06–T07,R01.**
- [ ] STATE-01: Replace bookmark-only assumptions with coherent mutable campaign state and revision-aware caches. **Stage 7; T07,M03,M06.**
- [ ] QA-01: Run leak/sanitizer/performance checks appropriate to changed components; compare actual original screenshots at matched dates/settings. **Stage 7; P01,R01.**
- [ ] DOC-01: Record each supported feature, source evidence, original-game differences and remaining unsupported cases in the project TODO/report. **Every stage.**

## 11. Recommended first implementation batch

Keep the first batch limited to **HUD diagnosis/correction, correct owner resolution, stationary right-click navigation, and basic province/owner/realm hover text**. It is independently useful and establishes the data/input foundation for later Vassals and map modes. Terrain/climate enrichment, complete list economics and dynamic thematic modes follow as separate stages.

No existing C, header, Makefile or original interface asset has been edited for this planning deliverable. Binary observations support the investigation targets and some display mappings; they do not establish complete equivalence with CK2.
