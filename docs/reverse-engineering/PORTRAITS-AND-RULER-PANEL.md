# Portraits and the setup ruler panel

## Reference case

Use 7 August 936, select Otto in Custom Game Setup, and record enabled
DLC, UI scale, and window size. The supplied vanilla panel shows King
Otto of Germany and 21 vassal rows. OpenCK2's history model produces
16 title rows, including multiple titles held by one person. The
first row names Heinrich by Altdorf. The reference displays the same
holder as Count Heinrich of Zurichgau. The source records Zurichgau
under Swabia and Altdorf under Germany in the current history model.
A matched roster requires tracing vanilla's direct-vassal initialization,
display title selection, count, and ordering.

## Confirmed inputs and fixes

- `history/characters/german.txt` defines character 1282 with
  `dna = rcrfdblvsvv` and `prp = aj000a00000000000000000000000000000000`.
  `common/bookmarks/00_bookmarks.txt` uses `properties` inside Otto's
  authored bookmark portrait. OpenCK2 now reads historical `prp`
  while keeping bookmark properties separate. Historical age is 23
  and the authored bookmark age is 24. Both select the same adult
  age variant at this date.
- The previous loader found profiles for all 832 distinct held county
  rulers in the 936 snapshot. Only six had authored DNA. The new
  renderer uses a stable character-ID-based DNA fallback when DNA
  is absent. It uses the installed portrait assets, including female
  and child variants where available, and reads age thresholds from
  `common/defines.lua`. This fallback does not reproduce vanilla's
  DNA initialization or scripted properties.
- `interface/frontend.gui` supplies the large portrait and its
  separate frame, title shield and crown, character-tab position,
  and the vassal row's portrait, frame, shield, and label positions.
  `interface/generalstuff.gfx` resolves the sprite references.
  The four ruler detail lines use the installed Arial12 bitmap font.
  The vassal list now draws visible faces and title shields and uses
  scrollbar sprites resolved from its `EU3_core.gui` scrollbar type.
  Wheel, arrows, and clicking the track scroll the visible rows.
  Portraits and shields are generated when selection or scroll changes.
- A controlled OpenCK2 render of Otto produced different pixels
  with base assets, with Monks and Mystics Content Pack enabled,
  and with that pack enabled plus the historical `prp` applied.
  This isolates two real input differences. It does not establish
  identical pixels with the vanilla compositor.

## Native evidence and remaining work

The local Windows PE has the literal `interface/portrait_properties`
at VA `0x1410371b8`. Disassembly references it at `0x1404ddd40`
in a sequence passing directory strings to the same routine.
The REA/Ghidra trace and a targeted decompile both reached their
time bounds. The literal and its reference provide a starting point
for tracing the property loader. They do not establish how the
engine seeds DNA or updates portrait properties.

The installed `interface/portrait_properties/00_portrait_properties.txt`
describes weighted selection, forced factors, two passes, and
layer hiding. OpenCK2 still needs the native trigger contexts,
evaluation order, seed behavior, and shader composition. The current
title shields use installed flags and the existing mask. Generated
dynasty arms, full ruler titles, vassal roster and order, scrollbar
dragging, difficulty score, diplomacy state, and DLC recommendations
remain pending. The County Holdings panel is still an OpenCK2 extension.

## Verification

`make check-portraits` runs the installed-data regression. Set
`CK2_DIR` if the game is outside the default Steam path. It checks
Otto's historical fields, changes in rendered pixels with and
without his properties, repeatable rendering for Heinrich, and
successful rendering for the four reference vassals, Hedwig, and
the five-year-old Liutgard. The live OpenCK2 check selected Otto
at 936, verified the portrait, crown, title shield, and visible
vassal faces, then exercised wheel and scrollbar-arrow scrolling.
The vanilla screenshot is the visual reference. A matched native
capture under controlled DLC and UI settings remains pending.

## Paused parity work

The user confirmed that the portraits still differ from vanilla and paused
exact portrait matching on 2026-11-11. The current renderer remains the
baseline. Resume from the 936 Otto comparison with the same DLC and UI
scale. Trace native default DNA, property initialization and refresh,
and layer hiding before changing the fallback. Compare the bookmark and
selected portraits separately, then verify the four reference vassals,
an adult woman, and a child. Exact vassal count and order remain a separate
history and UI investigation.
