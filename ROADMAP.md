# Local Crusader Kings II reimplementation in C

The first target is a C engine that reads the local base-game data and reproduces selected CK2 behavior under controlled scenarios. “Byte-for-byte” has two possible targets. Matching specific serialized files can be tested once their format and nondeterministic fields are understood. Matching the Steam executable itself would require reconstructing the original build inputs, including the compiler, linker, flags, dependencies, and packaging. Exact executable identity is an exploratory milestone with no demonstrated build path.

## Stage 0 — Pin and launch the reference build

**Done.** Steam app 203770, build 7399341, reached the main menu through the existing Proton 10.0 configuration. The menu reports CK2 3.3.5.1 (SOHY). The local reference is the Windows x86-64 PE executable `CK2game.exe`.

- Install directory: `$HOME/.steam/steam/steamapps/common/Crusader Kings II`
- Executable SHA-256: `a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d`
- Launch again with `steam -applaunch 203770`.
- For controlled gameplay comparisons, record the version, game checksum, active DLC and mods, scenario date, and inputs before running each scenario.

## Stage 1 — Set up local static analysis

REA 6.8.0 runs with Node 24.22.1 under `work/rea-runtime`. The project uses the REA CLI directly. Kuna 1.990 rejected CK2’s PE format, so this project uses the Ghidra provider for the Windows build. Use the [REA CLI guide](https://github.com/morluto/rea/blob/ada8fcfe82f5c6fe7ffe304261a9b249e39766fb/docs/getting-started/cli.md) for provider commands.

Ghidra 12.1.4 is installed under `work/ghidra_12.1.4_PUBLIC`. REA's Ghidra doctor passed. Ghidra imported the executable, but its initial auto-analysis exceeded REA’s default 330-second startup deadline. A second run completed the REA image inventory with a 900-second deadline. See the [launcher decompilation milestones](LAUNCHER-DECOMP.md) for the startup branch recovered from this build. From the original analysis workspace, after installing the tools under `work/`, the retry command is:

```sh
export PATH="$PWD/work/rea-runtime/node_modules/.bin:$PATH"
export GHIDRA_INSTALL_DIR="$PWD/work/ghidra_12.1.4_PUBLIC"
export XDG_CACHE_HOME="$PWD/work/rea-cache"
export REA_GHIDRA_STARTUP_TIMEOUT_MS=900000
export REA_GHIDRA_CHECKPOINT_DIR="$PWD/work/ghidra-checkpoints"
mkdir -p "$REA_GHIDRA_CHECKPOINT_DIR"
CK2="$HOME/.steam/steam/steamapps/common/Crusader Kings II"
rea doctor --provider ghidra --json
rea analyze "$CK2/CK2game.exe" --provider ghidra --json > work/ck2-ghidra-retry.json
```

**Exit check.** Import the pinned CK2 executable through REA. Save the result and tie every subsequent finding to the build hash, REA and provider versions, and the relevant function address or data location. Use the disassembly to answer specific questions from the gameplay comparisons.

## Stage 2 — Read base-game data

**Started.** The C17 title-history reader resolves the most recent dated `holder` assignment in one title file at a requested date. It handles comments, quoted values, nested blocks, and CRLF. This is a data-file experiment. In-game holder selection, DLC overlays, and scripted effects still need verification.

From the repository root:

```sh
mkdir -p bin
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  title-history.c -o bin/title-history
CK2="$HOME/.steam/steam/steamapps/common/Crusader Kings II"
./bin/title-history "$CK2/history/titles/c_york.txt" 1066.9.15
```

Expected output is `holder=5660 effective=1066.1.1` (Morcar). At that cutoff, 2,805 of 2,807 local title files parsed. The parser reports an error for the two files with unterminated blocks, `d_scholae_palatinae.txt` and `d_varangian_guard.txt`. Observe how CK2 handles those files before choosing a recovery rule.

**Exit check.** Compare a small table of title holders against CK2 on at least two bookmark dates. Expand the parser to cover character, province and title relations once these comparisons agree.

## Main menu milestone

**Done.** The native C executable reaches a standalone main menu using the
local CK2 install's art, bitmap font, and credits. The map now uses a
[perspective terrain renderer](MAP-RENDERING.md) based on the local assets. Options, Credits, and Quit work, while gameplay entry points remain
disabled. See the [build instructions](README.md).

## Stage 3 — Build a headless simulation

**Started.** Single Player runs a small York scenario with a daily clock,
province selection, monthly income, and holding upgrades. The scenario uses
local map data and prototype economy rules. Its calendar and treasury tests
run with `make check-campaign`.

Implement the date loop, stable IDs, characters, titles, and a small succession scenario in C. Advance one day at a time and compare each resulting state against the original under fixed inputs. Use REA for specific questions such as tick ordering and succession tie-breaking when observations leave multiple explanations.

**Exit check.** A reproducible scenario loads a small realm, advances days, changes a title holder through succession, and matches the corresponding CK2 observations.

## Stage 4 — Add saves, rules and a playable slice

Read one plain-text base-game save and compare parsed world state. Add a writer, reopen its output locally in CK2, then compare bytes where ordering and metadata are stable. Extend the script interpreter, economy, diplomacy and AI one scenario at a time. Add a minimal map and character view after the headless loop is repeatable.

**Exit check.** Load a scenario, inspect and advance the realm, save, reload, and compare the resulting state with CK2.

## Stage 5 — Measure parity

**Byte matching is on hold.** The current priority is a runnable native build and observable behavior.

Track differences independently for simulation state, serialized bytes and the executable. Expand scenario coverage across dates and systems before adding DLC or mods. For executable identity, first test whether a small identified routine can be compiled to identical machine code using a plausible toolchain. Whole-file identity would require the resulting executable SHA-256 to match the pinned Steam build.
