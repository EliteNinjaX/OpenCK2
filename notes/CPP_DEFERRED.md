# Possible C++ candidates — deferred (not prerequisites)

The current implementation remains **C17**. The items below are *candidates* for C++ if their complexity warrants it, not decisions to rewrite working C code. Each has a viable C design as well; do not migrate for syntax alone.

| Later subsystem | Why C++ could be helpful | Reason to defer |
|---|---|---|
| Fully general event/decision and scripted effect evaluator | Strongly typed node/variant tree, ownership-aware containers, multi-pass validation | First verify CK2 script syntax, trigger/effect behavior, and data contracts. The current C tokenizer and loose-file reader remain valuable. |
| Long-lived character/title/war relationship graph | Encapsulation and RAII for complex dynamically linked object lifetime | Implement and compare the historical snapshot/IDs in C before designing a mutable graph. |
| Campaign save games, stable object handles and versioned serialization | Schema evolution, typed serialization and managed ownership | No reliable complete gameplay-state model exists yet; locking in a format would be premature. |
| Multi-agent AI/economy/diplomacy planning | Large composable state/action objects and polymorphic strategy evaluation | Not part of this visual/interaction milestone; needs verified simulation mechanics first. |
| Highly dynamic mod hot reload/dependency graph | C++ containers and resource lifecycle helpers can simplify invalidation | Existing mounted assets and C parser work should first be proven correct for priority and cache invalidation. |

**Stay in C now:** sprite/GUI binding, hover/hit tests, map gesture input, ownership lookup, historical-file parsing, map-mode descriptors, SDL rendering adapters, utility caches and predictable data-array algorithms. These map naturally to small C modules and testable APIs.

If a later C++ component is approved, keep a narrow `extern "C"` C API boundary and avoid a whole-project rewrite. No language change is required for any remaining task in the immediate UI/map plan.
