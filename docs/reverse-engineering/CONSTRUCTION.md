# Holding construction evidence

Reference build is CK2 3.3.5.1 (SOHY), Windows Steam executable. Use the
binary hash and preferred base in the [evidence index](README.md) before
reusing the VAs below.

## Observed in installed files

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

## Binary-supported findings

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

## Implemented and checked in OpenCK2

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

## Remaining work

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
