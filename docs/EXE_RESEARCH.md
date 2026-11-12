# OpenCK2 — executable reverse-engineering evidence

> **Primary reference:** user-supplied `FreeCK2game.exe` (the historical CK2 3.3.5.1 / SOHY reference identified by prior notes). **Executable code is not decompiled source.** Each claim below is tagged **Verified binary**, **Verified loose file**, **Interpretation**, or **Open question**. Do not treat recovered strings, debug paths, inferred class fields, or proposed implementations as source-code equivalence.

## How to find information here

This is the **single maintained location for EXE-related research**. It contains a verified 2026-11-12 supplemental PE inspection, the earlier launcher and engine investigations, the complete recovered source-filename inventory, raw addresses, and the GUI/map disassembly roadmap. Research scripts, machine-readable cross-references, and instruction dumps remain in `../research/` (retained because they are evidence, not prose clutter).

- Start at **Executable metadata and dependency inventory** to verify you are using the same binary.
- Use **Verified string-to-function cross-references** for bloodlines, governments, tooltips and map modes.
- Use the **source note** sections lower down for original disassembly details and caveats; their original filenames and hashes are preserved for provenance.
- All addresses are preferred **VA**, not file offsets, except where explicitly marked. Preferred image base `0x140000000`; RVA = VA − base. ASLR changes loaded VAs.

## Executable metadata and dependency inventory — verified binary

| Field | Result |
|---|---|
| File | `FreeCK2game.exe` |
| SHA-256 | `a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d` |
| Exact size | 24,236,024 bytes |
| Format | PE32+, Windows x86-64 GUI, 10 sections |
| Preferred image base | `0x140000000` |
| Entry RVA / preferred VA | `0xDC4990` / `0x140DC4990` |
| Linker timestamp field | 2021-09-21 17:49:50 UTC **(metadata only, not proof of source revision or release date)** |
| `.text` | VA `0x140001000`, raw file offset `0x400`, approx 15.6 MiB of mapped instructions |
| `.rdata` | VA `0x140F9A000`, raw file offset `0xF98800`, contains recoverable strings |
| `.pdata` | VA `0x14185C000`, approx 47,410 plausibly valid unwind begin/end pairs from a fresh range scan; not a count of all semantic methods |
| Original matching `CK2game.pdb` | Path found in EXE; **file itself not supplied**. `tbb.pdb` in the DLL archive is for TBB, not CK2 game symbols. |

The binary imports the following DLLs as listed in its PE import table: `SDL2.dll`, `tbb.dll`, `WINMM.dll`, `WS2_32.dll`, `SHLWAPI.dll`, `d3d9.dll`, `d3dx9_41.dll`, `XINPUT1_3.dll`, `pops_api.dll`, `steam_api64.dll`, `physfs.dll`, `KERNEL32.dll`, `USER32.dll`, `SHELL32.dll`, `ole32.dll`, `ADVAPI32.dll`, `PSAPI.DLL`, `SDL2_mixer.dll`, and `PDXBrowser_IPC.dll`.

**Interpretation (bounded):** imports support that the executable links with SDL2, Direct3D 9, PhysFS, Steam and TBB, among other platform libraries. They do **not** prove gameplay algorithms reside in those DLLs or determine the exact renderer pass order. The companion DLL archive includes `tbb.pdb`, `tbbmalloc.pdb` and `tbbmalloc_proxy.pdb`, but no matching `CK2game.pdb`.

The development-path inventory lower in this document (also available as machine-readable `../research/source_filenames.csv`) contains 169 unique embedded paths: 167 `.cpp`, 1 `.h`, 1 `.pdb`. That supports original C++ compilation and indicates CK2-game, Clausewitz-library, and some third-party source origins. It is **not** a complete compilation-unit list and contains no original source body.

## Verified string-to-function cross-references — fresh recheck

A fresh independent scan located ASCII strings in `.rdata`, found selected direct x86-64 RIP-relative `LEA`/`MOV` references in `.text`, and mapped referring addresses to `.pdata` unwind bounds. This targeted pattern scan is **not** a complete disassembly: it can miss other instruction forms, indirect pointers, virtual calls, merged thunks, or references to the start of a larger string. The underlying targeted scan observations are preserved in `../research/pe_observations_20261112.json`. The dependency-free `../research/pe_probe.py` reproduces the simple RIP-relative reference scan from an executable path without installing a PE analysis library. The older Capstone-based cross-reference data under `../research/xrefs.json` is more complete for its selected terms.

### Content-path registration (verified binary)

The instructions at **`0x1404DDACF`** load the string `common/governments` (`.rdata` VA **`0x141036E18`**) into `rdx`, pass literal length **18** in `r8d`, and use destination `[rdi+0x628]` as `rcx`, before calling shared routine `0x1400B1240`.

At **`0x1404DDE08`** the same shared routine receives `common/bloodlines` (`.rdata` VA **`0x1410370D0`**), literal length **17** in `r8d`, destination `[rdi+0xA68]`. These are **high-confidence string/path-registration observations**, not proof of the file enumeration order, exact parser logic or runtime values stored in those member slots.

### GUI and map anchors (verified binary)

| String | `.rdata` VA | Concrete referencing instructions | Narrow conclusion |
|---|---|---|---|
| `bloodlines_window` | `0x141039450` | `0x1404FD11A` | A GUI binding reference exists; no click-handler guarantee |
| `vassal_entry` | `0x141039150` | `0x1404F6817` | Character-vassal list template lookup lead |
| `icon_topbar_societies` | `0x14105D450` | `0x14065FD1F`, `0x140660234`, `0x14066A667` | Multiple topbar routines reference the Society widget; visibility/layout conditions require caller tracing |
| `mapmode_governments` | `0x14105D7F8` | `0x140660C69`, `0x1406756BD`, `0x14067778F` | Map-mode GUI bindings exist; no government-coloring algorithm recovered |
| `CURRENT_WINTER` | `0x14107F330` | `0x1409170CC` | Referenced in province tooltip text assembly, not weather simulation |
| `PROVINCE_HOTKEY_INFO` | `0x14107F260` | `0x140917156` | Input-help text assembly identifier, not proof commands are wired |
| `TECHNOLOGY` | `0x1410341F0` | `0x14048CBB4` | Generic technology text key; does not by itself identify technology-research behavior |

### Original debug file anchors (verified binary)

- `bloodline.cpp`: full path begins VA `0x1410055C0`; referring instructions include `0x140181F69`, `0x140183580`, `0x140183744`. These fall in parser-related candidate unwind ranges documented below.
- `government.cpp`: full path begins VA `0x14106B4A0`; referring `0x1407AF084` in range `0x1407AEF70–0x1407B0F57`.
- `governmentflavor.cpp`: full path begins VA `0x14106BF58`; referring `0x1407B6A64`, `0x1407B6E0A`, `0x1407B6E35` in range `0x1407B69D0–0x1407B6F35`.
- `characterhistory.cpp`: path VA `0x14100B4E0`, reference `0x14028CD39`; `province.cpp`: path VA `0x14107ED10`, reference `0x14090DF2E`; `title.cpp`: path VA `0x14108E330`, reference `0x140A43B76`.
- `ConsoleCmdManager.cpp`: path VA `0x141027B50`, references `0x1403F0ABE`, `0x1403F0C20`; `ConsoleCmdImpl.cpp`: path VA `0x141022760`, references `0x1403D0670`, `0x1403D0DEF`. These are **useful diagnostic anchors**, not recovered function names or proof of all console commands.

**Caution:** an anchor in a function that reports a source filename is not necessarily the implementation of every method in that `.cpp`. Filename strings appear in assertion/error/logging sites. Do not rename whole function ranges based on the string alone.

### Earlier higher-level traces to retain (binary-supported, not all reverified in this pass)

- **Bloodlines parser:** original research identifies candidate `0x140181B40–0x140182090` with `inheritance` dispatch and a dword enum at `+0x45c`; token values `all_descendants=0`, `matrilineal=1`, `patrilineal=2`, `no_inheritance=3`. A second parser stores related enum values at `+0x158`. Constructor defaults and actual propagation remain open.
- **Governments parser:** candidate `0x1407AEF70–0x1407B0F57`; an AI aggression integer at `+0x193c` is checked against negative values, and multiple flags appear packed around `+0x196c–0x1970`. Offsets are build-specific observations, not OpenCK2 struct layout.
- **Government-flavor loader:** candidate `0x1407B69D0–0x1407B6F35`; an allocation of `0x20` bytes appears to be a collection wrapper, **not necessarily** one flavor record. Flavor selection/precedence is unknown.
- **Province tooltip builder:** candidate `0x140910520–0x1409178FC`, references terrain, climate, current winter, and modifier-hotkey text. Further trace owner, supply and climate producers before claiming numeric parity.
- **Map-mode binding routine:** candidate `0x140660310–0x1406610F9` binds 18 original GUI modes and five custom slots. The display semantics, DLC availability and palette logic are not recovered by the binding alone.
- **Launcher control flow:** older reference runs and disassembly trace `startgame`, `skiplauncher`, `nolauncher`, Continue file selection and DLC/mod settings. The detailed evidence and RVAs are preserved in the original launcher subsection below.

## Practical next EXE probes (not implemented)

1. Trace `0x140660310–0x1406610F9` callback hookups through actual map-mode render selection and color resolution. Separate mode visibility from selected mode.
2. Trace the `0x140910520–0x1409178FC` tooltip builder callers and data origin for holder/liege, terrain, climate, defense, and supply.
3. Trace the government parser fields through `set_government_type` and government `potential` evaluation; avoid defaulting to government by settlement type alone.
4. Trace caller graphs for bloodline parsers and births; explicitly verify paternal/maternal propagation, inversion and bastard exclusions before implementing exact inheritance claims.
5. Locate original technology level and research-point state transitions. Do not infer them from the `TECHNOLOGY` localization key or from `common/technology.txt` alone.
6. For console correctness, start with `ConsoleCmdManager.cpp` and `ConsoleCmdImpl.cpp` anchor ranges, then verify command registration/arguments against disassembly and examples. OpenCK2's own current console supports only a limited independent subset.
7. Preserve all initial Ghidra/REA observations with executable SHA, exact original path/identifier and method of confirmation. Update a finding as disproven, rather than deleting it.

## Preserved original research notes (source-identifiable archive)

Original notes were merged below, preserving the full text and original SHA-256 fingerprints. Historical status and repository paths may be stale; the updated overview above is the authoritative index. Links to previously separate Markdown notes have been redirected to their sections here. No speculative statement in an archived note becomes verified merely by inclusion.


<a id="source-docs-reverse-engineering-readme-md"></a>
## Source note: `docs/reverse-engineering/README.md`

> Preserved from the prior source archive. Original SHA-256: `97d0d09eab19c14f1065bfdb96c4fed54fe7c256590eb200db049e6811ab5953`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 reverse-engineering knowledge base
This index separates what has been learned about CK2 from what OpenCK2
currently implements. Start with the [roadmap](../ROADMAP.md) for task
order and use the [finding template](EXE_RESEARCH.md#source-docs-reverse-engineering-finding-template-md) for new evidence.
Existing detailed notes remain linked at their original locations.

#### Where to start
| Contributor | First useful step |
| --- | --- |
| Human new to C | Follow a roadmap scenario in original CK2 and OpenCK2. Record the date, ruler, settings, click sequence, and visible difference. Captures, historical checks, and reproduction notes are useful contributions. |
| Human who works in C | Read the relevant source and evidence, choose a bounded behavior, and add the implementation and a focused comparison or regression check. |
| Local AI agent | Give it this index, [AGENTS.md](../AGENTS.md), one roadmap task ID, and the relevant note or source files. If it lacks access to the local game or REA, have it mark the missing checks and leave commands for a contributor who can run them. |
| Codex | Start with [AGENTS.md](../AGENTS.md), then use the workspace and local tools to reproduce the task's reference scenario and verify the resulting change. |
| Claude Code | Start with [CLAUDE.md](../CLAUDE.md), which points to the same shared instructions. Give it one roadmap task and its evidence links. |

#### Reference and evidence conventions
The pinned Windows Steam reference is CK2 3.3.5.1 (SOHY), app 203770,
build 7399341. The executable is `CK2game.exe`, x86-64 PE, with preferred
image base `0x140000000` and SHA-256
`a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d`.
Check the hash and image base before reusing an address. Earlier work
used REA 6.8.0 and Ghidra 12.1.4. Use RVAs or clearly marked VAs, with
the source build and analysis profile alongside them.

Use one or more labels on each finding, according to its evidence:

- **Observed** means reproduced in the original game or directly read from
  a named installed definition. Say which of those applies.
- **Binary-supported** means backed by disassembly, decompilation, or a
  traced data reference. State exactly what the evidence establishes.
- **Inference** means the available evidence supports an interpretation.
  Explain the reasoning and how to distinguish alternatives.
- **Hypothesis** means an unverified explanation or proposed probe.

Record OpenCK2 verification separately. Use the same distinction when
describing recovered structures, field offsets, or relationships.
Decompiled pseudocode is analysis output. Use it to explain the evidence
and implement verified behavior in the C17 project.

#### How REA was used
Earlier work imported the local Windows executable through the Ghidra
provider, used string searches to locate startup and frontend anchors, and
traced callers, comparisons, result fields, and the Continue save-path
handoff. The startup branch was checked against disassembly and a local
Steam/Proton run. A separate decompilation identified terrain inputs;
installed shader files explained how the original renderer uses them.
Later searches located anchors for eras, game rules, portraits, and sound.
Those string findings guide further investigations into call order and
runtime behavior.

The [roadmap REA workflow](../ROADMAP.md#rea-workflow) records repeatable
commands and timeout guidance. The notes below retain evidence IDs,
addresses, and the limits of each investigation.

#### Subsystem index
| Area | Existing evidence | Next question |
| --- | --- | --- |
| Startup, launcher, and content | [Launcher decompilation](EXE_RESEARCH.md#source-docs-reverse-engineering-launcher-decomp-md) documents bypass decisions, the launcher loop, selection writeback, and the Continue handoff. | Native mount precedence, dependencies, initialization after the handoff, and save eligibility. |
| Architecture and world initialization | [Architecture map](EXE_RESEARCH.md#source-docs-reverse-engineering-architecture-md) tracks known subsystem boundaries and open questions. | Trace the 769 bookmark from selection through history application to the first playable state. |
| History, titles, holdings, frontend, portraits, rules, and sound | [Starting eras](EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md) records installed definitions, title resets, ruler selection, portrait inputs, and REA string anchors. | Separate history application, world creation, and first-tick effects. Trace scripted portrait properties and rule consumers. |
| Holding construction | [Construction evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-construction-md) covers original building and settlement data, observed prices, and binary references. | Recover modifier sources, eligibility, title assignment, and construction effects. |
| Map, rendering, and UI | [Map rendering](EXE_RESEARCH.md#source-docs-reverse-engineering-map-rendering-md) records a decompiled terrain function and installed shader inputs. [Starting eras](EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md) maps frontend art and UI definitions. | Recover pass order, projection, label layout, and representative UI input handling. |
| Scripting and events | Installed data readers exist in OpenCK2. General runtime execution has no verified trace in the current notes. | Follow one trigger and effect through scope resolution and the event queue. |
| Tick loop, economy, succession, diplomacy, combat, and AI | OpenCK2 has a prototype clock and economy. The original systems' algorithms remain open. | Recover the scheduler and isolate one daily transition before expanding to coupled systems. |
| Saves and determinism | The launcher notes document Continue's filename selection and handoff. World serialization and deserialization remain open. | Trace a plain-text non-Ironman save load and map stable world IDs to their records. |

For a new subsystem, add a focused Markdown file here and link it from this
index and the roadmap. Keep observations, addresses, concise traces, and
repeatable commands in Git. Keep tool projects and raw dumps in local scratch
space. When a later investigation changes a conclusion, link the new evidence
and explain which earlier interpretation it supersedes.

---

<a id="source-docs-reverse-engineering-architecture-md"></a>
## Source note: `docs/reverse-engineering/ARCHITECTURE.md`

> Preserved from the prior source archive. Original SHA-256: `da9d3ceaf5a89a9f19f4af90d42f2fe17ef9c44725b6d9068d694c2c2604deb3`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### CK2 architecture investigation
This map records evidence about original CK2 and identifies the current
OpenCK2 source boundaries. Treat the open entries as investigation tasks.
Use the pinned build and confidence conventions in the [knowledge base](EXE_RESEARCH.md#source-docs-reverse-engineering-readme-md).

| Boundary | Original CK2 evidence | OpenCK2 code |
| --- | --- | --- |
| Startup to launcher | Binary-supported startup branch, event-loop result, and Continue filename handoff. [Launcher evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-launcher-decomp-md) | `src/launcher/` models the recovered behavior. `src/open_launcher/` is the native launcher and content resolver. |
| Content to data loaders | Installed descriptors and observed launcher selections support DLC and mod discovery. General CK2 load order and object ownership remain open. | `src/open_launcher/asset_paths.c`, `zip_assets.c`, and `src/game/ck2_script.c` |
| Bookmark to initial world | Installed bookmark and history records define inputs. Executable string anchors identify relevant screens and directories. Initialization call order and live object lifetimes remain open. [Starting-era evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md) | `src/game/era.c`, `realms.c`, `rulers.c`, `holdings.c`, and `campaign.c` |
| World to renderer and UI | Installed interface and shader definitions, with a binary-supported terrain-function trace. The full pass order and layout engine remain open. [Renderer evidence](EXE_RESEARCH.md#source-docs-reverse-engineering-map-rendering-md) | `src/game/map_view.c`, `realms.c`, `portraits.c`, and `main.c` |
| Day advancement and save/load | The original simulation scheduler, ownership graph, RNG streams, and world serialization have no verified reconstruction in these notes. | `src/game/campaign.c` has a prototype daily clock and economy. |

#### Next architecture probe
Use 1 January 769 and record the DLC profile, ruler, title, and county. Follow
one bookmark-selection action to initial world construction. Identify callers,
the order in which dated character, title, and province records are applied,
where stable IDs become runtime objects, and where initialization hands off to
the first simulation tick. Check the same hypothesis against a 1066 start.
Leave an ordered trace with addresses and confidence levels, and record one
specific unresolved branch. This is roadmap task **P1.3**.

#### Code boundaries to establish as evidence accumulates
Keep historical data and deterministic simulation callable without SDL.
Separate platform paths, directory walking, archive and process services,
Steam discovery, windowing, and timing from simulation. Confirm these
boundaries through narrow changes to the existing C17 code and matching
scenario checks before moving more systems.

---

<a id="source-docs-reverse-engineering-launcher-decomp-md"></a>
## Source note: `docs/reverse-engineering/LAUNCHER-DECOMP.md`

> Preserved from the prior source archive. Original SHA-256: `f172db01978556fa133e53d4be25e6ac856d5d0dc6de561a0a477db350f25642`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### CK2 launcher decompilation
#### Milestones
1. **Startup decision — complete.** Locate the launcher entry path and reconstruct the bypass-token comparisons and branch predicate in C. Check all branch inputs and confirm one bypass flag on the original build.
2. **Launcher lifecycle — complete for the traced handoff.** Recover the setup and event-loop calls. Identify what Quit, Play and Continue return to startup. Verify the result codes and the Continue save-path handoff against the original.
3. **DLC and Mods — complete for installed, selectable content.** Recover where the two lists come from, how selections are persisted, and how Play applies them. Use local fixtures and compare the resulting launch state.
4. **Parity checks.** Byte matching is on hold. Compare the independent launcher with the original screen and scenarios. For machine-code matching, choose a small original function with clear boundaries and test a matching Windows toolchain before expanding to larger units.

#### Milestone 1 evidence
The reference is Windows Steam build 7399341, CK2 3.3.5.1, x86-64 PE.
The executable SHA-256 is `a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d`.
REA 6.8.0 with Ghidra 12.1.4 completed its image inventory on this executable.
REA reports a preferred image base of `0x140000000`. Addresses below are RVAs relative to that base. The branch trace was checked against local GNU objdump disassembly.

| RVA | Evidence |
| --- | --- |
| `0x80e4f0–0x8111d8` | PE unwind table entry enclosing the startup code. Its upper bound is exclusive. |
| `0x80f6e9`, `0x80f708`, `0x80f727` | References to `startgame`, `skiplauncher` and `nolauncher`. Each calls the same case-sensitive byte-string comparison at `0xe8ca40`. A zero result reaches the write of 1 to `[rbp+0x540]` at `0x80f737`. |
| `0x810441` | Calls `0x80d950`. Its return byte is still in `al` when the branch checks it. |
| `0x81044c–0x810466` | Skips launcher setup at `0x810785` only when `[rbp+0x540]` is nonzero, the global byte at RVA `0x1666cf9` is zero, and `al` is zero. |
| `0x81052f`, `0x8105d1` | Loads the `launcher/interface` string and passes it into the setup call at RVA `0xd76310`. |

The recovered C is in [startup.c](../src/launcher/startup.c). The token function accepts the internal token, so the tested external argument `-skiplauncher` and the compared internal string `skiplauncher` are different inputs. The two override names describe their effect at the branch. Their producers and the rest of command-line parsing remain outside this milestone. Both C functions model blocks within a larger original function, so there is no standalone original function to byte-match against them.

From the repository root:

```sh
make launcher check-launcher-startup CFLAGS='-O2 -Werror'
```

The test covers all eight input combinations and exact token matching. In a live Steam/Proton run, `steam -applaunch 203770 -skiplauncher` passed the flag to the pinned executable and reached the game’s main menu. An earlier launch without that flag showed the pre-game screen. The other two tokens were verified by disassembly. The C logic is a separate module from the Steam reference runner, which still opens the original.

#### Milestone 2 evidence
| RVA | Evidence |
| --- | --- |
| `0xd76310`, `0xd76850`, `0xd76db0` | Startup calls launcher setup with the interface path, supplies the saved selection lists, and passes `launcher/motd.txt` before entering the loop. |
| `0xd7877e`, `0xd787b7`, `0xd787f0` | The launcher object installs callbacks at `0xd7e1a0`, `0xd7e310`, and `0xd7e490`. |
| `0xd7965b`, `0xd796b7`, `0xd79713` | Binds those callbacks to `play_button`, `continue_button`, and `cancel_button`, respectively. |
| `0xd7e1be`, `0xd7e32e`, `0xd7e490` | Each callback sets the loop exit flag at object offset `0x88` and stores its result at offset `0x30`. Play stores 0, Continue stores 1, and Quit stores 2. |
| `0xd76f90` | REA/Ghidra decompilation confirms the loop processes events, updates the launcher, and calls `SDL_Delay(1)` while waiting for its approximately 60 Hz update interval. It returns the result field when the loop exits. |
| `0x810725–0x810785` | After selection writeback, result 2 exits via cleanup. Result 1 calls the Continue helper at `0x80da80` and copies its returned filename into a global consumed during game startup. Result 0 falls through to normal startup. |
| `0x9ddba0`, `0x9de00c`, `0x9de0b2` | The Continue helper delegates to a save search that enumerates `.ck2` files, reads modification times via `PHYSFS_getLastModTime`, and replaces its current eligible candidate only when the new timestamp is greater. At `0x711cc8`, startup passes the filename and associated metadata to the game’s load path. |

In an isolated Proton prefix, two copied saves were renamed and given different
modification times. Reversing their timestamps changed the Continue label from
`CodexNewer` to `CodexOlder`, with the filenames and save contents unchanged.
Clicking Continue entered game initialization. Both candidates were opened
during the run, so file-open events alone do not establish which save loaded.
The selected-filename handoff is established by the branch and call trace.
The isolated run did not verify arrival in a loaded campaign.

[lifecycle.c](../src/launcher/lifecycle.c) models the action dispatch and timestamp
selection after the game's eligibility checks.

#### Milestone 3 evidence
The original enumerates `.dlc` and `.mod` descriptors through the same listing
helper at RVA `0xd64fb0`, with call sites at `0xc75175` and `0xc75612`.
Holy Fury’s installed `dlc/dlc074.dlc` points to `dlc/dlc074.zip`.
A minimal `mod/codex_probe.mod` placed in the isolated profile's mod directory
appeared in the Mods tab. Steam Workshop and duplicate-descriptor precedence
were outside this test.

| RVA | Evidence |
| --- | --- |
| `0x8d0ea`, `0x8d102` | Registers the settings keys `last_dlcs` and `last_mods`. |
| `0x81060d–0x81061a` | Passes the settings vectors at offsets `0x2a8` and `0x2c0` into launcher setup. |
| `0x8106b3–0x810720` | Copies launcher selections back into the settings vectors and invokes settings serialization before checking the launcher result. |
| `0x9e9af5`, `0x9e9d10` | Serializes nonempty lists. The mod writer constructs quoted strings. |

The isolated profile produced the following after disabling Holy Fury,
enabling the probe mod, and clicking Quit:

```text
last_dlcs=
{
    dlc/dlc074.dlc
}
last_mods=
{
    "mod/codex_probe.mod"
}
```

`last_dlcs` records **disabled DLC descriptors**. `last_mods` records **enabled
mod descriptors**. On reopen, Holy Fury stayed unchecked and the probe stayed
checked. File-open monitoring started at the launcher immediately before each
button click:

| Action and selections | Holy Fury archive | Jade Dragon archive | Probe mod script |
| --- | --- | --- | --- |
| Continue, Holy Fury off and probe on | No open observed | Opened | Opened |
| Play, Holy Fury on and probe off | Opened | Opened | No open observed |

After the Play test, both settings blocks were absent, consistent with the
serializer skipping empty lists. [content.c](../src/launcher/content.c) models
selection restoration for already discovered, selectable descriptors. Ownership
checks, Workshop discovery, the general settings parser, and the full mount
ordering remain outside this C model.

Run the combined tests with:

```sh
make launcher check-launcher CFLAGS='-O2 -Werror'
```

---

<a id="source-docs-reverse-engineering-bookmarks-and-cursors-md"></a>
## Source note: `docs/reverse-engineering/BOOKMARKS-AND-CURSORS.md`

> Preserved from the prior source archive. Original SHA-256: `3d65255af92e809ce1f1ad6c6821beeb615f67c2eb6cb23b15c0a68d8511dc2c`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Custom Game Setup bookmarks and game cursors
Evidence from Steam CK2 3.3.5.1 and the OpenCK2 implementation.

#### Bookmarks and scrollbar
`common/bookmarks/00_bookmarks.txt` has 11 dated records. Five carry
`era = yes` and supply the large era cards. The other six are additional
dates for the setup list:

| Bookmark | Date |
| --- | --- |
| Norman Conquest | 26 December 1066 |
| Komnenos Dynasty | 1 April 1081 |
| Third Crusade | 1 January 1187 |
| The Latin Empire | 16 May 1204 |
| Age of the Mongols | 1 February 1220 |
| Rise of the Hansa | 1 May 1241 |

The screenshot shows the final four rows of the list, with Late Middle
Ages (1 January 1337) following the last three extra dates. The
`bookmarks_list` in `interface/frontend.gui` is 168 logical pixels
tall, and `bookmark_entry` is 42 pixels high. Four rows fit.
The list requests `standardlistbox_slider` from
`interface/EU3_core.gui`. The Interesting Characters list has its own
scroll position. The screenshot is consistent with scrolling dates
while the selected character list stays on Otto's bookmark.

OpenCK2 reads all 11 records for Custom Game Setup and keeps the
five era-tagged cards in the era picker. Setup uses the same CK2
scrollbar art as the vassal list. Wheel, arrows, track clicks, and
thumb dragging change the list offset independently of selection.
Selecting a bookmark reloads the historical snapshot and clears the
previous ruler selection. Leaving setup on a bookmark without an
era card returns to the card picker.

Native DLC-dependent bookmark availability and exact scroll behavior
remain to be verified against matching profiles.

#### Cursor
The installed game ships pointer artwork in `gfx/cursors/`.
`normal.png` is a 32 × 32 RGBA image. The decoded `normal.cur` has
identical RGBA pixels and a hotspot of (0, 0). The pinned Windows
executable contains `/cursors/normal.cur` at VA `0x14101f118`
and references it at `0x1403857d2`. It also references busy and
drag-select cursor assets and imports `LoadCursorFromFileA`,
`SetCursor`, and `SDL_SetCursor`. This establishes a game-supplied
cursor path. Exact state transitions and native platform-specific
selection remain unverified. OpenCK2 loads the installed `normal.png` into an SDL color cursor
with the original (0, 0) hotspot. The default cursor remains available
if loading or cursor creation fails and is restored on window leave,
focus loss, and shutdown.

Busy, drag-select, and movement cursors remain pending. Observe
vanilla's state transitions before adding them. Animated `.ani`
decoding and timing are separate from the static cursor.

#### Verification
On Linux, dragging the bookmark thumb to the bottom exposed The Latin
Empire, Age of the Mongols, Rise of the Hansa, and Late Middle Ages.
Selecting The Latin Empire changed the date to 16 May 1204, populated
its featured characters from dated history, refreshed the description
and Realms map, and opened Game Rules through Play. The X11 cursor
query matched the installed cursor's 365 opaque pixels, 32 × 32
dimensions, and (0, 0) hotspot. High-DPI and other platform backends
remain unverified.

---

<a id="source-docs-reverse-engineering-starting-eras-md"></a>
## Source note: `docs/reverse-engineering/STARTING-ERAS.md`

> Preserved from the prior source archive. Original SHA-256: `93ec1cde1d298a2c8012d8e922435ff8d57c146b10bc3805b1643558e9493c3f`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Starting eras and gameplay interface
#### Starting era screen
The reference installation is the local Windows Steam build of Crusader
Kings II. OpenCK2 reads its assets directly from the configured game directory.

| Installed source | What OpenCK2 uses |
| --- | --- |
| `interface/frontend.gui` and `frontend.gfx` | Reference layout and sprite names for the era screen and ruler entries. |
| `gfx/interface/pick_era_*.dds` | Original panel, cards, biography plate, portrait name banners, and selection highlight. |
| `common/bookmarks/00_bookmarks.txt` | Era dates, picture IDs, ruler IDs, and the bookmark's portrait DNA and properties. |
| `localisation/*.csv` | English era and ruler names, era descriptions, and biographies. |
| `history/characters/*.txt` and `history/titles/*.txt` | Birth records and title holders at the bookmark date for featured ruler details. |
| `interface/portraits/portraits.gfx` and `portrait_sprites.gfx` | Reference for layering base-game portrait sprite sheets. The sheets use BC3/DXT5 DDS. |
| `gfx/flags/*.tga` and shield masks and frames | Title flag artwork with an installed shield shape and frame. |

The portrait renderer indexes the installed portrait and sprite definitions
from the base game and enabled DLC and mods. It tries the ruler's ordered
`graphical_cultures` entries and chooses early or late layers using CK2's
date thresholds. In 769, `old_frankish` prefers the early German definitions
from the enabled German portrait DLC, with Holy Fury's Frankish art and
the Western base-game portrait as fallbacks. The renderer also uses the
installed age variants, frame counts, offsets, hair and eye palettes, and
rank frames. CK2's full dynamic clothing rules and dynasty arms remain
incomplete.

#### Game Rules and Realms
The rules UI reads `common/game_rules/00_game_rules.txt`, its localized
labels and descriptions, and the interface art referenced by
`interface/game_rules.gfx`. The local installation has 73 rules. The first
listed option is each rule's default. The UI supports changing options,
scrolling, filtering by category, resetting defaults, and saving and
loading one OpenCK2 preset. DLC requirements are shown as labels. DLC
gating, Ironman, achievements, and the simulation effects of the rules
remain pending.

Realms uses the selected bookmark date to resolve county titles, dated
holders and lieges, and the highest title held by an independent ruler.
The overlay uses the installed landed-title RGB colors and draws borders
on the terrain. Its labels use the installed Mapfont and date-specific
title names, including Wessex and West Francia in 769. The overlay represents
the selected start date. It does not simulate territorial change.

#### REA findings
REA 6.8.0 inspected the local executable's PE resource table. Evidence ID
`ev_92653533e219637f0add090bcc669495649599c93eecc5eb6b86d8e33753f78b`
covers the executable's embedded Windows resources. The gameplay interface
art in the table above comes from loose game files.

REA's Ghidra string search found these references in the same executable
under evidence ID
`ev_cd905a52e1fc24b65c60823067f852cce2d068a6336d878b93c73f6288b19950`.

| Address | String |
| --- | --- |
| `0x141036b58`, `0x141036b98` | `history/titles`, `history/characters` |
| `0x141036da8` | `common/bookmarks` |
| `0x1410696f0`, `0x141069850` | `pick_era_screen`, `era_character_entry` |
| `0x1410965f8`, `0x1410a09e8` | `portraitType`, `selectable_character` |

The first Ghidra analysis timed out. The retry used a reduced analyzer
profile and returned the string evidence. The native loader's call order
and DLC resolution remain unverified.

A subsequent REA search returned evidence
`ev_c15c036fea02e12281ecc8b4ace6bab5aef12111f936de2dd394cea94e605fcd`
for `common/game_rules` at `0x141036f50`, `mapmode_realms` at `0x14105d488`,
`interface/game_rules.gui` at `0x141067448`, and `game_rules_window` at
`0x1410674c0`. These are executable string references. The installed data
provides the rule inventory and historical title values.

#### In-game interface reference
The local `interface/main.gui` and `main.gfx` define the ruler portrait and
navigation at upper left, the resources and time controls at upper right,
and the minimap and map modes at lower right. `outliner.gui` defines the
expandable outliner. `province.gui` defines province selection and the
holdings panel. OpenCK2 now uses the installed sprites for the ruler and
resource bars, date control, outliner, province panel, and minimap frame
and map. The date control pauses and resumes time. The minimap is visual
only, and the ruler navigation tabs show the unfinished-feature tooltip.
The York scenario's controls and goal remain specific to the temporary test.


#### Ruler selection and sound
Custom Game Setup shows featured rulers and resolves the county holder
when you click a province. Shift-click opens the independent realm. Dated character history supplies culture,
religion, and dynasty. A realm takes the ruler's dynasty name when the
culture enables `dynasty_title_names` and the title permits it. This
produces the Abbasid and Umayyad labels at the 769 start. Rulers with
available portrait DNA use installed portrait layers.

REA evidence
`ev_96b8cb2b7f983421371c97c11b268e165cb0142564ef0463c38fd4639d578f73`
identifies `interface/sound.sfx` and `dynasty_title_names` in the Windows
executable. The local sound definitions map the `choose_country_1` through
`choose_country_10` cues to the ruler selection clips. OpenCK2 loads those
clips and the menu, bookmark, and other frontend cues from the installation
and uses the configured effect volume for playback.


#### Historical title and holding selection
Setup uses the installed lobby panels, date and bookmark art, and map mode
buttons. Click a county to inspect its historical holder. Alt-click opens
that holder's primary title, and Shift-click opens the independent realm.
The right panel's liege, vassal rows, and occupied holding slots are
selectable. Holding types, slot counts, capital moves, and removals are
read from dated province history. A holding with no recorded historical
holder has no generated ruler in this implementation.

`history/titles/k_france.txt` sets `WEST_FRANCIA` at the early dates and
resets the title name in 987.7.3. The title loader now honors dated
`reset_name = yes` and resolves the default `k_france` localization.
The installed data produces West Francia in 769 and France in 1066.

The selected portrait uses dated character data when available. Karl's
769 character history supplies `aj00bb00000`, while the era bookmark
thumbnail supplies `aj00bc00000`. Both use the installed culture and era
portrait definitions. The original engine's scripted portrait property
selection remains pending.

---

<a id="source-docs-reverse-engineering-map-rendering-md"></a>
## Source note: `docs/reverse-engineering/MAP-RENDERING.md`

> Preserved from the prior source archive. Original SHA-256: `ae3c178f8906274ed9f6de8c9ddc107c9d527761cd70b9cab3da61d5ae497a2f`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### CK2 terrain renderer investigation
#### Reference build
REA 6.8.0 with Ghidra 12.1.4 decompiled the local Windows Steam executable
at VA `0x1408e2100` (RVA `0x8e2100`). The image SHA-256 is
`a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d`.
The REA evidence ID is
`ev_59e7a0f32012a13320a726c79ea2056b690d7adcb4c26feae863ef2e780edbdc`.

The function references the topology bitmap, world normals, terrain atlas,
land and water color maps, water normals, tree tint, river textures, strait
texture, and reflection texture. The original installed data explains their
roles:

| Installed file | Finding |
| --- | --- |
| `common/defines.lua` | The frontend camera ends at (790, 500, 600) and looks at (790, 0, 1000). |
| `gfx/FX/pdxmap.lua` | The terrain shader samples a four-by-four repeating tile atlas, applies the terrain color overlay, and combines terrain and height normals for lighting. |
| `gfx/FX/pdxwater.lua` | The water pass uses moving normal/noise samples, water tint, shoreline height, reflection, and refraction. |
| `gfx/FX/tree.lua`, `map/default.map` | Trees render in their own pass and use tint and seasonal data. The map points to a separate tree definition bitmap. |
| `gfx/FX/river.lua` | The river pass shades a textured channel. |

#### OpenCK2 implementation
`src/game/map_view.c` projects the CK2 heightfield with a perspective camera.
The menu uses the original final camera coordinates and a tuned field of
view. The campaign camera is wider and farther north so York remains easy
to select. The terrain pass reads the original BC3 atlas, indexed terrain,
color map, and world normals. Water is animated using two moving samples of CK2's noise texture and
its water-normal detail. The CPU pass reads the installed underwater
terrain and reflection cubemap, uses the shader's 93.7 shoreline threshold,
and computes a view-dependent Fresnel reflection. The river bitmap supplies the river mask. Forest
crowns are scattered in the original tree cells with the installed pine
diffuse texture and tree tint.

The map records the source pixel for each projected screen pixel. Province
hit testing and the selection outline use those same samples, so water
animation and perspective do not shift province selection.

The original GPU renderer, camera motion, individual tree meshes, screen-space water
refraction, full river geometry, straits, and seasonal
effects remain outside this implementation. The current comparison is
visual, with no pixel-identical claim.


Realms mode draws borders using both the historical realm assignment and
the underlying province map. Labels use Mapfont and derive their size,
angle, and position from visible territory pixels. Collision placement
keeps prominent labels visible where space permits. CK2's curved-label
layout and dynamic changes from conquest remain pending.

#### Iceland and the northwest edge
**Installed data and OpenCK2 reproduction.** The local 3072 × 2048
`provinces.bmp` contains all four Iceland counties. Join the RGB values in
`map/definition.csv` for IDs 1, 2, 1616, and 1617 to the corresponding
pixels in `provinces.bmp`, then compare the same coordinates in
`topology.bmp` and `rivers.bmp`. Count a point as above the waterline
when its topology value is at least 94. This gives the following counts
on the local Steam install:

| County ID | Above-water pixels | Of those, river index 254 |
| --- | ---: | ---: |
| 1 (Vestisland) | 4,519 | 4,519 |
| 2 (Austisland) | 5,708 | 5,708 |
| 1616 (Nordland) | 6,309 | 6,309 |
| 1617 (Sudurland) | 5,513 | 2,701 |

OpenCK2 interpreted river index 254 as ocean, discarded the terrain
elevation there, and shaded those coordinates as water. The Realms pass
still read the county IDs from the province bitmap. The resulting
combination explains the flat, tinted land and the visible change in
shading across southern Iceland. The terrain pass now uses the topology
waterline for elevation and surface classification, with river indices
used for channel coloring. This is a CPU approximation of the
terrain/water split in `pdxmap.lua` and `pdxwater.lua`.

The angled cutoff beside Iceland is the projected northwest edge of the
map. OpenCK2 previously filled everything beyond it with a flat blue-gray
color. `interface/mapitems.gfx` declares the installed `frame3072.xac`
actor. The map now resolves that actor by name and draws its original
528 triangles and 1024 × 256 BC3 diffuse texture with the map camera.
Perspective-correct UVs preserve the wood pattern on the bevel and inner
wall. The depth test keeps the frame in front of the water where they
overlap and removes those pixels from province selection. The former
flat border serves as a fallback when the local model or its texture
cannot be loaded.

The current frame transform is inferred from the model coordinates and
the 3072 × 2048 map. It uses approximately 100 map units per model unit
and places the base of the inner wall at water level. Shading uses the
mesh normals and diffuse texture. The original normal and specular maps
and the precise native transform are still pending. A bounded REA/Ghidra
search for `frame3072` timed out in the earlier investigation. The next
native probe is to trace the actor's render submission and material setup.

At the 769 bookmark all four county histories name the same holder.
OpenCK2 currently labels equal-rank county titles separately. The vanilla
reference shows a prominent Vestisland label at close zoom. The native
primary-title and label selection logic still needs investigation.

#### Setup toolbar and image quality
**Installed interface definitions.** `interface/frontend.gui` supplies
the position and order of the eight setup map buttons.
`interface/alternate_start.gfx` resolves the large Shattered World and
Random World buttons and ornaments to the installed DDS art. Terrain and
Realms are interactive. The other six modes and the two world buttons
show the shared red “Not developed yet” tooltip on hover.

**Renderer comparison.** The artwork is read from the local CK2
installation. The visual differences come from how it is sampled and
composited:

- The main menu, setup and campaign maps are rendered into a fixed
  1600 × 900 CPU raster. SDL scales it to the window, so a 1920 × 1080
  or larger output still starts from the same raster. The interface
  also uses fixed logical coordinates and bitmap fonts.
- The former water pass evaluated all animated noise at whole map
  pixels. Zoom magnified each sample into a visible block. The current
  pass retains the fractional world hit, and bilinearly samples
  repeating water noise, water normals, underwater detail, and terrain
  atlas tiles. Province IDs and terrain type IDs still use integer
  lookup.
- Vanilla's installed `pdxmap.lua` and `pdxwater.lua` specify linear
  sampling and mip filtering for many layers. OpenCK2 still uses a
  CPU heightfield, fixed-resolution reflection faces, simple river
  coloring, pixel-difference province outlines, and approximate
  lighting. Zoomed-out detail can still alias, and smooth continuous
  borders and refraction remain missing.

For a repeatable visual check, open the 769 bookmark through
Single Player → New Game → Custom Game Setup and pan northwest in
Realms mode. Check the four Iceland counties, coast, and wooden corner,
then hover over a world button and a deferred map mode. For a rendering
comparison, capture Iceland and Northumbria at matching camera coverage
and output size. Compare the shoreline, province borders, water detail,
and UI independently before changing mip selection or render resolution.

The local OpenCK2 run showed all four Iceland counties with relief, the
wooden northwest corner, and the red tooltips over Shattered World and
a deferred map mode. The comparison uses the supplied vanilla
screenshots and the local CK2 files.

#### Interface model loading
`src/game/models.c` looks up model names inside `objectTypes` in
top-level `interface/*.gfx` files. It reads `EMFXActorType` and
`pdxmesh` declarations through the existing asset resolver so mounted
content participates in path lookup. The loader imports positions,
normals, primary UVs, and triangle indices from little-endian XAC
and Clausewitz binary mesh files. It also reads the declared scale and
the first diffuse texture reference. The format reference for the
Clausewitz binary properties is the
[IO PDX Mesh reader](https://github.com/ross-g/io_pdx_mesh/blob/master/pdx_data.py).

A local sweep extracted unique model names from the shipped `.gfx`
declarations and loaded each by name against the base Steam installation.
All 314 XAC actor names and all 56 PDX mesh names in the sweep returned
static geometry. Separate runtime checks panned to Iceland and zoomed
in to verify the frame's corner and visible inner wall. The sweep covers
base model geometry. It does not exercise installed DLC or mod variants,
texture compatibility for every model, or the appearance of other actors
in game.

Next steps for interface compatibility:

1. Preserve material bindings per mesh part, then support normal and
   specular textures, shader selection, and level-of-detail switching.
2. Recover skeletons, skin weights, XSM and Clausewitz animations, actor
   attachments, and scene placement. Verify one unit, settlement, and
   shield against vanilla before broadening rendering support.
3. Extend interface coverage beyond models. Audit the existing sprite,
   portrait, font, and GUI loaders against shipped declarations and
   implement missing widget layout, interaction, and engine bindings.

#### Current map checkpoint
The main menu and setup/campaign views consume local CK2 terrain,
topology, normal, river, tree, water and reflection assets. Setup
uses the same projected samples for province picking, selection
outlines and the Realms overlay. Historical titles and holders drive
realm colors and labels. Iceland's height-based shoreline fix and
the original 3D wooden frame are in place. The loader decoded
static geometry for 370 base model names. The frame is the model
verified in the running map.

The history resolver now chooses one province history file for both
Realms and holdings. The [Călărași audit](EXE_RESEARCH.md#source-docs-reverse-engineering-province-holdings-md) records
the Coconi fix and the scan of 1,740 provinces at all 11 bookmarks.
The [portrait and ruler panel notes](EXE_RESEARCH.md#source-docs-reverse-engineering-portraits-and-ruler-panel-md)
record the authored portrait fix, generated fallback, and paused
vanilla portrait parity work.

Remaining map parity includes native GPU lighting and filtering,
water refraction, full river and strait geometry, the exact frame
transform and materials, curved realm labels, and live realm updates.
The software map still renders to a fixed logical raster. Match the
reference camera, zoom, mode and resolution before evaluating further
changes.

---

<a id="source-docs-reverse-engineering-province-holdings-md"></a>
## Source note: `docs/reverse-engineering/PROVINCE-HOLDINGS.md`

> Preserved from the prior source archive. Original SHA-256: `84d48953b5f451a31c1ab02ae20282de050ca389fc0fcfb31ba1ca6e2beeec5d`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Province history and the missing Coconi city
#### Reference and identification
Reference files are from the local Steam CK2 3.3.5.1 installation. The
provided vanilla map labels the province north of the lower Danube
**Calarasi** and shows it inside Bulgaria. The province bitmap and
`map/definition.csv` give that area ID **1642**, RGB **85,27,21**.
The green patch in OpenCK2 was the terrain showing through an absent
realm assignment. A holding icon was also missing when the province
history could not be associated with the county.

| Original input | Finding |
| --- | --- |
| `history/provinces/1642 - Calarasi.txt` | `c_calarasi` has three slots. `b_calarasi = castle` and `b_coconi = city` are active. Other listed candidates are comments. One slot is intentionally unused in 769. |
| `history/provinces/1642 - Tutrakan.txt` | A second file claims the same province ID and names `c_tutrakan`, sets one slot, and places `b_hvamm` there as a tribal holding. |
| `common/landed_titles/landed_titles.txt`, `history/titles/c_calarasi.txt` | The hierarchy has `c_calarasi` with child titles `b_calarasi` and `b_coconi`. `c_tutrakan` is absent. The active 769 county holder is 145950 under `d_wallachia`. There is no standalone `history/titles/b_coconi.txt`; its absence does not erase the city defined in province history. |
| `common/holding_types/00_holding_types.txt`, `common/buildings/00_buildings.txt` | City is a holding type. City buildings such as `ct_wall_1` are upgrade definitions. The base holding is created by `b_coconi = city`. |
| `localisation/HolyFury.csv` | `PROV1642` resolves to Calarasi, and `b_coconi` resolves to Coconi. |
| `gfx/interface/placeholder_city.tga`, `province_new_settlement.tga` | Separate city and unused-slot art is available in the installation. |

#### Root cause and implementation
OpenCK2's Realms and holdings readers independently selected the last
filename with the requested numeric ID. In the base install,
`1642 - Tutrakan.txt` sorts after `1642 - Calarasi.txt`. Realms
discarded the county mapping because the resulting title was undefined.
The holding reader picked the same wrong file. This was a file-selection
bug. City art, localization, and the dated holding parser were usable.

The province map now selects a single history path, which both readers
use. Higher mounted layers retain priority. Within a layer, the
filename matching the active `map/definition.csv` name wins. A lone
file can still use a different historical spelling. In an ambiguous
same-layer tie with neither filename matching the map name, the
first listed candidate remains selected. This is OpenCK2's resolver
policy. The supplied vanilla screenshot confirms the resulting county
identity in this case. The native engine's general collision and mod
precedence rules remain unverified. A separate Proton/WineD3D run reached vanilla
769 Custom Game Setup, but severe map rendering artifacts prevented
a reliable direct Coconi inspection in that session. The vanilla
comparison uses the supplied screenshot for the county and realm
and the installed data for the holding.

In OpenCK2 at 769, clicking Călărași now selects the county within
Bulgaria. Its County Holdings panel shows the castle, the Coconi city,
and the unused third slot. Hovering the city displayed **City: Coconi**.
The renderer also wraps holding slots onto a second row when capacity
exceeds seven, so modded capacities within the supported limit no
longer draw multiple slots at the same coordinates. No audited base
province required that second row.

#### Full-map scan
The base-install scan covered all **1,740 mapped provinces with
history** at all **11 dates** in `common/bookmarks/00_bookmarks.txt`
(19,140 province/date checks). Twenty-one province IDs had duplicate
history files. Each had one filename matching the map definition.
The previous alphabetical last-file rule chose a different file for
six IDs:

| ID | Map province |
| --- | --- |
| 446 | Morava |
| 722 | Al 'Aqabah |
| 1642 | Călărași |
| 1652 | Borovichi |
| 1676 | Kromy |
| 1681 | Kursk |

Five of the six old selections pointed to a county title absent from
the shipped hierarchy. Morava used a different history file without
losing its county title. After the fix, every audited province had
bitmap pixels, a defined county, and at least one populated holding.
Every populated holding matched its parent county in the shipped
landed titles and had an exact localization key. An independent
read of the loose histories agreed with the C loader on active holding
IDs and types across the scanned dates, including the trailing dot in
Marmaros's `895.1.1.` date token. At 769, the audit found 958 cities
and 1,570 counties with at least one intentionally unused slot.

This is a data audit and a targeted runtime display check. It did
not visually inspect every tile, validate vanilla's generated holding
owners or building levels, or sweep DLC/mod combinations.

#### Reproduce
```sh
make holding-audit check-holdings CFLAGS='-O2 -Werror'
CK2="${CK2_DIR:-$HOME/.steam/steam/steamapps/common/Crusader Kings II}"
./bin/holding-audit "$CK2" 769.1.1
```

The audit emits province ID, map name, selected history file, county
title, slot capacity, and active holding IDs/types. To repeat the
comparison, run it at each `date` in `common/bookmarks/00_bookmarks.txt`.
Join each emitted county and holding against `common/landed_titles/`,
each holding against `localisation/`, and each province ID against its
RGB in `map/definition.csv` and `map/provinces.bmp`. The synthetic
test verifies canonical selection, renamed mod override precedence,
and dated addition and removal of holdings.

---

<a id="source-docs-reverse-engineering-portraits-and-ruler-panel-md"></a>
## Source note: `docs/reverse-engineering/PORTRAITS-AND-RULER-PANEL.md`

> Preserved from the prior source archive. Original SHA-256: `11dff3e48740ce0c4d68d5d97709423b08c6d1715af55f50bfba3fd0920088bf`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Portraits and the setup ruler panel
#### Reference case
Use 7 August 936, select Otto in Custom Game Setup, and record enabled
DLC, UI scale, and window size. The supplied vanilla panel shows King
Otto of Germany and 21 vassal rows. OpenCK2's history model produces
16 title rows, including multiple titles held by one person. The
first row names Heinrich by Altdorf. The reference displays the same
holder as Count Heinrich of Zurichgau. The source records Zurichgau
under Swabia and Altdorf under Germany in the current history model.
A matched roster requires tracing vanilla's direct-vassal initialization,
display title selection, count, and ordering.

#### Confirmed inputs and fixes
- `history/characters/german.txt` defines character 1282 with
  `dna = rcrfdblvsvv` and `prp = aj000a00000000000000000000000000000000`.
  `common/bookmarks/00_bookmarks.txt` uses `properties` inside Otto's
  authored bookmark portrait. OpenCK2 now reads historical `prp`
  while keeping bookmark properties separate. Historical age is 23
  and the authored bookmark age is 24. Both select the same adult
  age variant at this date.
- The previous loader found profiles for all 832 distinct held county
  rulers in the 936 snapshot. Only six had authored DNA. The new
  renderer uses a stable character-ID-based DNA fallback when DNA
  is absent. It uses the installed portrait assets, including female
  and child variants where available, and reads age thresholds from
  `common/defines.lua`. This fallback does not reproduce vanilla's
  DNA initialization or scripted properties.
- `interface/frontend.gui` supplies the large portrait and its
  separate frame, title shield and crown, character-tab position,
  and the vassal row's portrait, frame, shield, and label positions.
  `interface/generalstuff.gfx` resolves the sprite references.
  The four ruler detail lines use the installed Arial12 bitmap font.
  The vassal list now draws visible faces and title shields and uses
  scrollbar sprites resolved from its `EU3_core.gui` scrollbar type.
  Wheel, arrows, and clicking the track scroll the visible rows.
  Portraits and shields are generated when selection or scroll changes.
- A controlled OpenCK2 render of Otto produced different pixels
  with base assets, with Monks and Mystics Content Pack enabled,
  and with that pack enabled plus the historical `prp` applied.
  This isolates two real input differences. It does not establish
  identical pixels with the vanilla compositor.

#### Native evidence and remaining work
The local Windows PE has the literal `interface/portrait_properties`
at VA `0x1410371b8`. Disassembly references it at `0x1404ddd40`
in a sequence passing directory strings to the same routine.
The REA/Ghidra trace and a targeted decompile both reached their
time bounds. The literal and its reference provide a starting point
for tracing the property loader. They do not establish how the
engine seeds DNA or updates portrait properties.

The installed `interface/portrait_properties/00_portrait_properties.txt`
describes weighted selection, forced factors, two passes, and
layer hiding. OpenCK2 still needs the native trigger contexts,
evaluation order, seed behavior, and shader composition. The current
title shields use installed flags and the existing mask. Generated
dynasty arms, full ruler titles, vassal roster and order, scrollbar
dragging, difficulty score, diplomacy state, and DLC recommendations
remain pending. The County Holdings panel is still an OpenCK2 extension.

#### Verification
`make check-portraits` runs the installed-data regression. Set
`CK2_DIR` if the game is outside the default Steam path. It checks
Otto's historical fields, changes in rendered pixels with and
without his properties, repeatable rendering for Heinrich, and
successful rendering for the four reference vassals, Hedwig, and
the five-year-old Liutgard. The live OpenCK2 check selected Otto
at 936, verified the portrait, crown, title shield, and visible
vassal faces, then exercised wheel and scrollbar-arrow scrolling.
The vanilla screenshot is the visual reference. A matched native
capture under controlled DLC and UI settings remains pending.

#### Paused parity work
The user confirmed that the portraits still differ from vanilla and paused
exact portrait matching on 2026-11-11. The current renderer remains the
baseline. Resume from the 936 Otto comparison with the same DLC and UI
scale. Trace native default DNA, property initialization and refresh,
and layer hiding before changing the fallback. Compare the bookmark and
selected portraits separately, then verify the four reference vassals,
an adult woman, and a child. Exact vassal count and order remain a separate
history and UI investigation.

---

<a id="source-docs-reverse-engineering-construction-md"></a>
## Source note: `docs/reverse-engineering/CONSTRUCTION.md`

> Preserved from the prior source archive. Original SHA-256: `1a34788b68aa4583ba3ea1acbdf12e7733ce27ff984552bb29ce8255f15b563f`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Holding construction evidence
Reference build is CK2 3.3.5.1 (SOHY), Windows Steam executable. Use the
binary hash and preferred base in the [evidence index](EXE_RESEARCH.md#source-docs-reverse-engineering-readme-md) before
reusing the VAs below.

#### Observed in installed files
- Province history sets `max_settlements` and assigns a type to each
  occupied `b_*` title. A city occupies one county slot. An undeveloped
  county slot is distinct from the building list inside an existing city.
- Dated history can also assign a building directly to a barony. Search
  Salamanca's province history for `b_salamanca = ca_wall_1`.
- `common/buildings/00_Castle.txt` and the other holding-type catalogs
  define building IDs, cost in gold, duration in days, upgrades,
  prerequisites, scripted triggers, and effects. For example, the first
  Castle Fortifications entry costs 50 gold and takes 182 authored days.
  The first Castle Walls entry costs 50 gold and takes 365 authored days.
- `interface/buildview.gui` declares `build_buildings_view`, its list
  and `build_building_entry`. `interface/buildview.gfx` defines its
  sprites. The installed versions of its background and list rows are
  DDS textures. The same GUI file declares `build_settlement_view` and
  `build_settlement_entry`, which match the live vanilla Create Holding
  window.
- `interface/province.gui` declares `ChooseSettlementDialog`.
  `common/holding_types/00_holding_types.txt` gives placement triggers,
  with empty triggers for castle, city, and temple.
  `common/defines.lua` declares `ENFORCE_ONE_OF_EACH_HOLDING`,
  `HOLDING_SLOT_BUILD_COST_MOD`, and a fort discount toggle.
  `common/static_modifiers.txt` supplies base `build_cost_modifier` and
  `build_time_modifier` in the settlement type blocks. Castle, city, and
  temple each have base values of 500 gold and 730 days.
- Vanilla in a paused game as Edward III on 1 January 1337 displayed
  five occupied slots and one empty slot in Middlesex. The Create
  Holding dialog quoted 759.5 gold and 616 days for all three types.
  Westminster's building window grouped upgrades by family. The first
  Castle Fortifications row displayed tier III, 134.4 gold and 308 days.
  These observed prices and durations include modifiers beyond the base
  values in the loose files.

#### Binary-supported findings
REA 6.8.0 with Ghidra 12.1.4 was tried with `rea trace` on
`BUILD_NEW_SETTLEMENT` and `rea decompile` on `0x14095ca7e`,
`0x140936d20`, and `0x140935e40`. The bounded provider calls were cancelled before returning
analysis. The following findings come from independent PE disassembly and
RIP-relative cross-references.

- The reference to `BUILD_NEW_SETTLEMENT` at `0x14095ca7e` is followed
  by calls that consume the string and an apparent display destination.
  This cross-reference alone does not identify the construction command.
- The branch formatting `BUILD_SETTLEMENT_TIME_DESC` at `0x14018ed6f`
  calls `0x140936d20` first and formats its return value. That function
  looks up a type record, reads a signed integer at offset `0x2c`,
  optionally applies a fixed-point adjustment from `0x1403f25b0`, clamps
  to a minimum of 1000, then divides by 1000. The caller at
  `0x140974cd3` scales the return by 24 and adds it to another value
  before storing it. This is consistent with a deadline measured in
  hours. Identifying the target field and modifier is still required.
- The adjacent `0x140935e40` resolves the same kind of type record and
  reads offset `0x28`. The define registration at `0x14047f18d` binds
  `HOLDING_SLOT_BUILD_COST_MOD` to `0x1416661a0`. The code at
  `0x1409360dc` reads it and uses it in a fixed-point factor based on the
  length of a four-byte-element vector at province offsets
  `0xce8` and `0xcf0`. This branch subtracts 1000 from the vector
  length scaled by 1000 and adds the result times the define to the
  1000 baseline factor. The caller at `0x14018cac2` consumes the result
  to format the price. The later calculations at `0x140936c5b` onward
  also consume province fields and type-specific adjustments.
  Their data producers and exact rounding still need a comparison.
- The branch at `0x14092c863` compares the length of the same vector with
  the return of `0x14092c6d0`. The one-of-each define is registered at
  `0x14047e29f` and checked at `0x14092c9f4`. Its branch scans occupied
  holdings for three type indices. It passes through if the requested
  type is absent or if all three are already present.

#### Implemented and checked in OpenCK2
Click an occupied holding in a running campaign to open its building list.
The list reads original definitions through the mounted content resolver
and uses vanilla panel geometry and textures. Scroll to inspect more rows.
Construction checks direct ownership and gold and runs one project per
holding. Clicking **Next Month** and running the clock both advance the
same queue. The county panel shows occupied and available slots from
province history. In a directly controlled county, an empty slot opens
the original Create Holding layout. Castle, city, and temple with empty
placement triggers can use the first unoccupied, unheld landed barony
in authored order. The queue reserves one county slot and creates the
holding on completion. Completed holdings can receive buildings.

Linux smoke test used Edward III in 1337. Westminster queued Castle
Fortifications for 182 days, Next Month reduced it to 151 days, and the
empty Middlesex slot reported five occupied out of six. After advancing
the prototype treasury, the Create Holding dialog queued Staines as a
city, charged 900 gold, and completed it after the countdown. Middlesex
then reported six of six slots. Staines opened its own building window
and successfully queued City Walls for 365 days. The automated
construction check covers costs, duration, explicit dated building history,
prerequisites, independent building queues, ownership, required holding
types, reservation, completion, full-county rejection, and reset. Existing holdings
checks cover dated capacity and settlement history.

#### Remaining work
- The prototype uses the base duration and the slot factor, with prices
  rounded up to whole gold for the current treasury representation.
  Middlesex therefore quotes 900 gold and 730 days. Recover the local
  modifier sources and fixed-point rounding to match the vanilla quote.
  Find the exact eligibility, candidate order, ownership assignment,
  and completion ordering. One pending settlement per county is a
  prototype restriction. Compare the missing-type rule with vanilla for
  counties with several open slots and for nonstandard governments.
- Replace the prototype treasury, raw authored costs and times, and
  tech-zero eligibility with verified economy, construction modifiers,
  and scripted triggers. Historical building generation and building
  effects still require engine work. Construction state is in-memory.
- Populate the rest of the build-view statistics and native controls.
  Compare building order, tiers, prerequisite tooltips, and scrolling
  against vanilla. The current list uses wheel scrolling and text status.

---

<a id="source-docs-reverse-engineering-console-findings-md"></a>
## Source note: `docs/reverse-engineering/CONSOLE-FINDINGS.md`

> Preserved from the prior source archive. Original SHA-256: `26aaf7603196ddd61afdf2c3e12a63e919f332ddb28b98770f33a2a47620358c`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Console System Investigation Findings
#### SummaryThe console is a developer/player command interface in CK2 that allows executing cheat commands, debugging tools, and game modifications. It is **disabled in Ironman mode** and accessed via the grave accent key (``).

#### Key Access Method- **Primary key**: Grave accent / backtick (ASCII 96, keycode SDLK_GRAVE)
- **Location**: Left of `1` on US keyboards
- **Alternative methods**: Various Alt+key combinations for different layouts
- **Ironman restriction**: Console is disabled when Ironman mode is enabled

#### Implementation Requirements
##### Missing Components in OpenCK21. **Console UI**: Text input field and output display area
2. **Command parser**: Lua command interpreter (`console_commands.lua`)
3. **Key handler**: `SDL_KEYDOWN` case for SDLK_GRAVE (currently not handled)
4. **Ironman flag**: Game setting to enable/disable console access
5. **Security layer**: Command whitelist/verification for safe execution

##### Event Handler LocationBased on code analysis, console needs addition to:
- `src/game/main.c:2561` - Key handling switch statement (SDL_KEYDOWN case)
- Add case for `SDLK_GRAVE` to toggle console visibility
- Console screen state management in menu structure

##### Core Commands AvailableThe original game's Lua console supports extensive commands:

**Character Commands** (`add_*`, `remove_*`, trait manipulation):
- `add_trait`, `remove_trait` - Trait modifications
- `add_modifier`, `remove_modifier` - Temporary effects
- `kill`, `die`, `age` - Character state changes
- `add_diplomacy`, `add_intrigue`, etc. - Attribute adjustments

**Global Commands**:
- `marry_anyone` - Marriage restrictions
- `discover_plots` - Plot visibility toggle
- `recalc_council` - Council recalculation (Conclave feature)

**Debug Commands** (Developer only):
- `debug_ai`, `reloadinterface`, `reload_succession_voting`
- Event and trigger testing with `testevent`

##### Security Considerations- Original CK2 marks all `debug_*` commands as developer-only
- Ironman mode prevents console access entirely
- Save file format difference between Ironman/Savegame (`.sav`) vs non-Ironman (plain text)

#### Data Files Required1. **Lua scripts**: `console_commands.lua`, `events/lua/console/*.lua`
2. **Game definitions**: `common/debug_*.txt`, command parameter lists
3. **Interface assets**: Console window graphics, input field textures

#### Evidence from Original Game- Console accessible via backtick key in non-Ironman saves
- Ironman save format blocks console execution at runtime
- Command scope system: Character/Global/None/Debug categories
- Parameter validation through Lua string operations

#### Implementation Approach
##### Phase 1: Basic Console Frame1. Add console visibility flag to menu structure
2. Create SDL window overlay for console input/output
3. Handle `SDLK_GRAVE` keypress to show/hide console
4. Implement basic text rendering and input capture

##### Phase 2: Command Interpreter5. Create command parser for simple token-based execution
6. Implement character lookup via `charinfo` command
7. Basic parameter validation (title names, character IDs)

##### Phase 3: Safe Subsets8. Port non-destructive commands first (stats display, culture change)
9. Implement `help` command with documentation
10. Gradually add more complex operations with verification

#### Related Project Files- `src/game/main.c:2561` - Key event handling location (needs SDLK_GRAVE case added)
- `src/game/settings.c` - Settings structure
- `docs/UI-TODO.md` - Character panel related work  
- `docs/CK2-MECHANICS.md:44` - Notes on Conclave council system

#### Status: NOT IMPLEMENTED - NEEDS CONSOLE SUBSYSTEM
#### Recommended Implementation Order1. Add case `SDLK_GRAVE` to switch at `main.c:2631`
2. Create console screen state in menu structure  
3. Implement basic console overlay with input field
4. Parse and validate simple commands
5. Port selected safe commands first

---

<a id="source-docs-reverse-engineering-finding-template-md"></a>
## Source note: `docs/reverse-engineering/FINDING-TEMPLATE.md`

> Preserved from the prior source archive. Original SHA-256: `3a3106c851bc96ad8352a7097054a7baa79545ca55092cbde819d922bf73ef4e`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### Reverse-engineering finding template
Copy the fields into a subsystem note or a focused investigation page.

```text
Question and roadmap task:
Status: pending | investigating | partial | verified for the stated scenario
Reference version, platform, executable SHA-256, and image base:
Reference scenario, DLC/mods, save or bookmark date, and inputs:
Installed definitions and files consulted:
REA/provider versions, analysis profile, evidence IDs, and exact commands:
Relevant VAs/RVAs and function purpose:
Callers, callees, object/field offsets, and order of operations:
Observed behavior and reproduction steps:
Binary-supported findings:
Inferences and alternatives:
Unverified hypotheses and next probe:
OpenCK2 source, implementation status, and known approximations:
Comparison command, expected and actual results:
Remaining differences and replacement conditions:
```

Describe the scope of the comparison, such as one title transition, one
renderer pass, or one save round trip. Link earlier evidence that the
investigation builds on.

---

<a id="source-docs-reverse-engineering-source-filenames-report-md"></a>
## Source note: `docs/reverse-engineering/source_filenames_report.md`

> Preserved from the prior source archive. Original SHA-256: `e757354e81244ebe7b681f72e3d536ec1a084383b3fbb7a9abc538b750c60078`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### FreeCK2game.exe — recovered development filenames
These names were extracted from printable strings embedded in the executable. They are **references**, not recovered source code. This is not necessarily a complete list of the original project files.

#### Counts
| Extension | Unique paths |
|---|---:|
| `.cpp` | 167 |
| `.h` | 1 |
| `.pdb` | 1 |

**Total unique paths: 169**

#### Full inventory
##### .cpp (167)
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\3dicontype.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\achievements.cpp`
- `D:\jenkins\workspace\external_libs\common\emotionfx\EMotionFX-3\API\Source\Actor.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\ai.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\ai_foreignminister.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\ally_orders.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\alternate_start.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\animatedmaptexttype.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\application.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\artifact.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\artifact_spawn.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\assetfactory.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\battleresult.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\bitmap.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\bitmapfont.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\bloodline.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\bookmarks.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\building.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\casusbellitype.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character_action.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character_auto.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character_death.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character_manager.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character_objectives.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character_opinion.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\character_player_data.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\characterhistory.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\characterinteraction.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\ck_application.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\coalition.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\coamanager.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\combatmanager.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\ConsoleCmdImpl.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\ConsoleCmdManager.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\contentview.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\controlcommands.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\councilposition.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\culture.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\decision.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\defines.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\directorysettings.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_core\discfile.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\disease.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\dlc.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\dynasty.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\effect.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\effectimplementation.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\emfxactortype.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\eu3graphicalmap.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\eu3idler.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\eu4converter.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\event.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\eventmanager.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\eventoption.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\eventscopevalue.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\eventwindow.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\feat_progress_storage.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\fixedwindow.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\flagtexturemap.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\frontend.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\frontendmultiplayerview.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\frontendsettings.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\gamebook.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\gamesetup.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\gamestate.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\gamestatecommands.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_gfx\gfx_dx9.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_gfx\gfx_image.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_gfx\gfx_settings.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_gfx\gfx_util.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\government.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\governmentflavor.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\graphics.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\graphicssettings.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\highperformanceclock.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\hint_manager.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\history.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\id.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\ingamelobby.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\instanttextboxtype.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\internationalizedtext.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\laddercalculator.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\languageidler.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\largefile.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\lexer.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_core\luainterface.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\main.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\map.cpp`
- `D:\jenkins\workspace\external_libs\common\emotionfx\EMotionFX-3\API\Core\Source\MCoreSystem.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\memoryfile.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\mercenary.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\messagehandler.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\MessageWindow.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\metaserverapplication.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\metaserverchannel.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\metaservercommands.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\metaserverconfiguration.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\metaserveruser.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\militarycomposition.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\modifier.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\musicmanager.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\nickname.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\nomad.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\offmap_history.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\onaction.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\opinion_modifier.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\order.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\parser.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_achievements\pdx_achievements.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_browser\internal\pops\pdx_browser_pops.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_applicationutils\encryption\pdx_encryption.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_net\pdx_net.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_core\pdxasset.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\pdxinput.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\pdxmap.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\pdxmapgeneration.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\pdxmeshtype.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\pdxsound.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_core\pdxsysinfo_windows.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\peaceoffer.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\persistent.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\portrait_properties.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\portraittype.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\province.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\provincegraphics.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\provincehistory.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\provincetemplate.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\region.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\religion.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\remotefile.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\road_to_titus_progression.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\rome_bitmap.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\savegamehelper.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\savegameinterfaces.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\server.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\session.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\settlement.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\society_object.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\sound.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\special_troops.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\spritetype.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\statistics.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\subunit.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\systemsettings.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\table.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\tcp.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\technology.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\terrain.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\textboxtype.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\texturedlinetype.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\graphics\texturehandler.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\title.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\titlehistory.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\traderoute.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\trait.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\tributary.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\trigger.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\triggerimplementation.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\unit.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\unitcommands.cpp`
- `D:\jenkins\workspace\ck2\clausewitz\pdx_core\virtualfilesystem.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\war.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\warhistory.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\wonder.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\wonder_upgrade.cpp`
- `D:\jenkins\workspace\ck2\ck2\source\wonder_utility.cpp`

##### .h (1)
- `D:\jenkins\workspace\ck2\clausewitz\clausewitzlib\choice.h`

##### .pdb (1)
- `D:\jenkins\workspace\ck2\outputdir\release\CK2game.pdb`

#### Method and limitations
Scanned both single-byte and UTF-16LE printable strings, retaining Windows absolute paths with source/build-related extensions. Deduplicated case-insensitively. An embedded path indicates a filename was referenced by the compiled binary, not that the source is present or that its functions can be reconstructed from the string alone. Compiler optimization, stripped symbols, and build configurations can hide other files.

---

<a id="source-openck2-research-report-md"></a>
## Source note: `OpenCK2_research_report.md`

> Preserved from the prior source archive. Original SHA-256: `d204b3cd0a99c48b75c3205a7e401f4f9d0ddbfe3b74abdbcb19befb378c13b5`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2 Bloodlines and Governments: incremental research and implementation
#### Scope and limits
Static analysis of the supplied 64-bit Windows executable, uploaded C source, and interface definitions. No original executable was executed. No source-level symbols were recovered: function names below describe inferred roles. Absolute addresses apply only to this executable; subtract image base 0x140000000 for RVAs. Function ranges come from PE exception metadata. ASLR changes runtime addresses.

Executable SHA-256: a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d.

This is an incremental implementation, not complete functional equivalence. In particular, the new definition catalog is an API awaiting runtime/UI wiring. No bloodline membership, births, propagation, government trigger evaluator, or full government assignment has been implemented.

#### Method and reproducibility
`research/trace_binary.py` reads PE exception metadata and disassembles each covered text function using Capstone. It finds direct RIP-relative and immediate references to selected strings. Output: 69 matching strings and 99 instruction references in `xrefs.json`; containing function disassembly in `disassembly.txt`. `annotate.py` provides focused disassembly with readable RIP-relative strings in `focused.txt`. Install Python `pefile` and `capstone`, place the original file at `inputs/FreeCK2game.exe`, and run scripts from their parent workspace layout. Searches do not recover indirect/vtable references or functions without unwind coverage. Unrelated inheritance references are retained as context, not classified as bloodline logic.

#### Located functions and confidence
| Function range (VA, end exclusive) | Evidence | Interpretation | Confidence |
|---|---|---|---|
| 0x140181b40–0x140182090 | bloodline.cpp at 0x140181f69; invalid-inheritance diagnostic at 0x140181f82 | Bloodline definition token dispatcher | High for parser role; class name not proven |
| 0x1401834a0–0x1401839fd | bloodline.cpp at 0x140183580, 0x140183744, 0x1401838d5; unknown owner/member diagnostics | Runtime/history/save bloodline object parser resolving characters | High for owner/member resolution, medium for persistence context |
| 0x1407aef70–0x1407b0f57 | government.cpp at 0x1407af084; negative AI aggression diagnostic | Government definition token dispatcher | High |
| 0x1407b69d0–0x1407b6f35 | governmentflavor.cpp references at 0x1407b6a64, 0x1407b6e0a, 0x1407b6e35; .txt enumeration; malformed flavor diagnostic | Government flavor collection loader | High for loader role; selection precedence unknown |
| 0x1404dd640–0x1404ddf54 | common/governments at 0x1404ddacf; common/bloodlines at 0x1404dde08 | Data-directory registration/initialization | High for directory connection |
| 0x1405fae30–0x1405fb065 | create_bloodline inheritance assertion at 0x1405faf83 | Script effect argument parsing | High for parsing, no proof of execution algorithm |
| 0x1404f5f80–0x1404f63e4 | inheritance tooltip localization references | Bloodline inheritance presentation | High for presentation only |
| 0x1404fd080–0x1404fe1d5 | bloodlines_window at 0x1404fd11a | Window construction/binding candidate | Medium |
| 0x140502f10–0x1405079c9 and 0x14050a670–0x14050beff | government_icon at 0x1405072f3 and 0x14050b180 | Government icon UI binding candidates | Medium |

#### Verified bloodline parser behavior
The token-registration routine at 0x14007e3f0 associates strings with numeric IDs. At 0x14008f682 it registers `inheritance` as 0x307f. The definition dispatcher branches on that ID at 0x140181b70/0x140181b7d to 0x140181f2d. There it compares the parsed value token and writes a 32-bit field at object+0x45c:

| Loose-file value | Token ID | Stored enum | Branch/store evidence |
|---|---|---|---|
| matrilineal | 0x2e4d | 1 | registration 0x14008c412; store 0x140182014 |
| patrilineal | 0x3675 | 2 | registration 0x1400983fa; store 0x140182007 |
| all_descendants | 0x3676 | 0 | registration 0x140098412; store 0x140181ffe |
| no_inheritance | 0x368e | 3 | registration 0x140098652; store 0x140181fec |

Confidence: high. The second parser writes the same enum choices at object+0x158 (0x1401837c4–0x1401837f1). Invalid choices produce diagnostics, rather than visibly selecting an arbitrary fallback in the examined branch. Original constructor defaults remain unknown. OpenCK2 therefore returns UNKNOWN for missing/unrecognized inheritance, rather than fabricating a default.

These enum values establish allowed input and representation. They do not establish sex filtering, bastard exclusions, marriage inversion, adoption, retroactive propagation, or special event exceptions. Tooltip strings mention patrilineal, matrilineal, inversion and no-bastards behavior, but do not prove those algorithms.

#### Data structure observations
Confidence high for instruction-level offsets, medium or lower for semantic labels. Offsets are build-specific and are not copied into OpenCK2 structs.

- Definition inheritance is a dword at +0x45c. Three boolean-reader branches address +0x458, +0x459 and +0x45a at 0x140182043–0x140182063. `matrilineal_override` is token 0x3771 and routes to +0x45a; other boolean names need registration tracing.
- Runtime bloodline parser stores inheritance at +0x158. A character-like pointer is written at +0x100 at 0x14018367e; owner/member diagnostics and validity checks support character-reference handling, but exact field identity requires complete control-flow tracing.
- Government parsing writes many bits in bytes +0x196c through +0x1970. Do not assume one C boolean per option. At 0x1407af052 it reads an integer into +0x193c, tests it against zero at 0x1407af061, and emits “AI aggression cannot be negative” on failure. The field's semantic identification is high confidence from this branch.
- Flavor loader allocates a 0x20-byte collection holder (0x1407b69fa onward), clears three pointer-sized slots and a dword, enumerates `.txt` files and validates parsed entries. This is a collection wrapper, not proof that each flavor record is 0x20 bytes. Selection weighting and ordering remain unresolved.

#### Government assignment status
The source previously has only a Governments map-mode identifier. It has no government rule engine. Executable strings include `set_government_type`; registration and references do not prove how the default government is chosen. No priority ordering, liege inheritance rule, holding classification, or potential-trigger behavior is claimed as verified.

The patch retains an explicitly authored `government` scalar from character history and applies dated scalar overrides at or before the bookmark, with latest-date precedence independent of input ordering. This is an OpenCK2 history-reader extension following existing code conventions, not a proven reconstruction of original government assignment. Unknown/absent assignment remains empty. Scripted `set_government_type` effects are not executed. Title history assignment is not added.

#### GUI connections
The supplied original definitions provide:

- `bloodlines.gui`: `bloodlines_window`, `bloodlines_list`, `bloodline_entry`, `bloodline_portrait`, `bloodline_icon`, condition controls and `bloodline_living_member_button`.
- `bloodlinememberview.gui`: `bloodlinememberwindow`.
- `domestic_dynastic.gui`: `button_bloodlines` with `GFX_bloodlines_button`, and `government_icon` with a default `GFX_icon_feudal_government`.

Executable cross-references above connect these names to UI routines. The feudal sprite in the GUI is a layout default, not evidence that all rulers should be assigned feudal government. The patch adds no hardcoded replacement windows or icons. Rendering and click callbacks remain TODO.

#### Implemented changes
1. `src/game/definitions.c/.h`: owned catalog of keys, government groups and losslessly retained block bodies. Uses the existing CK2 tokenizer and mounted asset resolver, reads `.txt` files from common/governments or common/bloodlines, retains unknown nested fields, and rejects incomplete braces. Parse calls are transactional. Later same-key definitions replace earlier entries. This override policy and grouped government format are provisional engine choices pending actual gameplay files; not binary-verified semantics. Directory load can partially succeed and returns false if any file fails; empty/missing directories currently return true with an empty catalog.
2. Verified explicit inheritance enum decoder, without implementing unverified propagation or defaults.
3. `rulers.h/.c`: retain explicit government history values. Buffer size 128 follows existing bounded key style; long values truncate and require future diagnostics.
4. `tests/`: parser regression fixtures and authored government history tests, using the real tokenizer and ruler parser with a clearly marked test-only SDL shim.

Call `ck2_definitions_load(&catalog, root, true)` for governments and false for bloodlines after mounting content. Initialize catalog to zero, inspect return status, query via `ck2_definition_get`, and free it during teardown. Add definitions.c to the real build target. The uploaded ZIP has no build configuration, so automatic target wiring cannot be supplied or verified here. No active gameplay caller is added for the catalog yet.

#### Validation
Ran `ASAN_OPTIONS=detect_leaks=0 sh tests/run_definitions.sh` from source: passed. Both test executables compile as C11 with -Wall -Wextra -Werror, AddressSanitizer and UndefinedBehaviorSanitizer. Tests cover grouped records, nested/unknown data retention, same-key replacement, malformed input rollback, unmatched close brace, all four inheritance values, unknown inheritance, dated government precedence, future-date exclusion and absent assignment reset. Ruler tests compile the complete rulers.c translation unit and invoke its actual static history parser; unused external functions are discarded at link time.

LeakSanitizer cannot run under this environment's tracing setup and was disabled after it reported that limitation. This is not a successful leak check. Test shim does not validate SDL ABI. Full engine compilation and rendering are blocked by absent build files and SDL development libraries. No real common/governments, common/bloodlines, history/bloodlines or playable game installation was supplied, so fixture tests do not demonstrate compatibility with all original files or DLC/mod precedence.

#### Remaining TODOs, in order
1. Obtain the actual loose gameplay definitions and original project build files. Validate file layout, mounted content order and duplicate semantics; add load failure diagnostics and integration tests.
2. Wire catalogs into engine initialization/teardown and the real build target. Do not treat successful loading as support for rules execution.
3. Complete token-registration mapping for bloodline fields; trace constructors, owner/member containers, and callers of both parsers. Recover history/bloodlines loading, founders, creation/death dates and membership.
4. Trace propagation callers around births, marriages and scripted effects; establish exclusions and inversion with instruction-level evidence and tests.
5. Map government parser token IDs to field bits and trigger blocks. Recover assignment, validation, holding restrictions and script-effect execution; implement an explicit supported trigger subset and report unsupported conditions.
6. Trace government flavor entry parsers and selection criteria; retain original flavor definitions rather than inventing title/name substitutions.
7. Bind original GUI definitions and sprites to catalog/character state; connect close, list selection and living-member actions. Validate screenshots against the original game only after data and rendering work.
8. Test full game builds, bookmark/history behavior, DLC combinations, mods, save/load and long-running simulation before making compatibility claims.

#### Deliverables
Modified source ZIP contains the entire uploaded source tree plus incremental changes and tests. Research bundle includes this report, machine-readable xrefs, full relevant function disassembly, focused annotated disassembly and reproducible Python analysis scripts. Neither deliverable redistributes the original executable or interface archive.

---

<a id="source-openck2-ui-map-implementation-plan-md"></a>
## Source note: `OpenCK2_UI_Map_Implementation_Plan.md`

> Preserved from the prior source archive. Original SHA-256: `45e453288398f1c6c5e08a8cb6bad31556060d77fa0895b341e2cbe6aa4114c3`. Older status/paths within the note may describe an earlier checkpoint; current status takes precedence in the overview above.

### OpenCK2: province interaction, HUD layout, Vassals and map modes
Planning baseline: 12 November 2026. **Planning and investigation only; no project code has been modified.**

#### 1. Baseline and evidence rules
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

#### 2. Main findings and scope
1. **Right-click navigation is blocked by existing drag handling. [S]** `handle_event()` records both middle and right button down as map drags, then consumes the matching button release before any navigation logic. A stationary right-click currently never reaches an owner-opening path.
2. **The county holder and independent ruler are already separate concepts. [S]** `realm_county_info(..., false, ...)` resolves the county; the `true` variant resolves the independent realm. `realm_province_ruler()` uses the latter. Using that function blindly would open the emperor instead of a count in a vassal province.
3. **A basic province tooltip already exists, but only in setup. [S]** `setup_tooltip()` resolves county holder and independent realm. Campaign drawing has HUD/character tooltips, not the requested province tooltip.
4. **Society is a top HUD icon, not one of the six character-detail tabs. [S,L]** The current HUD draws eight raw textures in a fixed loop. The GUI defines Society next to Religion. The source already assigns it the expected basic x-slot, so the reported visible separation is not yet explained by an obvious extra index. Background extensions, sprite cropping and actual runtime asset variants must be checked before shifting it.
5. **The “blank tab left of wealth” is not conclusively identified. [U]** There is no separate republic/nomad entry in the current eight-icon draw loop. The original GUI defines optional Republic/Nomad controls, a Back control and decorative extension pieces. They are diagnostic candidates, not a verified identification of the unwanted element.
6. **Vassal selection and wheel scrolling already exist. [S]** The list is incomplete, not absent: it shows a limited set of portraits/shields/combined labels. It lacks original sort controls, tax/opinion columns and visible scrollbar binding. Its data query is title-based and can omit or duplicate character vassals.
7. **There are 18 explicit campaign map-mode buttons in `main.gui`. [L]** This is different from the setup screen's smaller set. Only setup Terrain/Realms currently switch behavior. Campaign mode buttons and a shared mode state remain to be built.
8. **Latest archive is not a cumulative merge of every earlier patch. [S]** It contains `bloodline.c`, but not the earlier `definitions.c/.h` catalog or an explicit government key in `ruler_profile`. Do not design the government map mode as if those earlier changes are present.

**Difficulty:** HUD alignment is a bounded task; shared input/hover/owner handling is medium complexity; full CK2-equivalent Vassal economics/opinion and all dynamic map modes are large subsystem work. Implement these as separate reviewable stages. No single “add all map modes” change should imply the simulation behind them already exists.

#### 3. Exact OpenCK2 code map
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

##### Baseline limitations to resolve before implementation
- `Makefile` exists in this archive, unlike the earlier source-only upload. It references `tests/*.c` and `tools/*.c` that are **not included** in this 56-file archive. A target name alone is not a runnable test suite. [S]
- SDL2/SDL2_image/SDL2_mixer dependency discovery fails in the current environment. No full build or game launch is claimed. [S]
- Rendering/history frequently uses `menu->start_date` while the campaign calendar advances in `menu->campaign.date`. This plan must define whether the first milestone is a bookmark snapshot or live-date state. Do not quietly claim live ownership/weather by displaying a newer date over old data. [S,P]
- `province_at()` returns land provinces only; water/ocean hover needs a separate lookup policy if desired. [S]

#### 4. Original interface definitions to reuse
##### 4.1 Society, conditional topbar controls and the unidentified blank area
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

##### 4.2 Character tabs and Vassals
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

##### 4.3 Tooltip skin and data
The supplied `interface/EU3_core.gfx` defines a **textSpriteType** named `ToolTip`, using `gfx/interface/tooltip_bg.tga`. `assets.c` already loads a tooltip DDS for the setup tooltip. The existing `panel_tooltip()` instead draws a flat fill and border. A generic sprite loader that searches only `spriteTypes`/`spriteType` is not automatically sufficient for `textSpriteType`. [L,S]

The screenshot shows a province/owner heading, realm line, terrain, static climate, current winter, military information/defender bonus, supply limit, and modifier-key commands. [V] No complete dedicated dynamic province-tooltip layout was identified in the inspected GUI files. Do not invent a missing `province_tooltip.gui` or claim all tooltip content is authored in one loose window. The executable builds at least part of this text dynamically. [B,U]

##### 4.4 Original campaign map-mode inventory
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

#### 5. Executable research: concrete entry points and limits
##### Method
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

##### Next disassembly questions, in useful order
1. Trace callers and mode branches in `0x140910520`; find owner, realm-name, defender bonus and supply-value producers. Resolve `PROVINCE_HOTKEY_INFO` from the original localization before translating placeholders.
2. Follow callbacks registered by `0x140660310` and `0x14065f8d0`. Recover mode IDs, state-dependent visibility and extension offsets. Do not infer original numeric IDs from OpenCK2's setup enum.
3. Follow row construction and sort callback targets from `0x1404f67d0` and `0x1404fc2f0`. Determine tax units, opinion target/direction and stable ordering.
4. Investigate source-path anchors when relevant data flows reach them; do not spend time decompiling an entire file-sized region before implementing a bounded UI milestone.

#### 6. Data contract and architecture to implement
All names in this section are **proposed APIs/types [P]**, unless explicitly called existing.

##### Shared map hit and owner model
Introduce a `map_target` result containing `province_index`, `province_id`, county title key, `owner_id`, independent realm title key and `top_liege_id`, plus separate known/valid flags. Keep occupied/controller identity separate for later support. Do not store an ambiguous single `ruler` field.

Resolve via existing `map_view_pick()` → `province_at()` → pointer difference for **array index** → `realm_county_info(...false)` / `realm_county_info(...true)` → `realm_ruler()`. RGB-sorted province array indexes are not province IDs. Validate all transitions and handle missing county/holder/profile without dereferencing null.

Define owner navigation initially as **county holder**, based on the existing county mapping. [P] Verify province-capital holding owner versus county holder against actual CK2 data; the screenshot's emperor example does not distinguish them. Do not use independent ruler as a silent fallback. Shift-right-click targets the top liege when known. [V,P]

For the first milestone, identify the state as bookmark history. For live-date support, separate immutable history inputs from mutable campaign ownership/state and update the same read model consistently. Avoid reparsing all history files on every calendar day or mouse move.

##### Input ownership
Route input to modal windows, character/building panels, HUD/mode controls, then map. Derive blocking rectangles from visible widgets/windows; the fixed `campaign_hud[]` does not accurately represent an open 500+-pixel character panel. Its current fixed left panel ends around x=348 while the character panel extends farther. [S,P]

Use a pending pointer gesture: button, down point, maximum movement, down target and modifiers. A small movement threshold (initially configurable, e.g. 4 logical pixels) is an OpenCK2 usability choice, not a recovered CK2 constant. Preserve right-drag pan once threshold is crossed; on stationary release dispatch owner navigation. Middle-button pan remains available. Cancel pending gestures on focus loss, screen transition and incompatible modal changes. Decide whether modifier state is captured on press or release and test it consistently.

Do not advertise Alt-right-click interactions until a real interaction menu exists. The reference screenshot documents Control-left-click sub-realms, Right-click owner, Shift-right-click top liege, Alt-right-click interaction and Alt+Shift-right-click top-liege interaction. Reproduce supported commands first and build hint text from actual supported commands.

##### Province facts and tooltip model
Proposed `province_facts` cache: date/revision, terrain key, climate key, province religion/culture keys and known flags. Resolve static/dated information through original province history and relevant common/map definitions, with mounted DLC/mod overrides. Candidate files such as `map/terrain.txt` and `map/climate.txt` must be confirmed against the user's actual installation before being treated as authoritative paths. `common/religions` and `common/cultures` directories occur in the binary; their complete schemas/colors still require the loose files.

The renderer's `terrain.bmp` palette selection (`index_at(...) & 15`) is a visual tile operation, **not a verified province terrain classification**. Do not infer province terrain from its center pixel or assign a climate category based only on latitude. Read authored overrides first, then investigate any original fallback algorithm.

Separate static climate from current winter, and current winter from arbitrary month-based guesses. Unknown supply limit, tax/opinion, defense percentage, disease and weather should be omitted or clearly shown as unavailable. Supply is potentially viewer/army/context dependent; a single cached number per province may be insufficient.

Tooltip model should be structured lines/runs with localization keys, resolved values, optional section separators and supported action hints. First milestone can reuse `panel_tooltip()`'s wrapping/clamping; visual parity later uses `ToolTip` skin and the original font/text styling. Do not print raw CK2 formatting codes or invent supply values. Cache facts outside the draw loop; invalidate on province, date, content or relevant simulation revision.

##### Character-based Vassal model
Keep `realm_vassal_titles()` for existing title-based setup callers. Add a separate direct-character-vassal query that considers all titles held by the selected character and resolves each candidate's actual liege relationship. Deduplicate by character ID, choose one display title per character, exclude self and deeper indirect vassals, and return a total count independent of page capacity.

Existing failure modes to fix: multiple held titles belonging to one vassal can yield duplicate rows; considering only `realm_primary_title()` can omit vassals under another held title; equality to one liege title key does not fully model character allegiance. These follow from source structure; the exact set of affected historical rulers has not been tested.

Proposed row state: character ID, display title, rank, name/title text, portrait/shield, optional tax, optional opinion, visibility flags and stable sort key. Unknown tax/opinion remain unknown, never zero. Sort Tier/Name first; activate Tax/Opinion sorting only when the model can distinguish known values. Preserve page position by stable row identity when practical and clamp after filtering/sort changes.

##### Shared map-mode model
Proposed `map_modes.c/.h`: semantic mode enum/descriptor, supported-state flag, selected/reference character or realm, revision key and a province color resolver. Proposed `campaign_hud.c/.h` can own campaign navigation/mode widget binding. New module names are proposals, not files already in the archive.

Reuse `map_view.samples`/province geometry, existing realm color data and water rendering. Separate data mapping from projected overlay generation. Cache province-to-group/color values; rebuild only when their inputs change. Camera movement changes projection; it should not reload history unnecessarily. Preserve selection highlights, borders, realm labels and tooltip hit detection across mode switches. Keep de jure parentage distinct from current liege chains and ownership distinct from occupation/controller.

Original GUI/GFX governs placement, sprites, shortcut and tooltip metadata; engine code still needs semantic handlers. An explicit mapping from original widget IDs to OpenCK2 capabilities is appropriate. Hardcoding historical rulers, province IDs, colors or guessed mechanics is not.

#### 7. Implementation stages and exit gates
##### Stage 0 — Establish a reproducible baseline (P0)
**Change later:** build/test scaffolding only as necessary. **Inspect now:** Makefile and mounted content path.

- Recover missing existing tests/tools from the actual repository; obtain SDL dependencies and actual game-data root without redistributing those assets.
- Record enabled mods/DLC, asset-priority order, resolution/logical size and source revision. Capture baseline OpenCK2 HUD and a known province with county owner different from top liege.
- Verify whether earlier research modules were intentionally omitted from this archive; any merge must be explicit.
- Establish the first delivery as bookmark-correct; document live-simulation limitations.

**Exit gate:** baseline build launches or exact build blockers are recorded; relevant test files are actually present; a known owner/top-liege fixture and visual comparison setup are identified.

##### Stage 1 — HUD binding audit and bounded layout corrections (P1, low/medium)
**Files:** `assets.c/.h`, `main.c` navigation drawing/topbar lifecycle, `setup_toolbar.c` GFX loading, `frontend_gui.c` only if required; optionally new `campaign_hud.c/.h`.

- Bind the eight actual navigation nodes from `main.gui`, including Society's correct GFX source and two-frame metadata.
- Respect parent offsets, GUI scale where used, selected/hover/disabled presentation, and missing-asset fallback. Avoid globally reinterpreting all anchors before a targeted fixture proves it necessary.
- Audit extension backgrounds and conditional Republic/Nomad/Back elements. Identify the blank element by draw record before hiding/removing it.
- Replace coarse HUD navigation hover/hit rectangles with visible widget geometry. Keep unsupported tabs visible with clear status; hiding the specifically unwanted blank control should not create an invisible clickable region.

**Exit gate:** all eight navigation icons appear in the original defined order; Society shares the row and expected background; blank target is positively identified and removed/conditioned; real wealth and other requested icons remain present. No gameplay screens need to be implemented merely to correct the row.

##### Stage 2 — Shared picking and province-owner right-click (P1, medium)
**Files:** `main.c` input paths, `realms.c/.h`, `campaign.c/.h`; optional new `province_interaction.c/.h`.

- Implement one validated map-target query and use it for hover, clicks and mode context.
- Separate UI input from map input, and stationary right-click from drag.
- On owner action call `inspect_campaign_character()` with the county holder's actual profile. Open top liege only for the explicit modified action. Reopen/refresh correctly if the same ID already has an open view.
- Preserve left-click selection and right/middle panning; cancel on focus loss. Keep setup behavior unless intentionally changed and documented.

**Exit gate:** nested-vassal fixture opens count on right-click and independent ruler on Shift-right-click; dragging never opens a character; clicking panels never selects the map behind them.

##### Stage 3 — Honest campaign province tooltips (P1/P2, medium)
**Files:** `main.c` hover/draw order, `campaign.c/.h` or new `province_facts.c/.h`, `ck2_script.c` as needed, shared tooltip renderer extracted from `character_view.c` only when useful.

- First add province/owner/realm lines and correct action hints from the same query used by navigation.
- Resolve terrain and static climate from verified loose data. Expose province religion/culture for upcoming map modes.
- Add hover delay/cancellation and one tooltip target per frame; no file I/O in render or per-frame motion handling.
- Then bind original tooltip skin and localization/rich text. Add winter/supply/defense only after state/formula verification; unavailable sections remain absent.

**Exit gate:** correct tooltip on unobscured land; no tooltip through UI, during drag, after window leave or over invalid pixels. Tooltip content never disagrees with the owner action. Unknown numeric military details are not fabricated.

##### Stage 4 — Complete the structural Vassals list (P2, medium/high)
**Files:** `realms.c/.h`, `character_view.c/.h`, `main.c` click/wheel paths, reusable scrollbar code in `setup_toolbar.c/.h` or a small shared widget module.

- Add and test character-based direct-vassal enumeration; preserve existing setup function behavior.
- Bind original sort headers, row name/title/rank/shield columns, visible count and `standardlistbox_slider`.
- Use one function to compute row rectangles for drawing and picking, including template spacing/borders, scale and clipping. Remove unrelated fixed row offsets/dimensions only after mapping them to original definitions.
- Add Tier/Name sort directions and deterministic tie-breaks; complete portrait/row navigation and scrolling.
- Bind tax/opinion visuals with unknown placeholders. Implement real values and conditional church/relationship overlays in a subsequent separately validated change.

**Exit gate:** each direct character vassal appears once; total count and scrollbar match; sort/select/scroll remain consistent; row labels and shield refer to the same character/title; no invented tax/opinion.

##### Stage 5 — Campaign map-mode controls and shared state (P2, medium)
**Files:** new proposed `map_modes.c/.h` and possibly `campaign_hud.c/.h`; `main.c`, `setup_toolbar.c`, `realms.c`, `Makefile`.

- Bind all 18 original mode icons, toggle and custom slots from their correct GUI parents; preserve frame metadata, shortcuts and tooltips.
- Implement shared Terrain/Realms selection first, with active visual state. Expose other modes as unavailable until their resolvers work; clicking an unavailable mode must not silently recolor the map as Realms or set a false active state.
- Build explicit setup-to-semantic-mode mapping without merging setup-only Shattered/Random controls into the map modes.
- Test mode state through pan, zoom, resize/logical-size changes and panel opening. Unimplemented custom slot remapping can remain disabled with a clear status.

**Exit gate:** Terrain and Realms switch in campaign via buttons/shortcuts; remaining original icons render consistently and report support status; no click-through or shortcut theft from text entry/modal windows.

##### Stage 6 — Incremental data-backed thematic modes (P2/P3, medium/high)
Recommended order: province Religion → province Culture → de jure Duchies/Kingdoms/Empires → Dynasties → Direct Vassals. Reorder only if actual loose-file coverage favors another mode.

- Religion/Culture share province facts and use original definition colors with explicit missing-data behavior.
- De jure modes traverse authored parent hierarchy and separately handle dated changes; never substitute current liege chains.
- Dynasty colors/palette and holder basis require original comparison; do not pretend a deterministic engineering fallback palette is an original CK2 color rule.
- Direct Vassals requires the character liege model and an explicitly chosen reference realm.
- Keep cache/revision accounting per mode; add legends and mode-specific tooltip facts where data supports them.

**Exit gate per mode:** independently verified province group fixtures; correct unknown/unlanded/sea handling; unchanged geometry/selection; screenshot comparison at the same bookmark and content configuration. Do not wait until all six are coded to test the first.

##### Stage 7 — Dynamic mechanics, visual parity and regression (P3)
Diplomacy, Economy, Revolt Risk, Opinions, Trade Zones, Governments, Coalitions, Epidemics and Players depend on systems absent or incomplete in this baseline. Implement prerequisites in separate features, then add each mode. Supply/winter/defense and exact tax/opinion are similarly separate milestones.

Compare original CK2 and OpenCK2 at matching bookmark/date, character/province, resolution, UI scale and DLC/mod state. Start with 769, 867 and 1066 where data is available; additionally use the 1337 reference for the shown list/tooltip. A screenshot at a different date or content configuration is not a reliable numeric oracle.

**Exit gate:** all supported modes have data tests and runtime validation; unsupported behavior is documented; no claim of complete CK2 equivalence without side-by-side evidence and edge-case coverage.

#### 8. Acceptance-test matrix
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

#### 9. Remaining unknowns and required evidence
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

#### 10. Prioritized TODO checklist
Check boxes remain open because this request authorizes planning, not implementation.

##### P0 — Baseline and evidence
- [ ] BASE-01: Confirm `OpenCK2_character_icons_extended.zip` matches the intended working branch; record revision/content settings. **Stage 0; acceptance B01–B02.**
- [ ] BASE-02: Restore missing tests/tools and install/locate SDL build dependencies; run baseline build and existing checks where available. **Stage 0; B01.**
- [ ] BASE-03: Capture current OpenCK2 HUD and record draw identities for Society/blank element; identify exact offending layer. **Stage 0/1; H01–H03.**
- [ ] BASE-04: Create fixture distinguishing county owner, intermediate liege and independent ruler; explicitly choose bookmark-state milestone. **Stage 0; I01,T07.**
- [ ] BASE-05: Preserve legacy research/module differences as an explicit merge decision; do not assume government catalog exists. **Stage 0.**

##### P1 — First usable interaction delivery
- [ ] HUD-01: Bind original eight topbar navigation widgets, add Society GFX source and correct sprite-frame geometry. **Stage 1; H01–H02,H04.**
- [ ] HUD-02: Correct background/conditional composition and remove only positively identified blank element and its hitbox. **Stage 1; H03,H05.**
- [ ] INPUT-01: Create one validated map target, preserving province index/ID and owner/top-liege separation. **Stage 2; I01–I03.**
- [ ] INPUT-02: Add visible-UI occlusion and click-versus-drag state; preserve left selection and right/middle panning. **Stage 2; I04–I06.**
- [ ] INPUT-03: Connect stationary owner right-click and Shift-right-click to existing character window opening. **Stage 2; I01–I03.**
- [ ] TIP-01: Add campaign province/owner/realm tooltip and supported action hints; single tooltip target with delay/cancellation. **Stage 3; T01–T03,T06.**

##### P2 — Data and structural UI completeness
- [ ] TIP-02: Parse/cache province terrain, climate, religion and culture from confirmed loose data with date/provenance/known flags. **Stage 3; T04,T07.**
- [ ] TIP-03: Bind original tooltip skin, localized runs and viewport-safe layout; preserve unknown military fields. **Stage 3; T05–T06.**
- [ ] VAS-01: Add character-based direct-vassal enumeration across held titles; deduplicate and return total separately from visible page. **Stage 4; V01–V03.**
- [ ] VAS-02: Bind original headers and row name/title/rank/shield layout, count and visible scrollbar. **Stage 4; V03,V06–V07.**
- [ ] VAS-03: Add Tier/Name sorting with deterministic ties and shared draw/hit geometry; keep tax/opinion unknown when unsupported. **Stage 4; V04–V05.**
- [ ] MODE-01: Introduce semantic shared map-mode state and GUI bindings for all 18 campaign buttons, toggle and custom slots. **Stage 5; M01,M07.**
- [ ] MODE-02: Implement campaign Terrain/Realms switching and selection feedback; keep setup-only controls distinct. **Stage 5; M02–M03.**
- [ ] MODE-03: Add Religion/Culture from province facts and original color definitions. **Stage 6; M04. Depends TIP-02,MODE-01/02.**
- [ ] MODE-04: Add de jure Duchies/Kingdoms/Empires from authored hierarchy with explicit dated-change limits. **Stage 6; M05.**

##### P3 — Dependent mechanics and parity
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

#### 11. Recommended first implementation batch
Keep the first batch limited to **HUD diagnosis/correction, correct owner resolution, stationary right-click navigation, and basic province/owner/realm hover text**. It is independently useful and establishes the data/input foundation for later Vassals and map modes. Terrain/climate enrichment, complete list economics and dynamic thematic modes follow as separate stages.

No existing C, header, Makefile or original interface asset has been edited for this planning deliverable. Binary observations support the investigation targets and some display mappings; they do not establish complete equivalence with CK2.

---

<a id="source-external-bloodlines-governments-strings"></a>
## Earlier string-only reconnaissance — preserved from outside the archive

> This initial scan predates the stronger function-level research above. Its observations are string-level unless separately verified. Source: `ck2_binary_bloodlines_governments_report.md`.

# CK2 executable research: bloodlines and governments

## Source and method
Analyzed the supplied `FreeCK2game.exe` (24 MB, Windows PE32+ x86-64), without executing or modifying it. Extracted printable ASCII strings and inspected PE metadata. These are **static string findings**, not recovered function implementations or proof of exact algorithms. PE header linker timestamp: 2021-09-21 17:49:50 (not necessarily the game's release or exact build version). The executable's preferred image base is `0x140000000`; entry point RVA is `0xDC4990`.

## Bloodlines: confirmed internal evidence
- Debug source path: `D:\jenkins\workspace\ck2\ck2\source\bloodline.cpp`.
- Data directory: `common/bloodlines`.
- GUI identifiers: `bloodlines_window`, `button_bloodlines`, `bloodlines_list`, `bloodlines_owner_text`, `bloodline_member_view`, `bloodline_entry`, `bloodline_living_member_button`, `bloodline_icon`, `bloodline_condition` and numbered variants.
- Data/UI fields: `description`, `founder_name`, `modifier`, `modifier2`, `foundation_date`.
- Inheritance and state localization keys: `BLOODLINE_NO_BASTARDS_INHERITANCE`, `BLOODLINE_ACTIVE`, `BLOODLINE_INACTIVE`, `BLOODLINE_MATRILINEAL_INVERSION`, `BLOODLINE_MATRILINEAL_INHERITANCE`, `BLOODLINE_PATRILINEAL_INHERITANCE` and `*_DELAYED` variants.
- Script/effect identifiers: `CREATE_BLOODLINE_EFFECT`, `DESTROY_BLOODLINE_EFFECT`, `ADD_TO_BLOODLINE_EFFECT`, `ADD_BLOODLINE_MEMBER_EFFECT`, `REMOVE_FROM_BLOODLINE_EFFECT`, `REMOVE_BLOODLINE_MEMBER_EFFECT`, `PROPAGATE_BLOODLINE_FROM_EFFECT`, `TRIGGER_NEW_BLOODLINE`, `on_bloodline_renamed`.
- Validation/error strings: `Unknown Bloodline owner:`, `Unknown Bloodline member:`, `Invalid inheritance type in Bloodlines`, `Invalid Bloodline type`, `Invalid scope in add_to_bloodline effect`.

**Implementation implication:** treat bloodlines as entities with founder, date, inheritance policy, member relations, modifiers, and script effects; load their definitions from CK2 loose files, and bind GUI widgets to engine state. This is an architectural inference, not decompiled code.

## Governments: confirmed internal evidence
- Debug source paths: `D:\jenkins\workspace\ck2\ck2\source\government.cpp`, `...\governmentflavor.cpp`.
- Data directories: `common/governments`, `common/government_flavor`.
- Government groups: `feudal_governments`, `theocracy_governments`, `republic_governments`, `tribal_governments`, `nomadic_governments`, `nogovernment`.
- Script/console identifiers: `set_government`, `validate_government`, `set_government_type`, `reloadgovernmentflavor`.
- GUI/map identifiers: `government_icon`, `government_type_image`, `mapmode_governments`, `GFX_mapmode_governments`, `TabGovernment`.
- Rule/description keys: `GOV_CAN_HOLD_DESC`, `GOV_CAN_BUILD_DESC`, `GOV_CANNOT_BUILD_DESC`, `GOV_RAIDING_DESC`, `GOV_VASSAL_CALL_TO_ARMS_DESC`, `GOV_NO_VASSAL_TAX_DESC`, `GOV_CAN_NOT_INHERIT`, `GOV_NO_GENDER_SUCC_LAWS_DESC`, `GOV_USES_BRIDE_PRICE`, `GOV_CONCUBINAGE_DESC`, and other `GOV_*` entries.
- Validation/error strings: `%s is not a valid government. %s`, `Government not found`, `has unknown government`, `Badly scripted government_flavor at %s`.

**Implementation implication:** separate government group/type definitions, character government assignment, scripted eligibility and behavior rules, government-flavor presentation, and government map mode. Avoid hardcoding group membership or gameplay effects where source definitions exist.

## What this does NOT recover
- Function addresses, call graphs, class layouts, inheritance algorithm, save serialization, and exact government rule evaluation are **not established** by a strings scan.
- The EXE contains identifiers and code, but a true function-level recovery requires disassembly/decompilation and xrefs (e.g. Ghidra). The executable is not a substitute for loose-file data definitions.
- No source files were changed as part of this inspection.

## Recommended next steps
1. In Ghidra, import EXE as x86-64 PE, run auto-analysis, locate the strings `common/bloodlines`, `common/governments`, `CREATE_BLOODLINE_EFFECT`, and `set_government`; follow references to identify parser and effect-registration functions.
2. Record each discovered function address, callers, key data structures, and confidence; avoid copying decompiler guesses directly into production.
3. Compare parser keys and rules against actual CK2 `common/bloodlines` and `common/governments` loose files and OpenCK2's current loader architecture.
4. Implement and test one vertical slice per system (parse -> state -> GUI -> save/load -> game rules), then expand to effects and edge cases.


## Dynasty coat-of-arms reconstruction cross-check (12 November 2026)

**Verified executable string:** `FreeCK2game.exe` includes `D:\jenkins\workspace\ck2\ck2\source\dynasty.cpp`. This establishes a source-path reference but does not disclose implementation details, original RNG state, shield template ordering, pixel shader or class layout. No original `dynasty.cpp` or matching PDB was recovered. **Do not cite the C approximation below as the original function's reconstructed algorithm.**

**Verified original loose-file evidence, not executable disassembly:** `common/dynasties/00_dynasties.txt` records an authored Coat of Arms for dynasty ID 106 (`Plantagenet`) with `data={0 3 0 34 0 0 0}`. `interface/coat_of_arms/coats_of_arms.txt` identifies style group `christian`, its fourth (index 3) `dynasties.tga` atlas with `size={x=16 y=4}`, `noOfFrames=51`, `color=0`; `interface/coats_of_arms.gfx` defines original `CoatOfArmsType` templates/masks and points to `gfx/FX/coa_shield.lua`. The seven-integer format is supported by the source schema and has been parsed for authored definitions, but the exact shader/compositing logic has not been recovered from the executable.

**Reconstruction boundary:** `src/game/dynasty_arms_core.c` and `dynasty_arms.c` implement the verified *data path* and a provisional CPU pixel compositor. Authored atlas selection is supported by the supplied source definitions; exact alpha/color blending, optional emblems and autogenerated random designs remain hypotheses or pending functionality. To advance genuine 1:1 fidelity, inspect the user's installed `gfx/FX/coa_shield.lua` and actual texture files, compare output pixels against original CK2, and only then revisit executable cross-references for RNG and dynasty metadata generation.


## November 12, 2026 — Genealogy interface executable cross-references (new evidence)

**Evidence:** The supplied `FreeCK2game.exe` has image base `0x140000000` (64-bit PE). The table records preferred VAs from independent `objdump -d -M intel` RIP-relative string cross-references, not recovered original C++ function names. Under ASLR, rebase by RVA. These cross-references are **verified references to UI names, not evidence that CK2's original layout, ranking, genealogy or prestige algorithms have been reconstructed**.

| Referenced string | Preferred string VA | Example instruction referencing string | Confidence and interpretation |
| --- | --- | --- | --- |
| `OPEN_DYNASTY_TREE` | `0x14103a730` | `0x14051eb4f` | High: original localization/action identifier used by executable |
| `DynastyTree` | `0x14103b0d0` | `0x14052c222`, `0x14052cd17`, `0x140531095` | High: GUI tree root looked up in multiple call sites; methods unidentified |
| `DynastyTreeBranch` | `0x14103b3c0` | `0x14053130a` | High: branch widget lookup; branch layout behavior not recovered |
| `FamilyTree` | `0x14103b5e8` | `0x140535dcc`, `0x140537eaf` | High: separate family view widget |
| `RealmTree` | `0x14103b9f8` | `0x14053a72c` | High: separate realm view widget |
| `RealmTreeBranch` | `0x14103ba98` | `0x14053f553` | High: realm-branch widget lookup |
| `button_realmtree` | `0x1410393f0` | `0x1404fc475`, `0x1404fd364` | High: character interface control binding |

**Original loose-file corroboration:** `interface/dynasty_tree.gui` defines `DynastyTree`, `FamilyTree`, and `RealmTree`; related sprite metadata is in `interface/dynasty_tree.gfx`. Localization `v2_42.csv` includes `DYNASTYTREE_INTERFACE_LABEL`, `FAMILYTREE_INTERFACE_LABEL`, `REALM_TREE_INTERFACE_LABEL`; other CSV files define house name and member-count text. These identifiers are sufficient to connect the right interface assets and text keys but insufficient to copy CK2's internal layout algorithm.

**OpenCK2 C implementation:** `src/game/tree_history.c` parses CK2 character history; `src/game/tree_view.c` renders a bounded, original-file-backed graph using window/sprite names from loose files. The rules for branch placement, navigation, dynasty prestige, historical members with incomplete dates and recursive realm vassals are **new OpenCK2 implementations/approximations**, not decompiled CK2 functions. The supplied binary was **not executed**. Follow up with Ghidra or disassembly call graphs for `0x14052c222`, `0x140535dcc`, `0x14053a72c` to investigate layout/branch algorithms.
