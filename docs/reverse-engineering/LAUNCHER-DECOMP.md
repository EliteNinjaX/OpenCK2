# CK2 launcher decompilation

## Milestones

1. **Startup decision — complete.** Locate the launcher entry path and reconstruct the bypass-token comparisons and branch predicate in C. Check all branch inputs and confirm one bypass flag on the original build.
2. **Launcher lifecycle — complete for the traced handoff.** Recover the setup and event-loop calls. Identify what Quit, Play and Continue return to startup. Verify the result codes and the Continue save-path handoff against the original.
3. **DLC and Mods — complete for installed, selectable content.** Recover where the two lists come from, how selections are persisted, and how Play applies them. Use local fixtures and compare the resulting launch state.
4. **Parity checks.** Byte matching is on hold. Compare the independent launcher with the original screen and scenarios. For machine-code matching, choose a small original function with clear boundaries and test a matching Windows toolchain before expanding to larger units.

## Milestone 1 evidence

The reference is Windows Steam build 7399341, CK2 3.3.5.1, x86-64 PE.
The executable SHA-256 is `a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d`.
REA 6.8.0 with Ghidra 12.1.4 completed its image inventory on this executable.
REA reports a preferred image base of `0x140000000`. Addresses below are RVAs relative to that base. The branch trace was checked against local GNU objdump disassembly.

| RVA | Evidence |
| --- | --- |
| `0x80e4f0–0x8111d8` | PE unwind table entry enclosing the startup code. Its upper bound is exclusive. |
| `0x80f6e9`, `0x80f708`, `0x80f727` | References to `startgame`, `skiplauncher` and `nolauncher`. Each calls the same case-sensitive byte-string comparison at `0xe8ca40`. A zero result reaches the write of 1 to `[rbp+0x540]` at `0x80f737`. |
| `0x810441` | Calls `0x80d950`. Its return byte is still in `al` when the branch checks it. |
| `0x81044c–0x810466` | Skips launcher setup at `0x810785` only when `[rbp+0x540]` is nonzero, the global byte at RVA `0x1666cf9` is zero, and `al` is zero. |
| `0x81052f`, `0x8105d1` | Loads the `launcher/interface` string and passes it into the setup call at RVA `0xd76310`. |

The recovered C is in [startup.c](../../src/launcher/startup.c). The token function accepts the internal token, so the tested external argument `-skiplauncher` and the compared internal string `skiplauncher` are different inputs. The two override names describe their effect at the branch. Their producers and the rest of command-line parsing remain outside this milestone. Both C functions model blocks within a larger original function, so there is no standalone original function to byte-match against them.

From the repository root:

```sh
make launcher check-launcher-startup CFLAGS='-O2 -Werror'
```

The test covers all eight input combinations and exact token matching. In a live Steam/Proton run, `steam -applaunch 203770 -skiplauncher` passed the flag to the pinned executable and reached the game’s main menu. An earlier launch without that flag showed the pre-game screen. The other two tokens were verified by disassembly. The C logic is a separate module from the Steam reference runner, which still opens the original.

## Milestone 2 evidence

| RVA | Evidence |
| --- | --- |
| `0xd76310`, `0xd76850`, `0xd76db0` | Startup calls launcher setup with the interface path, supplies the saved selection lists, and passes `launcher/motd.txt` before entering the loop. |
| `0xd7877e`, `0xd787b7`, `0xd787f0` | The launcher object installs callbacks at `0xd7e1a0`, `0xd7e310`, and `0xd7e490`. |
| `0xd7965b`, `0xd796b7`, `0xd79713` | Binds those callbacks to `play_button`, `continue_button`, and `cancel_button`, respectively. |
| `0xd7e1be`, `0xd7e32e`, `0xd7e490` | Each callback sets the loop exit flag at object offset `0x88` and stores its result at offset `0x30`. Play stores 0, Continue stores 1, and Quit stores 2. |
| `0xd76f90` | REA/Ghidra decompilation confirms the loop processes events, updates the launcher, and calls `SDL_Delay(1)` while waiting for its approximately 60 Hz update interval. It returns the result field when the loop exits. |
| `0x810725–0x810785` | After selection writeback, result 2 exits via cleanup. Result 1 calls the Continue helper at `0x80da80` and copies its returned filename into a global consumed during game startup. Result 0 falls through to normal startup. |
| `0x9ddba0`, `0x9de00c`, `0x9de0b2` | The Continue helper delegates to a save search that enumerates `.ck2` files, reads modification times via `PHYSFS_getLastModTime`, and replaces its current eligible candidate only when the new timestamp is greater. At `0x711cc8`, startup passes the filename and associated metadata to the game’s load path. |

In an isolated Proton prefix, two copied saves were renamed and given different
modification times. Reversing their timestamps changed the Continue label from
`CodexNewer` to `CodexOlder`, with the filenames and save contents unchanged.
Clicking Continue entered game initialization. Both candidates were opened
during the run, so file-open events alone do not establish which save loaded.
The selected-filename handoff is established by the branch and call trace.
The isolated run did not verify arrival in a loaded campaign.

[lifecycle.c](../../src/launcher/lifecycle.c) models the action dispatch and timestamp
selection after the game's eligibility checks.

## Milestone 3 evidence

The original enumerates `.dlc` and `.mod` descriptors through the same listing
helper at RVA `0xd64fb0`, with call sites at `0xc75175` and `0xc75612`.
Holy Fury’s installed `dlc/dlc074.dlc` points to `dlc/dlc074.zip`.
A minimal `mod/codex_probe.mod` placed in the isolated profile's mod directory
appeared in the Mods tab. Steam Workshop and duplicate-descriptor precedence
were outside this test.

| RVA | Evidence |
| --- | --- |
| `0x8d0ea`, `0x8d102` | Registers the settings keys `last_dlcs` and `last_mods`. |
| `0x81060d–0x81061a` | Passes the settings vectors at offsets `0x2a8` and `0x2c0` into launcher setup. |
| `0x8106b3–0x810720` | Copies launcher selections back into the settings vectors and invokes settings serialization before checking the launcher result. |
| `0x9e9af5`, `0x9e9d10` | Serializes nonempty lists. The mod writer constructs quoted strings. |

The isolated profile produced the following after disabling Holy Fury,
enabling the probe mod, and clicking Quit:

```text
last_dlcs=
{
    dlc/dlc074.dlc
}
last_mods=
{
    "mod/codex_probe.mod"
}
```

`last_dlcs` records **disabled DLC descriptors**. `last_mods` records **enabled
mod descriptors**. On reopen, Holy Fury stayed unchecked and the probe stayed
checked. File-open monitoring started at the launcher immediately before each
button click:

| Action and selections | Holy Fury archive | Jade Dragon archive | Probe mod script |
| --- | --- | --- | --- |
| Continue, Holy Fury off and probe on | No open observed | Opened | Opened |
| Play, Holy Fury on and probe off | Opened | Opened | No open observed |

After the Play test, both settings blocks were absent, consistent with the
serializer skipping empty lists. [content.c](../../src/launcher/content.c) models
selection restoration for already discovered, selectable descriptors. Ownership
checks, Workshop discovery, the general settings parser, and the full mount
ordering remain outside this C model.

Run the combined tests with:

```sh
make launcher check-launcher CFLAGS='-O2 -Werror'
```
