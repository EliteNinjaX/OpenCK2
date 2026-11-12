# OpenCK2 parity roadmap

This is the entry point for human contributors and AI coding agents. It records
the implementation baseline at commit `c211c59`, the questions that still need
original CK2 investigation, and the checks that let the next contributor resume
the work. Update the relevant milestone and link its evidence when closing a
task.

## Target and scope

OpenCK2 is a cross-platform, reverse-engineered reconstruction of the
original Crusader Kings II engine in C17. The fidelity target is how a
native Windows, Linux, and macOS port of the original engine would behave.
The project works from the installed game, observed behavior, and binary
analysis. The current runnable baseline is Linux.

Recover the original architecture, data structures, algorithms, ordering,
rounding, and edge cases where evidence permits. Use the installed game
assets, scripts, and history at runtime. Document temporary approximations
with a reason, replacement condition, and roadmap item. Keep extracted
binaries, commercial assets, personal saves, and raw analysis dumps local.

Track **architecture and recovered behavior**, **historical state**,
**simulation**, **rendering**, **interactions**, **scripts**,
**save compatibility**, and **DLC and mod compatibility** separately.
Determinism, save interoperability, and mod support are core engineering
milestones. Exact executable byte matching is on hold.

See the [loose-file compatibility audit](docs/GAME_DATA_AND_UI.md#source-docs-loose-files-md) for the current reader coverage, known exceptions, and the next GUI, map, and script milestones.

Start with [AGENTS.md](AGENTS.md) for shared instructions and the
[reverse-engineering knowledge base](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-readme-md)
for evidence and starting points for humans and AI tools.

## Reference and tools

| Item | Baseline |
| --- | --- |
| Reference | Steam app 203770, Windows x86-64 PE, build 7399341, CK2 3.3.5.1, vanilla checksum SOHY |
| Executable | `CK2game.exe`, SHA-256 `a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d` |
| Runtime | Local Steam installation, originally observed through Proton 10.0 |
| Analysis | REA 6.8.0 with Ghidra 12.1.4; Kuna did not support this PE in the prior investigation |
| Source notes | [Launcher startup and content](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-launcher-decomp-md), [starting eras and historical UI](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md), [map renderer](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-map-rendering-md), [settings backlog](docs/GAME_DATA_AND_UI.md#source-docs-settings-todo-md) |

From the repository root, build with GNU Make, a C17 compiler, pkg-config,
SDL2, SDL2_image, SDL2_mixer, and zlib. Zenity is used by the graphical folder
picker.

```sh
make BUILD_DIR=bin/roadmap-check launcher game check-launcher check-campaign CFLAGS='-O2 -Werror'
CK2="${CK2_DIR:-$HOME/.steam/steam/steamapps/common/Crusader Kings II}"
sha256sum "$CK2/CK2game.exe"
./bin/roadmap-check/OpenCK2Launcher --assets "$CK2"
./bin/roadmap-check/ck2-game --assets "$CK2"
```

The existing automated checks cover the recovered launcher model and the
prototype campaign. They do not establish vanilla parity. Use an isolated
profile for reference runs and save fixtures. Record the build hash, in-game
checksum, selected DLC and mods, start date, selected ruler, resolution, UI
scale, language, and inputs. Compare base-only and DLC-enabled profiles
separately. Use the same camera position and zoom for visual comparisons.

### REA workflow

1. Start with a narrow question and a reproducible CK2 observation. Read the
   installed GUI, GFX, shader, localization, or history files when they define
   the behavior directly.
2. When behavior remains ambiguous, use REA with the Ghidra provider. Confirm
   the executable hash, locate a known string or data anchor, then trace its
   references and callers. A string hit identifies where to look.
3. Record findings in the knowledge base. Include the REA and provider
   versions, evidence ID, analysis profile, executable hash, image base,
   and relevant VA or RVA. Label direct observations, binary-supported
   findings, inferences, and hypotheses separately. Record callers, object
   relationships, field offsets, and update order when known.
4. Implement the observed behavior in the existing C module or a focused new
   module. Verify it through the normal launcher, UI, or simulation entry
   point and compare it against original CK2. Add a focused regression
   check when existing tests would miss it. Record remaining differences
   and return to partial systems as new evidence becomes available.

REA and Ghidra are installed separately from the repository. With `rea` on PATH
and `GHIDRA_INSTALL_DIR` set to a local Ghidra installation, a starting probe is:

```sh
mkdir -p work/rea
rea doctor --provider ghidra --format json
rea search "$CK2/CK2game.exe" 'game_rules_window' --provider ghidra --format json > work/rea/rules.json
```

If auto-analysis times out, earlier work completed using
`REA_GHIDRA_STARTUP_TIMEOUT_MS=900000` or the reduced analyzer profile described
in the evidence notes. Record the chosen profile with the result. If the
reference executable or the analysis provider is unavailable, mark any
engine behavior that depends on it as unverified.

## What works at the baseline

**Working** means the feature has a usable path in OpenCK2. **Partial** means
the feature exists with a documented behavior or fidelity gap. **Pending**
means the target behavior still needs implementation.

| Area | Status and current behavior | Code |
| --- | --- | --- |
| Steam reference runner | Working. Launches original CK2 through Steam. The recovered startup, lifecycle, and content models have separate tests. | `src/launcher/`, `tests/launcher-*.c` |
| Native launcher and content | Working. Fixed-size launcher, install discovery and path chooser, per-tab master toggles, persisted selections, enabled DLC ZIPs and folder or ZIP mods. Mods can override files and declare `replace_path`. Partial CK2 content compatibility. | `src/open_launcher/`, `src/game/assets.c` |
| Main menu and settings | Working. Native menu, era entry, credits, soundtrack, menu and ruler sounds, audio sliders, window modes, and content editor. Several settings are UI previews. | `src/game/main.c`, `settings.c`, `music.c` |
| Era selection and rules | Working. Five installed era cards, featured ruler data, bookmark switching, rules UI, option filtering, reset, and a local preset. Partial DLC gating and rule behavior. | `src/game/era.c`, `rules.c`, `ck2_script.c` |
| Dated history and setup | Partial. Dated holders and lieges, dynasty labels, title name resets, county and realm selection, liege/vassal navigation, and dated holding slots. A barony without a recorded holder has no generated ruler. | `src/game/realms.c`, `rulers.c`, `holdings.c`, `main.c` |
| Portraits | Partial. Installed sprite and portrait definitions, graphical culture and age/period fallbacks, DNA, historical properties, and rank frames. Scripted clothing and full dynasty arms remain pending. | `src/game/portraits.c`, `era.c`, `assets.c` |
| Map | Partial. Pannable terrain and Realms views, projected picking, province and sea-zone borders, dynastic names, geometry-based label placement, and CPU water shading with the installed cubemap. The political overlay reflects the start date. | `src/game/map_view.c`, `realms.c`, `campaign.c` |
| Gameplay | Prototype. The campaign has a clock, fixed treasury/income, province inspection, and holding upgrades. Continue resumes the current process's campaign. The York scenario is a smoke test. | `src/game/campaign.c`, `main.c`, `tests/campaign.c` |
| Platform support | Partial. Linux build and tests are the baseline. Windows and macOS builds and CI are pending. | `Makefile`, platform boundaries in `src/game/` and `src/open_launcher/` |
| Saves, simulation systems, multiplayer | Pending beyond the prototype. Full feudal simulation, save interoperability, multiplayer, and achievements are not implemented. | See milestones below |

## Ordered implementation plan

Start with **P1**. P2 and P3 can proceed in parallel once they use the same
stable world IDs and historical snapshot checks. P4 depends on a headless
simulation. P5 and later expand the verified playable slice. Each row pairs an
investigation with code to add and an exit check. Platform work can advance
alongside P1–P4 as the required boundaries become clear.

| ID and status | Investigate or decompile | Put in the codebase and verify |
| --- | --- | --- |
| **P0 Baselines — partial** | Record repeatable reference runs and resolve any mismatched reference hashes before reusing addresses. Confirm native launcher content precedence, Continue eligibility, and reproducible UI capture inputs. | Document a fixture manifest and a repeatable comparison command. Verify fresh builds and launcher/menu smoke runs in base-only and DLC-enabled profiles. Keep launcher-model and actual launcher checks distinct. |
| **P1 Historical world and hierarchy — partial** | Trace CK2's dated history application, same-date precedence, primary-title choice, liege traversal, capital changes, generated barons, de facto vs. de jure relationships, and DLC/replace-path precedence. Start with cases that disagree with the local data readers. | Build a reusable historical snapshot keyed by stable character, title, province, and holding IDs. Keep county owner, holding owner, primary title, and independent realm separate. Reuse it for setup, map, and campaign. Verify West Francia in 769 and France in 1066, Karl and a subordinate ruler, at least one barony, and Domažlice's dated capital and holding changes. |
| **P1.3 Engine architecture — pending** | Trace startup after launcher handoff, content initialization, bookmark loading, history application, world creation, and the first simulation tick. Identify callers, object ownership, ID mappings, and relevant layout fields. | Maintain the [architecture map](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-architecture-md). Use the recovered boundaries to evolve the existing C17 modules. Verify an ordered 769 initialization trace and cross-check it at 1066. |
| **Platform builds — pending** | Inventory POSIX, Linux desktop, Steam, timing, and filesystem assumptions. Verify behavior on Windows and macOS as each boundary is isolated. | Add a portable build and an automated Windows, Linux, and macOS build/test matrix. Keep a headless simulation target and isolate platform services. Verify compilation and synthetic tests on all three systems, plus launcher and local-asset smoke runs where CK2 files are available. |
| **P2 Frontend and character UI — partial** | Read `frontend.gui`, `frontend.gfx`, `province.gui`, `main.gui`, and portrait definitions. Trace only behaviors the definitions leave unresolved, including layout anchors, viewport scaling, tooltips, selection gestures, title presentation, and dynamically computed portrait properties. Compare Karl's bookmark and selected portraits under known DLC. | Reuse actual game sprites and fonts for the lobby, date controls, province/character panels, holding slots, government and religion icons, rank and arms, and interaction entries. Implement pointer and keyboard behavior, clipping and scaling, and portrait layer logic. Compare matching screenshots and click paths at 769 and 1066. Verify multiple graphical cultures, ranks, and DLC combinations. |
| **P3 Map and audio fidelity — partial** | Use the original shaders and [renderer evidence](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-map-rendering-md) to trace water inputs and pass order, coastline and river rendering, map modes, border geometry, label visibility and curvature, and zoom transitions. Use `interface/sound.sfx` plus reference playback for cue routing and ambient conditions. | Add the missing renderer passes, map-mode data, minimap navigation, label fitting and invalidation, and ambient or contextual audio. Verify terrain and Realms captures at the same coordinates and zoom. Check water over several frames, coastal/riverside pixels, labels on major and small realms, and click picking after pan/zoom. Separate visual differences from map-state errors. |
| **P4 Headless simulation, succession, and saves — pending** | Trace world creation, tick boundaries, update order, RNG use, death and succession, plus save container, IDs, state fields, and ordering. Use controlled snapshots to isolate same-day tie-breaking and save/reload behavior. | Add a deterministic simulation boundary independent of SDL. Wire a persistent world state, title transfer, basic landed inheritance, and read/write support for a pinned non-Ironman save. Verify a fixed death/succession sequence day by day and after reload. Test an OpenCK2 save in CK2 before claiming interoperability. |
| **P5 Script runtime, economy, and playable ruler — partial** | Trace trigger/effect scopes, event queues and on-actions, modifiers, game-rule resolution, taxation, levies, and construction timing. Start with one observed economy rule and one scripted event. | Add a bounded Clausewitz data parser/evaluator and implement verified scopes, triggers, effects, and rule consumers. Replace fixed economy logic with data-derived income, buildings, and levies. The campaign can inspect holdings and queue a subset of original building definitions. Verify a month's ledger, new settlement construction, building effects, an event outcome, and a game-rule change against the original. See [construction evidence](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-construction-md). Report unsupported script constructs explicitly. |
| **P6 Diplomacy, war, and AI — pending** | Trace eligibility and resolution for marriage, alliance, claims, factions, war declarations, movement, combat, sieges, peace, and AI scheduling. Isolate inputs and outcomes one subsystem at a time. | Implement one end-to-end path from a ruler interaction through war and peace, with AI actions and territorial changes. Verify eligibility, army movement and combat outcomes, the peace result, changed map labels, and save/reload of the resulting world. Then expand event, religious, and government systems. |
| **P7 Content breadth and services — pending** | Investigate DLC ownership/gating, DLC-specific mechanics, Workshop/dependency ordering, total-conversion assumptions, long-run AI, multiplayer synchronization, Ironman, and achievements. Use isolated profiles to distinguish installed assets from enabled features. | Expand compatibility by validated DLC/mod scenarios. Add supported services after stable saves and deterministic simulation. Verify paired base/DLC runs, conflicts and dependency ordering, long campaigns, and eventually deterministic replay or multiplayer synchronization. |
| **P8 Parity and release — pending** | Audit behavioral mismatches and unresolved reference assumptions. Measure visual, history, simulation, and save parity independently. | Maintain a fixture matrix across eras, culture packs, government types, settings, and save variants. Publish the measured coverage and open differences. Revisit executable-byte experiments only if explicitly resumed. |

## First tasks to pick up

- **P1.1 Add a repeatable local-history comparison.** Turn the 769/1066 France
  names, one county/realm ownership pair, and the dated holding changes into
  a runnable check against the local installation. Record reference values
  and identify what has only been checked in OpenCK2.
- **P1.2 Define the shared snapshot.** Identify what `realms.c`, `rulers.c`,
  `holdings.c`, and `campaign.c` already own. Move one read path onto shared
  dated data, then verify that map picking and the ruler panel resolve the
  same IDs without changing setup behavior.
- **P1.3 Trace bookmark-to-world initialization.** Start with 769 and verify
  the trace against 1066. Record loader calls, data ownership, the order
  of dated changes, and the handoff to the first tick in the architecture
  notes.
- **P2.1 Trace one portrait discrepancy.** Record Karl's portrait definition,
  DNA, properties, selected layer indices, DLC, and outfit at 769. Compare
  with another ruler and distinguish static layer selection from scripted
  changes. Implement the smallest verified rule.
- **P3.1 Capture a matched map pair.** Record a reproducible camera and zoom
  over western Europe with Terrain and Realms views. Pick one measurable
  difference such as shore colors, sea-zone borders, or label curvature,
  and verify the corresponding pass before changing the renderer.
- **P4.1 Choose a succession fixture once P1 is stable.** Record the CK2
  title owner, heir, vassals, and treasury before and after a controlled
  death. Preserve the fixture metadata and normalized state needed to
  implement and validate the headless transition.

## Contributor handoff

Use this entry when leaving a task for the next person or agent:

```text
Milestone/task:
Status: pending | investigating | partial | verified
Reference build/hash, date, ruler, DLC/mods, and settings:
Observed CK2 behavior and reproduction steps:
Evidence files or REA evidence ID, addresses, and provider:
Source files changed and normal entry point:
Verification commands and result:
Remaining difference or next smallest question:
```

Mark a result verified only for the scenario actually compared. Preserve
working launcher, menu, rules, and setup entry points while changing
underlying systems. When a milestone spans multiple sessions, leave the next
specific probe or failing check so another contributor can resume from it.
