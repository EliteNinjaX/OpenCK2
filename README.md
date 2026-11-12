# OpenCK2

### Documentation

- [Roadmap](ROADMAP.md) — active priorities and compatibility goals.
- [Executable research](docs/EXE_RESEARCH.md) — recovered CK2 function addresses, debug filenames, disassembly notes and provenance.
- [Game data and interface](docs/GAME_DATA_AND_UI.md) — loose-file loaders, gameplay mechanics and UI tasks.
- [Development and tests](docs/DEVELOPMENT.md) — consolidated progress, Qwen notes, deferred C++ ideas and regression checklists.
- [AI/contributor instructions](AGENTS.md) — shared workflow.


OpenCK2 reconstructs the original Crusader Kings II engine in C17. Its
fidelity target is the experience of a native Windows, Linux, and macOS
port of the original engine. The current build runs on Linux and reads
assets and data from your local CK2 installation.

Start with the [development notes](docs/DEVELOPMENT.md) and the
[roadmap](ROADMAP.md) for project status and milestones.
[AGENTS.md](AGENTS.md) covers the shared workflow for humans, local AI,
Codex, and Claude Code. The [executable research](docs/EXE_RESEARCH.md)
indexes prior findings and gives contributors with or without C experience
a starting point.

## OpenCK2Launcher

From the repository root, build with GNU Make, a C17 compiler, pkg-config,
and the SDL2, SDL2_image, SDL2_mixer, and zlib development packages. The graphical
folder picker uses Zenity.

```sh
make open-launcher
./bin/OpenCK2Launcher
```

The launcher reads its backdrop, panels, buttons, and bitmap font from
your local installation. A locally supplied `assets/openck2-background.png`
overrides the original backdrop when building. The DLC tab lists
expansions first, with collapsible categories for other packs. DLC absent
from the local descriptors and Steam installed-depot records has a gray
row. Hover over that row to open its Steam Store page. The launcher does
not query Steam account licenses, so an owned but uninstalled pack can
also appear there. The Mods tab lists locally discovered descriptors. **Settings → Choose Folder** opens a Zenity
directory picker. Startup opens the picker only when a valid installation
cannot be detected or restored. A valid selection is remembered in
`$XDG_DATA_HOME/OpenCK2/Launcher/install-path.txt` (default
`~/.local/share/OpenCK2/Launcher/install-path.txt`). Without a saved choice,
the launcher checks Steam’s standard install roots and configured libraries
for app 203770. Use `--assets PATH` or `CK2_DIR` for a one-run override.

**Play** starts the adjacent `ck2-game` executable with the selected
`--assets` path. Use Tab or Shift+Tab to focus buttons, Enter to activate
them, and Escape to go back or quit. Scroll the content lists with the wheel,
arrow keys, or scrollbar. Check a row to change its saved selection. The
checkbox beside Mods applies to the current tab. A dash indicates a mix of
enabled and disabled entries in that tab. The launcher window is fixed at
1000 × 600.

Selections are saved in `content-selection.txt` in the launcher preferences
directory. On Play, OpenCK2 mounts enabled local DLC ZIPs and mod directories
or ZIPs from their descriptors. Mods take precedence over DLC and base files.
The mounts provide file overrides and `replace_path` for the game's implemented
asset and data loaders. Content changes in the in-game editor apply on the
next launch. CK2 script execution, arbitrary GUI layouts, and full
conversion compatibility remain incomplete.

## Native main menu

Build from the repository root with GNU Make, a C17 compiler, pkg-config,
SDL2, SDL2_image, SDL2_mixer, and zlib development libraries. The executable uses the assets
from your existing CK2 installation at runtime.

```sh
make game
./bin/ck2-game
```

The default asset directory is
`$HOME/.steam/steam/steamapps/common/Crusader Kings II`. For a different
Steam library, pass its game directory with `--assets PATH` or set `CK2_DIR`.

The menu uses the installed terrain colormap, title and panel sprites,
Garamond bitmap font, and credits file. **Options** has Game, Video, Audio,
and Controls tabs. Apply changes the windowed resolution, window mode, and
music and effect volume for the current session. The [settings TODO](docs/GAME_DATA_AND_UI.md#source-docs-settings-todo-md)
tracks the remaining controls. Credits scroll with the wheel, arrow keys,
or Page Up and Page Down. Escape goes back or exits from the main menu.

The main menu shows expansion icons and a bottom-left OpenCK2 version plate.
**Content Settings** opens the launcher’s content editor and refreshes the
icons when it closes. Saved DLC and mod changes apply on the next launch. Manual, Paradox Plaza, Reddit, and GitHub open in the browser.

## First playable scenario

Choose **Single Player → New Game** to browse CK2's five starting eras.
Select an era and click **Start Game** to open **Game Rules**, or choose
**Custom Game Setup** to inspect the starting map first. Press **W** to
switch between Realms and Terrain in the preview or campaign. Drag with
the right or middle mouse button or use the arrow keys to pan. Scroll to
zoom, or press **Home** to view the whole installed map. In Custom Game
Setup, click a featured ruler or county to inspect the ruler's culture,
religion, and dynasty. Alt-click opens the county holder's primary title,
and Shift-click opens the independent realm. The right panel supports
liege, vassal, and holding selection. Menus and ruler selection play the installed CK2
sound effects. **Options → Audio → Effect** controls their volume.
The rules screen supports category filters, a local preset, and default reset.
Its settings are available for configuration while their simulation effects
are pending. **Continue** resumes the current prototype campaign until you
quit OpenCK2. See the [starting-era notes](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-starting-eras-md) for the REA
findings and installed data sources. Click land provinces to inspect them.
The prototype goal is to upgrade your capital and build a treasury of
100 gold. The 1066 start defaults to York when you leave the ruler unselected.

- **Space** pauses or resumes the daily clock.
- **Next Month** pauses and advances to the first of the next month, when
  you collect income.
- **Improve Holding** spends 25 gold and adds 2 gold to monthly income. The
  prototype starts with 50 gold and earns 5 gold per month before upgrades.
- **Main Menu** or **Escape** returns to the menu. Single Player resumes the
  same campaign during the current session.

The province shapes and names come from the local CK2 install. The economy
is a prototype with fixed rules. Saving campaigns and multiplayer are pending.

The menu and campaign render the installed CK2 terrain layers in perspective,
including elevation, lighting, forests, rivers, and animated water. See the
[map investigation](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-map-rendering-md) for the REA evidence and remaining
differences.

## Steam reference launcher

Build and run from the repository root with GNU Make, a C17 compiler and the Linux Steam client on `PATH`. CK2 must already be installed in Steam.

```sh
make launcher
./bin/ck2-launcher
```

The launcher hands app ID 203770 to Steam. Steam locates the local game and applies its existing compatibility settings, including the selected Proton version. Once Steam starts, it owns the launch and reports any game startup errors.

Use `./bin/ck2-launcher --help` for usage.

The launcher entry point lives in `src/launcher/main.c`, with its build recipe in `Makefile`. This follows the separation of source and build tooling in [fnv_decomp](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6). The [title-history tool](tools/title-history.c) remains a standalone C program. The [roadmap](ROADMAP.md) tracks the wider CK2 project.

## Original launcher

The local Windows Steam build 7399341 reports CK2 3.3.5.1 (SOHY). Steam
launches `CK2game.exe` through Proton 10.0. It is the only executable in
the game directory. The original pre-game screen runs in that executable.
The `launcher/` directory supplies the screen’s assets and interface definitions.

For an independent C rebuild, the reference is the original pre-game screen,
including the DLC and Mods tabs, their selection controls, the version and
checksum, and the Quit, Continue and Play actions. The shipped
`bin/ck2-launcher` opens the original through Steam. OpenCK2Launcher implements the local folder, content catalog, and Play
workflow. Continue and native content loading remain pending.

The local layout is defined in `launcher/interface/main.gui` and
`launcher/interface/main.gfx`, with text in `localisation/z_launcher.csv`.
The layout defines a 1000 by 600 design surface.

See [launcher decompilation milestones and evidence](docs/EXE_RESEARCH.md#source-docs-reverse-engineering-launcher-decomp-md) for the recovered startup, lifecycle, and content-selection logic.

## License

OpenCK2 project code is licensed under the GNU General Public License,
version 3. See [LICENSE](LICENSE) (`GPL-3.0-only`). CK2 game files and other
third-party materials retain their respective licenses. OpenCK2 reads the
player's installed CK2 files at runtime.
