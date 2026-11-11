# OpenCK2 reverse-engineering knowledge base

This index separates what has been learned about CK2 from what OpenCK2
currently implements. Start with the [roadmap](../../ROADMAP.md) for task
order and use the [finding template](FINDING-TEMPLATE.md) for new evidence.
Existing detailed notes remain linked at their original locations.

## Where to start

| Contributor | First useful step |
| --- | --- |
| Human new to C | Follow a roadmap scenario in original CK2 and OpenCK2. Record the date, ruler, settings, click sequence, and visible difference. Captures, historical checks, and reproduction notes are useful contributions. |
| Human who works in C | Read the relevant source and evidence, choose a bounded behavior, and add the implementation and a focused comparison or regression check. |
| Local AI agent | Give it this index, [AGENTS.md](../../AGENTS.md), one roadmap task ID, and the relevant note or source files. If it lacks access to the local game or REA, have it mark the missing checks and leave commands for a contributor who can run them. |
| Codex | Start with [AGENTS.md](../../AGENTS.md), then use the workspace and local tools to reproduce the task's reference scenario and verify the resulting change. |
| Claude Code | Start with [CLAUDE.md](../../CLAUDE.md), which points to the same shared instructions. Give it one roadmap task and its evidence links. |

## Reference and evidence conventions

The pinned Windows Steam reference is CK2 3.3.5.1 (SOHY), app 203770,
build 7399341. The executable is `CK2game.exe`, x86-64 PE, with preferred
image base `0x140000000` and SHA-256
`a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d`.
Check the hash and image base before reusing an address. Earlier work
used REA 6.8.0 and Ghidra 12.1.4. Use RVAs or clearly marked VAs, with
the source build and analysis profile alongside them.

Use one or more labels on each finding, according to its evidence:

- **Observed** means reproduced in the original game or directly read from
  a named installed definition. Say which of those applies.
- **Binary-supported** means backed by disassembly, decompilation, or a
  traced data reference. State exactly what the evidence establishes.
- **Inference** means the available evidence supports an interpretation.
  Explain the reasoning and how to distinguish alternatives.
- **Hypothesis** means an unverified explanation or proposed probe.

Record OpenCK2 verification separately. Use the same distinction when
describing recovered structures, field offsets, or relationships.
Decompiled pseudocode is analysis output. Use it to explain the evidence
and implement verified behavior in the C17 project.

## How REA was used

Earlier work imported the local Windows executable through the Ghidra
provider, used string searches to locate startup and frontend anchors, and
traced callers, comparisons, result fields, and the Continue save-path
handoff. The startup branch was checked against disassembly and a local
Steam/Proton run. A separate decompilation identified terrain inputs;
installed shader files explained how the original renderer uses them.
Later searches located anchors for eras, game rules, portraits, and sound.
Those string findings guide further investigations into call order and
runtime behavior.

The [roadmap REA workflow](../../ROADMAP.md#rea-workflow) records repeatable
commands and timeout guidance. The notes below retain evidence IDs,
addresses, and the limits of each investigation.

## Subsystem index

| Area | Existing evidence | Next question |
| --- | --- | --- |
| Startup, launcher, and content | [Launcher decompilation](LAUNCHER-DECOMP.md) documents bypass decisions, the launcher loop, selection writeback, and the Continue handoff. | Native mount precedence, dependencies, initialization after the handoff, and save eligibility. |
| Architecture and world initialization | [Architecture map](ARCHITECTURE.md) tracks known subsystem boundaries and open questions. | Trace the 769 bookmark from selection through history application to the first playable state. |
| History, titles, holdings, frontend, portraits, rules, and sound | [Starting eras](STARTING-ERAS.md) records installed definitions, title resets, ruler selection, portrait inputs, and REA string anchors. | Separate history application, world creation, and first-tick effects. Trace scripted portrait properties and rule consumers. |
| Map, rendering, and UI | [Map rendering](MAP-RENDERING.md) records a decompiled terrain function and installed shader inputs. [Starting eras](STARTING-ERAS.md) maps frontend art and UI definitions. | Recover pass order, projection, label layout, and representative UI input handling. |
| Scripting and events | Installed data readers exist in OpenCK2. General runtime execution has no verified trace in the current notes. | Follow one trigger and effect through scope resolution and the event queue. |
| Tick loop, economy, succession, diplomacy, combat, and AI | OpenCK2 has a prototype clock and economy. The original systems' algorithms remain open. | Recover the scheduler and isolate one daily transition before expanding to coupled systems. |
| Saves and determinism | The launcher notes document Continue's filename selection and handoff. World serialization and deserialization remain open. | Trace a plain-text non-Ironman save load and map stable world IDs to their records. |

For a new subsystem, add a focused Markdown file here and link it from this
index and the roadmap. Keep observations, addresses, concise traces, and
repeatable commands in Git. Keep tool projects and raw dumps in local scratch
space. When a later investigation changes a conclusion, link the new evidence
and explain which earlier interpretation it supersedes.
