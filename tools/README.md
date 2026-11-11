# OpenCK2 developer tools

Tools are standalone research and verification programs. Build the game and
launchers through the root [Makefile](../Makefile). Keep local reference files,
REA projects, scratch scripts, and generated comparisons in ignored storage.

## Title history probe

[`title-history.c`](title-history.c) reports the last dated holder assignment
at or before the requested date in one installed CK2 title-history file.

```sh
make title-history
CK2="${CK2_DIR:-$HOME/.steam/steam/steamapps/common/Crusader Kings II}"
./bin/title-history "$CK2/history/titles/c_york.txt" 1066.9.15
```

The pinned reference reports `holder=5660 effective=1066.1.1`. This probe
reads a single file. For runtime history, DLC and mod overlays, title-name
resets, and hierarchy selection, see [the starting-era notes](../docs/reverse-engineering/STARTING-ERAS.md).
