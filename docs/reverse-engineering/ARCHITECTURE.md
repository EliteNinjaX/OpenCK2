# CK2 architecture investigation

This map records evidence about original CK2 and identifies the current
OpenCK2 source boundaries. Treat the open entries as investigation tasks.
Use the pinned build and confidence conventions in the [knowledge base](README.md).

| Boundary | Original CK2 evidence | OpenCK2 code |
| --- | --- | --- |
| Startup to launcher | Binary-supported startup branch, event-loop result, and Continue filename handoff. [Launcher evidence](LAUNCHER-DECOMP.md) | `src/launcher/` models the recovered behavior. `src/open_launcher/` is the native launcher and content resolver. |
| Content to data loaders | Installed descriptors and observed launcher selections support DLC and mod discovery. General CK2 load order and object ownership remain open. | `src/open_launcher/asset_paths.c`, `zip_assets.c`, and `src/game/ck2_script.c` |
| Bookmark to initial world | Installed bookmark and history records define inputs. Executable string anchors identify relevant screens and directories. Initialization call order and live object lifetimes remain open. [Starting-era evidence](STARTING-ERAS.md) | `src/game/era.c`, `realms.c`, `rulers.c`, `holdings.c`, and `campaign.c` |
| World to renderer and UI | Installed interface and shader definitions, with a binary-supported terrain-function trace. The full pass order and layout engine remain open. [Renderer evidence](MAP-RENDERING.md) | `src/game/map_view.c`, `realms.c`, `portraits.c`, and `main.c` |
| Day advancement and save/load | The original simulation scheduler, ownership graph, RNG streams, and world serialization have no verified reconstruction in these notes. | `src/game/campaign.c` has a prototype daily clock and economy. |

## Next architecture probe

Use 1 January 769 and record the DLC profile, ruler, title, and county. Follow
one bookmark-selection action to initial world construction. Identify callers,
the order in which dated character, title, and province records are applied,
where stable IDs become runtime objects, and where initialization hands off to
the first simulation tick. Check the same hypothesis against a 1066 start.
Leave an ordered trace with addresses and confidence levels, and record one
specific unresolved branch. This is roadmap task **P1.3**.

## Code boundaries to establish as evidence accumulates

Keep historical data and deterministic simulation callable without SDL.
Separate platform paths, directory walking, archive and process services,
Steam discovery, windowing, and timing from simulation. Confirm these
boundaries through narrow changes to the existing C17 code and matching
scenario checks before moving more systems.
