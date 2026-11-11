# OpenCK2

A Linux C reimplementation project for Crusader Kings II. The current
build includes a native main menu and a launcher. It reads game assets from
your local CK2 installation.

## OpenCK2Launcher

From the repository root, build with GNU Make, a C17 compiler, pkg-config,
and the SDL2, SDL2_image, and SDL2_mixer development packages. The graphical
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
checkbox beside Mods controls both lists. A dash indicates mixed selections,
and clicking it enables everything.

Selections are saved separately in `content-selection.txt` in the launcher
preferences directory. OpenCK2 does not mount DLC archives or mods yet.
Use the original launcher to apply content to the original game or continue
a save.

## Native main menu

Build from the repository root with GNU Make, a C17 compiler, pkg-config,
SDL2, SDL2_image, and SDL2_mixer development libraries. The executable uses the assets
from your existing CK2 installation at runtime.

```sh
make game
./bin/ck2-game
```

The default asset directory is
`$HOME/.steam/steam/steamapps/common/Crusader Kings II`. For a different
Steam library, pass its game directory with `--assets PATH` or set `CK2_DIR`.

The menu uses the installed terrain colormap, title and panel sprites,
Garamond bitmap font, and credits file. Options toggles fullscreen for the
current session and can mute or resume the looping CK2 main theme. Credits scroll with the wheel, arrow keys or Page Up and
Page Down. Use the mouse or arrows and Enter to navigate. Escape goes back
or exits from the main menu.

The terrain is a static 2D crop. The original 3D camera and live banners
remain pending, and Single Player and Multiplayer are disabled. The native
executable starts directly at its main menu.

## Steam reference launcher

Build and run from the repository root with GNU Make, a C17 compiler and the Linux Steam client on `PATH`. CK2 must already be installed in Steam.

```sh
make launcher
./bin/ck2-launcher
```

The launcher hands app ID 203770 to Steam. Steam locates the local game and applies its existing compatibility settings, including the selected Proton version. Once Steam starts, it owns the launch and reports any game startup errors.

Use `./bin/ck2-launcher --help` for usage.

The launcher entry point lives in `src/launcher/main.c`, with its build recipe in `Makefile`. This follows the separation of source and build tooling in [fnv_decomp](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6). The [title-history tool](title-history.c) remains a standalone C program. The [roadmap](ROADMAP.md) tracks the wider CK2 project.

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

See [launcher decompilation milestones and evidence](LAUNCHER-DECOMP.md) for the recovered startup, lifecycle, and content-selection logic.
