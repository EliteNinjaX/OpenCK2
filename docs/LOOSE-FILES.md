# CK2 loose-file compatibility

OpenCK2 targets CK2 3.3.5.1 (SOHY) behavior. Use the player's installation
for data, scripts, and art wherever the original engine exposes them as files.
The OpenCK2 launcher is outside this requirement. The engine supplies the
meaning of those files, including script execution, screen actions, and
simulation rules. The matrix below records current coverage by file family.
A listed reader establishes source coverage for the stated fields only.

## Coverage and exceptions

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
| `interface/*.gfx` model declarations, `.xac`, `.mesh`, `.xsm`, and other animation files | Static geometry imports for the 370 unique base model names checked earlier. The 3D map frame renders in-game. DDS support covers the formats used by existing readers. | Temporary. Per-part material binding, general actor placement, skinning, animations, and the full material/shader and DDS formats remain pending. See [map rendering evidence](reverse-engineering/MAP-RENDERING.md). |
| `interface/sound.sfx`, `sound/`, `music/`, `soundtrack/` | Uses local sound definitions for selected cues and plays the installed menu track. | Temporary. Contextual playlists, ambient triggers, mixing rules, and the remaining audio declarations need implementation. |
| `common/defines.lua` and other native settings inputs | Selected portrait values are read as text. Settings uses a separate C layout and applies a subset of controls. | Temporary. Recover which defines are read, their override order, and which are Lua data or engine-bound behavior. UI scale, video pipeline and most simulation settings are pending. |
| DLC archives, folder and ZIP mods, `replace_path` | Existing resolver supplies mounted assets to supported readers. Toolbar tests cover a folder mod's GUI and a sprite definition overriding a base definition in another supported file. | Temporary. Vanilla discovery, duplicate declaration handling, ZIP overrides, total conversions, and order across multiple mods need matched reference runs. Format support and mount availability are separate. |
| `tutorial/`, save and conversion formats, multiplayer and account data | No native tutorial progression, vanilla save round-trip, or network runtime. | Temporary for tutorials and save data. Implement the engine systems behind their data. Steam, account and other online services require runtime integrations and cannot be recreated by parsing loose files alone. |

## Toolbar evidence and temporary choices

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

## Verification

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

## Next milestones

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
