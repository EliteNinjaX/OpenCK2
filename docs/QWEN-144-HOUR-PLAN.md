# OpenCK2 local AI work plan

This is a 144-hour work budget for Qwen 3.5 9B. The 18 parts have eight
hours each. Finish the acceptance check and checkpoint early whenever
possible. An eight-hour budget does not require an uninterrupted session.

## Start and resume

1. Open `/media/london/EFFC-FDAD/OpenCK2`. Read `AGENTS.md`, the relevant
   row below, and its linked notes. Record the current commit and
   `git status --short` before editing. Preserve changes already present.
2. Work on one part at a time. Divide it into slices of roughly 30 to
   90 minutes. At the end of each slice, record the exact files changed,
   evidence, commands run, results, and next command in a local progress
   log at `/media/london/EFFC-FDAD/OpenCK2-Qwen-progress.md`. Keep screenshots,
   installed CK2 assets, saves, and raw REA output
   outside the repository.
3. On resume, read the last checkpoint and compare it with the current
   worktree. Continue from the first unfinished acceptance check. If a
   reference or an engine dependency is missing, record the missing
   evidence and move to the next independent part. Keep the blocked part
   visible in the log.
4. At the end of each part, run its focused check, `make game`, and
   `git diff --check`. Record existing failures separately from failures
   introduced by the change. Leave the worktree available for review.

Start with `docs/UI-TODO.md` and [construction evidence](reverse-engineering/CONSTRUCTION.md).
Use the [evidence conventions](reverse-engineering/README.md) for CK2
observations, binary traces, and hypotheses.

## Work queue

| Part | Hours | Work | Acceptance check |
| --- | ---: | --- | --- |
| 01 | 8 | Capture the build baseline and repair the construction test fixture. | Existing queue tests still run. Insufficient gold and a second pending settlement leave treasury and queue unchanged. `make check-construction` passes. |
| 02 | 8 | Add focused character-history fixtures for traits, claims, and resource assignments. | The fixture proves repeated keys, dated changes, future-date exclusion, claim removal, and explicit zero versus missing values. |
| 03 | 8 | Verify claims display against Edward III in 1337 and one authored weak claim. | Record CK2 and OpenCK2 shields and dates. Fix clipping, source ordering, or strong/weak presentation where verified. |
| 04 | 8 | Verify portrait navigation and the consort heading. | Open Philippa from Edward, return to Edward, open a family member, and inspect a vassal. Player treasury and time stay unchanged. |
| 05 | 8 | Verify trait ordering and attribution. | A fixture with two bonuses and a penalty shows each named contribution and moves the education icon first. Validate signed values and unknown base handling. |
| 06 | 8 | Compare tooltip geometry with vanilla. | Wealth, Demesne, Traits, Modifiers, Claims, and Martial tooltips stay readable and within the viewport at two resolutions. |
| 07 | 8 | Research generated starting prestige and piety. | Capture king, emperor, and unlanded cases at one date with documented DLC. Locate a loose-file or binary source for any rule proposed for implementation. |
| 08 | 8 | Bind verified resource initialization. | After Part 07, reproduce one authored and one generated case. Add a focused regression check and keep missing values distinguishable. |
| 09 | 8 | Research demesne, vassal-limit, and realm-size counting. | Record the count inputs and one vanilla comparison for each. Identify where OpenCK2 lacks title, holding, law, or government state. |
| 10 | 8 | Inspect vanilla coronations and their trait effects. | Record a scripted priest, bishop, or pope path, DLC requirements, and the resulting trait. Implement only a verified, testable slice. |
| 11 | 8 | Inventory character modifiers and opinion inputs. | Trace one authored modifier with its source and duration. Record the input components of one vanilla opinion tooltip before proposing totals. |
| 12 | 8 | Decode one authored dynasty coat of arms. | Use an existing dynasty, original atlas files, and a vanilla capture. Record the palette, atlas frame, emblem, and shield-mask evidence. |
| 13 | 8 | Render the verified dynasty-arm case. | After Part 12, display the result in the CK2 shield slot. Add a regression fixture and record which atlas or religion variants remain unsupported. |
| 14 | 8 | Verify one historical bloodline family. | Trace one founder and named descendants in vanilla. Record DLC, lineage direction, and any exceptions before coding membership. |
| 15 | 8 | Add one bloodline view path. | After Part 14, use the original panel and symbols for verified memberships. Confirm an eligible and an ineligible character. |
| 16 | 8 | Compare two historical Great Works. | Verify dates, stage, damage, and completed upgrades for Hagia Sophia and another authored wonder. Add a fixture for repeated upgrade keys. |
| 17 | 8 | Verify one settlement construction cost and duration. | Compare Middlesex in vanilla and OpenCK2. Trace a specific modifier input and record the fixed-point calculation before changing the price. |
| 18 | 8 | Verify integration and prepare handoff. | Run the relevant Make checks, smoke-test launcher and campaign, and update the UI TODO with verified results, open gaps, and the next smallest task. |
| **Total** | **144** | | |

## Checkpoint template

```text
Part / slice:
Base commit and pre-existing changes:
Status: in progress / verified / blocked
Files changed:
Evidence and comparison scenario:
Checks and results:
Remaining risks or gaps:
Next exact command or edit:
```

## Paste into Qwen

> Work through docs/QWEN-144-HOUR-PLAN.md one part at a time. Start with the
> first unfinished part. Read AGENTS.md and preserve existing worktree
> changes. Keep a local checkpoint after each small slice so I can stop
> and resume you. Keep original CK2 files and raw captures outside the
> repository. Report verified results separately from assumptions and
> blockers. Do not push changes. Stop after the current checkpoint if I
> ask to pause.
