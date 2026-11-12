# OpenCK2 Bloodlines and Governments: incremental research and implementation

## Scope and limits

Static analysis of the supplied 64-bit Windows executable, uploaded C source, and interface definitions. No original executable was executed. No source-level symbols were recovered: function names below describe inferred roles. Absolute addresses apply only to this executable; subtract image base 0x140000000 for RVAs. Function ranges come from PE exception metadata. ASLR changes runtime addresses.

Executable SHA-256: a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d.

This is an incremental implementation, not complete functional equivalence. In particular, the new definition catalog is an API awaiting runtime/UI wiring. No bloodline membership, births, propagation, government trigger evaluator, or full government assignment has been implemented.

## Method and reproducibility

`research/trace_binary.py` reads PE exception metadata and disassembles each covered text function using Capstone. It finds direct RIP-relative and immediate references to selected strings. Output: 69 matching strings and 99 instruction references in `xrefs.json`; containing function disassembly in `disassembly.txt`. `annotate.py` provides focused disassembly with readable RIP-relative strings in `focused.txt`. Install Python `pefile` and `capstone`, place the original file at `inputs/FreeCK2game.exe`, and run scripts from their parent workspace layout. Searches do not recover indirect/vtable references or functions without unwind coverage. Unrelated inheritance references are retained as context, not classified as bloodline logic.

## Located functions and confidence

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

## Verified bloodline parser behavior

The token-registration routine at 0x14007e3f0 associates strings with numeric IDs. At 0x14008f682 it registers `inheritance` as 0x307f. The definition dispatcher branches on that ID at 0x140181b70/0x140181b7d to 0x140181f2d. There it compares the parsed value token and writes a 32-bit field at object+0x45c:

| Loose-file value | Token ID | Stored enum | Branch/store evidence |
|---|---|---|---|
| matrilineal | 0x2e4d | 1 | registration 0x14008c412; store 0x140182014 |
| patrilineal | 0x3675 | 2 | registration 0x1400983fa; store 0x140182007 |
| all_descendants | 0x3676 | 0 | registration 0x140098412; store 0x140181ffe |
| no_inheritance | 0x368e | 3 | registration 0x140098652; store 0x140181fec |

Confidence: high. The second parser writes the same enum choices at object+0x158 (0x1401837c4–0x1401837f1). Invalid choices produce diagnostics, rather than visibly selecting an arbitrary fallback in the examined branch. Original constructor defaults remain unknown. OpenCK2 therefore returns UNKNOWN for missing/unrecognized inheritance, rather than fabricating a default.

These enum values establish allowed input and representation. They do not establish sex filtering, bastard exclusions, marriage inversion, adoption, retroactive propagation, or special event exceptions. Tooltip strings mention patrilineal, matrilineal, inversion and no-bastards behavior, but do not prove those algorithms.

## Data structure observations

Confidence high for instruction-level offsets, medium or lower for semantic labels. Offsets are build-specific and are not copied into OpenCK2 structs.

- Definition inheritance is a dword at +0x45c. Three boolean-reader branches address +0x458, +0x459 and +0x45a at 0x140182043–0x140182063. `matrilineal_override` is token 0x3771 and routes to +0x45a; other boolean names need registration tracing.
- Runtime bloodline parser stores inheritance at +0x158. A character-like pointer is written at +0x100 at 0x14018367e; owner/member diagnostics and validity checks support character-reference handling, but exact field identity requires complete control-flow tracing.
- Government parsing writes many bits in bytes +0x196c through +0x1970. Do not assume one C boolean per option. At 0x1407af052 it reads an integer into +0x193c, tests it against zero at 0x1407af061, and emits “AI aggression cannot be negative” on failure. The field's semantic identification is high confidence from this branch.
- Flavor loader allocates a 0x20-byte collection holder (0x1407b69fa onward), clears three pointer-sized slots and a dword, enumerates `.txt` files and validates parsed entries. This is a collection wrapper, not proof that each flavor record is 0x20 bytes. Selection weighting and ordering remain unresolved.

## Government assignment status

The source previously has only a Governments map-mode identifier. It has no government rule engine. Executable strings include `set_government_type`; registration and references do not prove how the default government is chosen. No priority ordering, liege inheritance rule, holding classification, or potential-trigger behavior is claimed as verified.

The patch retains an explicitly authored `government` scalar from character history and applies dated scalar overrides at or before the bookmark, with latest-date precedence independent of input ordering. This is an OpenCK2 history-reader extension following existing code conventions, not a proven reconstruction of original government assignment. Unknown/absent assignment remains empty. Scripted `set_government_type` effects are not executed. Title history assignment is not added.

## GUI connections

The supplied original definitions provide:

- `bloodlines.gui`: `bloodlines_window`, `bloodlines_list`, `bloodline_entry`, `bloodline_portrait`, `bloodline_icon`, condition controls and `bloodline_living_member_button`.
- `bloodlinememberview.gui`: `bloodlinememberwindow`.
- `domestic_dynastic.gui`: `button_bloodlines` with `GFX_bloodlines_button`, and `government_icon` with a default `GFX_icon_feudal_government`.

Executable cross-references above connect these names to UI routines. The feudal sprite in the GUI is a layout default, not evidence that all rulers should be assigned feudal government. The patch adds no hardcoded replacement windows or icons. Rendering and click callbacks remain TODO.

## Implemented changes

1. `src/game/definitions.c/.h`: owned catalog of keys, government groups and losslessly retained block bodies. Uses the existing CK2 tokenizer and mounted asset resolver, reads `.txt` files from common/governments or common/bloodlines, retains unknown nested fields, and rejects incomplete braces. Parse calls are transactional. Later same-key definitions replace earlier entries. This override policy and grouped government format are provisional engine choices pending actual gameplay files; not binary-verified semantics. Directory load can partially succeed and returns false if any file fails; empty/missing directories currently return true with an empty catalog.
2. Verified explicit inheritance enum decoder, without implementing unverified propagation or defaults.
3. `rulers.h/.c`: retain explicit government history values. Buffer size 128 follows existing bounded key style; long values truncate and require future diagnostics.
4. `tests/`: parser regression fixtures and authored government history tests, using the real tokenizer and ruler parser with a clearly marked test-only SDL shim.

Call `ck2_definitions_load(&catalog, root, true)` for governments and false for bloodlines after mounting content. Initialize catalog to zero, inspect return status, query via `ck2_definition_get`, and free it during teardown. Add definitions.c to the real build target. The uploaded ZIP has no build configuration, so automatic target wiring cannot be supplied or verified here. No active gameplay caller is added for the catalog yet.

## Validation

Ran `ASAN_OPTIONS=detect_leaks=0 sh tests/run_definitions.sh` from source: passed. Both test executables compile as C11 with -Wall -Wextra -Werror, AddressSanitizer and UndefinedBehaviorSanitizer. Tests cover grouped records, nested/unknown data retention, same-key replacement, malformed input rollback, unmatched close brace, all four inheritance values, unknown inheritance, dated government precedence, future-date exclusion and absent assignment reset. Ruler tests compile the complete rulers.c translation unit and invoke its actual static history parser; unused external functions are discarded at link time.

LeakSanitizer cannot run under this environment's tracing setup and was disabled after it reported that limitation. This is not a successful leak check. Test shim does not validate SDL ABI. Full engine compilation and rendering are blocked by absent build files and SDL development libraries. No real common/governments, common/bloodlines, history/bloodlines or playable game installation was supplied, so fixture tests do not demonstrate compatibility with all original files or DLC/mod precedence.

## Remaining TODOs, in order

1. Obtain the actual loose gameplay definitions and original project build files. Validate file layout, mounted content order and duplicate semantics; add load failure diagnostics and integration tests.
2. Wire catalogs into engine initialization/teardown and the real build target. Do not treat successful loading as support for rules execution.
3. Complete token-registration mapping for bloodline fields; trace constructors, owner/member containers, and callers of both parsers. Recover history/bloodlines loading, founders, creation/death dates and membership.
4. Trace propagation callers around births, marriages and scripted effects; establish exclusions and inversion with instruction-level evidence and tests.
5. Map government parser token IDs to field bits and trigger blocks. Recover assignment, validation, holding restrictions and script-effect execution; implement an explicit supported trigger subset and report unsupported conditions.
6. Trace government flavor entry parsers and selection criteria; retain original flavor definitions rather than inventing title/name substitutions.
7. Bind original GUI definitions and sprites to catalog/character state; connect close, list selection and living-member actions. Validate screenshots against the original game only after data and rendering work.
8. Test full game builds, bookmark/history behavior, DLC combinations, mods, save/load and long-running simulation before making compatibility claims.

## Deliverables

Modified source ZIP contains the entire uploaded source tree plus incremental changes and tests. Research bundle includes this report, machine-readable xrefs, full relevant function disassembly, focused annotated disassembly and reproducible Python analysis scripts. Neither deliverable redistributes the original executable or interface archive.
