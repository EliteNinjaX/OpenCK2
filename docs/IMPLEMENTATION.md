# OpenCK2 implementation inventory

OpenCK2 is a C17 engine reconstruction. The current build runs on Linux and
reads the player's local CK2 installation. The [roadmap](../ROADMAP.md) tracks
parity and the Windows and macOS targets. The [vanilla mechanics guide](CK2-MECHANICS.md)
describes the reference game separately from the current implementation.

## Executables and content

| Feature | Implementation and current limit |
| --- | --- |
| Steam reference runner | [`src/launcher/`](../src/launcher) starts the original Steam app. The separate startup, lifecycle, and content models cover the recovered bypass branch, launcher actions, Continue filename selection, and selection restoration. [REA evidence](reverse-engineering/LAUNCHER-DECOMP.md) |
| OpenCK2Launcher | [`src/open_launcher/main.c`](../src/open_launcher/main.c) implements the fixed-size launcher, local CK2 styling, separate DLC and Mods master toggles, categories, store links, and the installation picker. Play starts the adjacent native game. |
| Installation and catalog | `install.c` finds and remembers the Steam installation. `catalog.c` discovers descriptors and persists selections. DLC availability is based on local files and installed-depot records. Steam account entitlement checks remain pending. |
| Content resolver | `asset_paths.c` and `zip_assets.c` mount enabled DLC and folder or ZIP mods at startup. Mods take precedence over DLC and base files. Supported reads use the overlay and honor `replace_path`. Arbitrary CK2 script execution, full conversion support, and full load-order parity remain pending. |

## Frontend and campaign

| Feature | Implementation and current limit |
| --- | --- |
| Main menu, credits, and settings | [`src/game/main.c`](../src/game/main.c), `assets.c`, and `settings.c` draw the installed frames, title art, bitmap fonts, DLC icons, and version plate. Options applies window mode, windowed resolution, and volume controls during the session. See the [settings backlog](SETTINGS-TODO.md) for unfinished controls. |
| Music and sound | `music.c` plays the local main theme and loads cues from the installed sound definitions. Ruler selection uses the country-selection clips. Ambient and contextual playback remain pending. |
| Single Player and rules | `era.c`, `rules.c`, and `ck2_script.c` read five era bookmarks, localization, featured rulers, biographies, and game-rule definitions. The rules screen supports filtering, changing options, reset, and a local preset. Rule effects, Ironman, and achievements remain pending. |
| Setup layout | `frontend_gui.c` indexes the installed frontend tree. `setup_toolbar.c` binds widgets across the four setup panels and resolves their sprites through the installed `.gfx` definitions. See the [loose-file compatibility audit](LOOSE-FILES.md) for coverage and exceptions. |
| Historical setup | `realms.c`, `rulers.c`, and `holdings.c` read dated title, character, and province history. Setup distinguishes county holder, primary title, and independent realm. The panels expose culture, religion, dynasty, lieges, vassals, and dated holding slots. Barons without recorded holders are still unassigned. |
| Portraits | `portraits.c` and `era.c` use installed sprite and portrait definitions, DLC, age and period variants, DNA, properties, and rank frames. Historical portraits and bookmark thumbnails can use different properties. Scripted clothing and dynamic dynasty arms remain incomplete. |
| Terrain and Realms | `map_view.c` and `realms.c` render a pannable, zoomable map, CPU terrain and water shading, projected picking, political colors, province and sea-zone borders, and territory-based labels. The map uses the starting historical snapshot. The full vanilla rendering pipeline and live political changes remain pending. |
| Campaign prototype | `campaign.c` supplies daily advancement, fixed monthly income, and holding upgrades. Continue resumes the campaign in the current process. Save compatibility, the full economy, succession, diplomacy, combat, events, and AI remain milestones. |

See [starting-era and UI evidence](reverse-engineering/STARTING-ERAS.md)
and [terrain and water evidence](reverse-engineering/MAP-RENDERING.md) for
the installed inputs, REA findings, and known differences.

## Build and verification

Run from the repository root.

```sh
make launcher game title-history check-launcher check-campaign CFLAGS='-O2 -Werror'
```

The tests in [`tests/`](../tests) cover launcher model branches and the
prototype campaign. The standalone [title-history probe](../tools/README.md)
checks a dated holder in a local game file. Visual and gameplay parity
requires matched reference scenarios. The [evidence template](reverse-engineering/FINDING-TEMPLATE.md)
records the build, inputs, results, and remaining differences.
