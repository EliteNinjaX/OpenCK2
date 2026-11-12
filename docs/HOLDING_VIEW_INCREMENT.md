# Province panel visibility and holding view increment

## Implemented (C17)

- Province panel is only rendered when a province is selected. The map, ruler HUD, minimap, and map-mode controls remain visible when it is closed.
- The close control is read from `interface/province.gui` (`province/button_close`), using `GFX_main_close_button` from `interface/generalstuff.gfx` where available.
- Escape closes the province panel after higher-priority modal views have been dismissed. Clicking an unowned sea tile also clears the selection.
- Clearing the selected province resets the holdings snapshot and highlighted province, preventing stale data from leaking into the next selection.
- The holding renderer now tolerates missing culture-specific settlement artwork without dereferencing a null texture. It preserves the original GUI frame and its original `capital_entry`/`settlement_entry` offsets.

## Still needed for reference-image fidelity

- Original holding owner/controller/realm heraldic shields for each barony; these are separate widgets in `province.gui` and require barony-level title/holder data.
- Correct original province-panel text columns and icon data (levies, garrison, income, supply, revolts) instead of the current development text overlay.
- Religion-specific temple artwork and precise per-holding culture selection (the current renderer uses province graphical culture).
- The original GUI panel extensions, settlement progress bars and building level display.
- Full graphical comparison at the same resolution, selection and bookmark; no SDL build is available in this environment.

## Linux validation

```sh
make -B game CFLAGS='-O2 -g -Werror'
make check-map-core check-economy-construction
```

1. Select a province; confirm its holdings panel opens.
2. Click its close button; the map should remain visible without the province panel.
3. Select another province; the new holdings and culture/religion must load.
4. Press Escape once with the province panel open; it should close without leaving the campaign.
5. Click the sea and confirm that the panel disappears.
6. Verify that the empty slots retain their original frames and clicking an occupied slot still opens construction.
