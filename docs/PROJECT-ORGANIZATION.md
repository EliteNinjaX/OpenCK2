# Repository organization

The reference is [momusdigital/fnv_decomp at 91e1b6e](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6).
Its matching targets are listed in a per-unit manifest and checked with the
original compiler toolchain. OpenCK2 adopts its separation of implementation,
tooling, build recipes, and evidence. Our current acceptance checks target
documented behavior and reference scenarios.

| Reference directory | What it contains | OpenCK2 practice |
| --- | --- | --- |
| [`src/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/src) and [`include/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/include) | Reconstructed C++ implementation units and reusable declarations. | Keep C17 runtime code under `src/` by subsystem. Private headers stay with their implementation. A public header directory can follow an actual cross-module API. |
| [`tools/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/tools) | `msvc.py` runs historical compiler tools, `build_and_match.py` builds units and compares them against a local executable, `compare.py` checks relocations, `function_spans.py` defines function extents, and `compare_builds.py` checks changed units against the base build. | The independent title-history probe lives under [`tools/`](../tools/README.md), with a dedicated Make target. Add repeatable scenario and comparison tools here as they become available. |
| [`build/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/build) | Tracked metadata includes `units.json` for source files, compiler options, and function bindings, and `image_check.json` for reference image checks. | The root Makefile is the current build recipe. Keep generated files under ignored `bin/`. Add versioned reference manifests when comparisons need machine-readable inputs. |
| [`.github/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/.github) | Workflow and scripts for build comparisons. The pull request check uses two builds and can run without the commercial game files. | Keep automated checks under `tests/` and separate synthetic checks from local CK2 comparisons. Cross-platform CI is a roadmap milestone. |
| [`third_party/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/third_party) and [`patches/`](https://github.com/momusdigital/fnv_decomp/tree/91e1b6e8d8bb0f9a33e8cf88e77268757449e0a6/patches) | The reference vendors a specific zlib version, documents its license, and records changes in a separate patch. | OpenCK2 currently uses system dependencies. If vendoring becomes necessary, record the upstream version, license, and local patches together. |

The current layout separates runtime code in `src/`, automated regression
checks in `tests/`, independent probes in `tools/`, and prose in `docs/`.
Reference notes and reusable investigation evidence live in
[`docs/reverse-engineering/`](reverse-engineering/README.md). The project
roadmap and contributor entry points stay at the root. No reference-project
code was imported.
