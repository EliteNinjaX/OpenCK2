# Manual runtime acceptance checklist (once SDL + CK2 assets are available)

- [ ] `make game` builds OpenCK2 on Linux; repeat a clean compile with warnings enabled.
- [ ] Confirm eight navigation icons appear in the original order, including Society, and do not overlap the wealth/date display. Compare against same-resolution reference.
- [ ] Identify the exact widget/source corresponding to the old blank tab; verify no invisible click target remains after any future fix.
- [ ] Confirm all eighteen campaign mode sprites appear at sensible positions with tooltips. Unsupported modes stay in the current mode rather than showing fake colors.
- [ ] Click Terrain and Realms icons and press `q`/`w`; verify renderer and selected icon agree. Pan and zoom while in both modes.
- [ ] Hover a landed province with county ruler different from independent ruler; verify province name/ID, county, owner and top liege match title history.
- [ ] Stationary right-click opens county holder, not top liege. Shift-right-click opens top liege. Right drag pans without opening a character.
- [ ] Right-click on character panel, HUD buttons, map-mode icons, buildings and wonder modal must not open a background province owner.
- [ ] Right-click on water or a province with missing holder must not crash; no fabricated owner tooltip data.
- [ ] Change inspected character: show direct vassals from more than one held title, no duplicate person IDs, stable order, correct list counts/scrolling.
- [ ] Compare historical ownership against 769, 867, 1066 and 1337 bookmark data. Do not represent campaign date advancement as historical ownership updates without implementation.
- [ ] Run `make check-map-core` and remaining existing repo tests when their missing source files are restored.
