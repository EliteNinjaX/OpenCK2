# OpenCK2 contributor instructions

OpenCK2 reconstructs the Crusader Kings II engine in C17, with the fidelity
expected of a native Windows, Linux, and macOS port of the original engine.
This is a target for reconstructed behavior and architecture. The project uses
the installed game as a reference.

Read [ROADMAP.md](ROADMAP.md) for the current baseline and priorities, then
check the [reverse-engineering knowledge base](docs/reverse-engineering/README.md)
before starting a new investigation. The same workflow applies to local AI
agents, Codex, Claude Code, and human contributors.

- Inspect the worktree, relevant code, and prior evidence. Preserve existing
  changes and working launcher, map, UI, setup, and campaign paths.
- For substantial engine work, start with a narrow reference scenario. Read
  the installed definitions and use REA with Ghidra for undocumented engine
  behavior. Trace callers, data flow, state changes, and ordering as relevant.
  Record the evidence and compare against original CK2 before calling the
  behavior verified.
- Separate direct observations, binary-supported findings, inferences, and
  hypotheses. If CK2, REA, or Ghidra is unavailable, use the checked-in notes
  and available data, state which verification is missing, and leave a
  reproducible next probe. A string match identifies an investigation lead.
- Preserve original behavior, including edge cases and ordering. Track
  temporary approximations with their reason, a replacement condition, and
  a roadmap item. Keep internal differences when they serve a documented
  portability or compatibility need. Base architecture changes on evidence.
- Keep simulation logic platform-independent and isolate SDL, filesystem,
  Steam integration, timing, and other platform services at their boundaries.
  Linux is the current build baseline. Windows and macOS are target
  platforms awaiting verified builds and automated checks.
- Keep original executables, commercial assets, personal saves, and raw
  analysis dumps local. Commit notes, synthetic fixtures, comparison tools,
  and C implementations as appropriate.
- Run the relevant checks in the roadmap and exercise the normal entry
  point. Record what passed, the scope of the reference comparison, and
  what remains uncertain. A successful prototype test establishes only
  the behavior it exercised.

Contributors new to C can help with reference captures, reproduction steps,
historical comparisons, data inventories, and documentation. Contributors
working in C should use the same evidence and verification format. See
[contributor starting points](docs/reverse-engineering/README.md#where-to-start).
