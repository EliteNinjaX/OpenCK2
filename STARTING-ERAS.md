# Starting eras and gameplay interface

## Starting era screen

The reference installation is the local Windows Steam build of Crusader
Kings II. OpenCK2 reads its assets directly from the configured game directory.

| Installed source | What OpenCK2 uses |
| --- | --- |
| `interface/frontend.gui` and `frontend.gfx` | Reference layout and sprite names for the era screen and ruler entries. |
| `gfx/interface/pick_era_*.dds` | Original panel, cards, biography plate, portrait name banners, and selection highlight. |
| `common/bookmarks/00_bookmarks.txt` | Era dates, picture IDs, ruler IDs, and the bookmark's portrait DNA and properties. |
| `localisation/*.csv` | English era and ruler names, era descriptions, and biographies. |
| `history/characters/*.txt` and `history/titles/*.txt` | Birth records and title holders at the bookmark date for featured ruler details. |
| `interface/portraits/portraits.gfx` and `portrait_sprites.gfx` | Reference for layering base-game portrait sprite sheets. The sheets use BC3/DXT5 DDS. |
| `gfx/flags/*.tga` and shield masks and frames | Title flag artwork with an installed shield shape and frame. |

The portrait renderer indexes the installed portrait and sprite definitions
from the base game and enabled DLC and mods. It tries the ruler's ordered
`graphical_cultures` entries and chooses early or late layers using CK2's
date thresholds. In 769, `old_frankish` prefers the early German definitions
from the enabled German portrait DLC, with Holy Fury's Frankish art and
the Western base-game portrait as fallbacks. The renderer also uses the
installed age variants, frame counts, offsets, hair and eye palettes, and
rank frames. CK2's full dynamic clothing rules and dynasty arms remain
incomplete.

## Game Rules and Realms

The rules UI reads `common/game_rules/00_game_rules.txt`, its localized
labels and descriptions, and the interface art referenced by
`interface/game_rules.gfx`. The local installation has 73 rules. The first
listed option is each rule's default. The UI supports changing options,
scrolling, filtering by category, resetting defaults, and saving and
loading one OpenCK2 preset. DLC requirements are shown as labels. DLC
gating, Ironman, achievements, and the simulation effects of the rules
remain pending.

Realms uses the selected bookmark date to resolve county titles, dated
holders and lieges, and the highest title held by an independent ruler.
The overlay uses the installed landed-title RGB colors and draws borders
on the terrain. Its labels use the installed Mapfont and date-specific
title names, including Wessex and West Francia in 769. The overlay represents
the selected start date. It does not simulate territorial change.

## REA findings

REA 6.8.0 inspected the local executable's PE resource table. Evidence ID
`ev_92653533e219637f0add090bcc669495649599c93eecc5eb6b86d8e33753f78b`
covers the executable's embedded Windows resources. The gameplay interface
art in the table above comes from loose game files.

REA's Ghidra string search found these references in the same executable
under evidence ID
`ev_cd905a52e1fc24b65c60823067f852cce2d068a6336d878b93c73f6288b19950`.

| Address | String |
| --- | --- |
| `0x141036b58`, `0x141036b98` | `history/titles`, `history/characters` |
| `0x141036da8` | `common/bookmarks` |
| `0x1410696f0`, `0x141069850` | `pick_era_screen`, `era_character_entry` |
| `0x1410965f8`, `0x1410a09e8` | `portraitType`, `selectable_character` |

The first Ghidra analysis timed out. The retry used a reduced analyzer
profile and returned the string evidence. The native loader's call order
and DLC resolution remain unverified.

A subsequent REA search returned evidence
`ev_c15c036fea02e12281ecc8b4ace6bab5aef12111f936de2dd394cea94e605fcd`
for `common/game_rules` at `0x141036f50`, `mapmode_realms` at `0x14105d488`,
`interface/game_rules.gui` at `0x141067448`, and `game_rules_window` at
`0x1410674c0`. These are executable string references. The installed data
provides the rule inventory and historical title values.

## In-game interface reference

The local `interface/main.gui` and `main.gfx` define the ruler portrait and
navigation at upper left, the resources and time controls at upper right,
and the minimap and map modes at lower right. `outliner.gui` defines the
expandable outliner. `province.gui` defines province selection and the
holdings panel. OpenCK2 now uses the installed sprites for the ruler and
resource bars, date control, outliner, province panel, and minimap frame
and map. The date control pauses and resumes time. The minimap is visual
only, and the ruler navigation tabs show the unfinished-feature tooltip.
The York scenario's controls and goal remain specific to the temporary test.


## Ruler selection and sound

Custom Game Setup shows featured rulers and resolves the independent ruler
when you click a province. Dated character history supplies culture,
religion, and dynasty. A realm takes the ruler's dynasty name when the
culture enables `dynasty_title_names` and the title permits it. This
produces the Abbasid and Umayyad labels at the 769 start. Rulers with
available portrait DNA use installed portrait layers.

REA evidence
`ev_96b8cb2b7f983421371c97c11b268e165cb0142564ef0463c38fd4639d578f73`
identifies `interface/sound.sfx` and `dynasty_title_names` in the Windows
executable. The local sound definitions map the `choose_country_1` through
`choose_country_10` cues to the ruler selection clips. OpenCK2 loads those
clips and the menu, bookmark, and other frontend cues from the installation
and uses the configured effect volume for playback.


## Historical title and holding selection

Setup uses the installed lobby panels, date and bookmark art, and map mode
buttons. Click a county to inspect its historical holder. Alt-click opens
that holder's primary title, and Shift-click opens the independent realm.
The right panel's liege, vassal rows, and occupied holding slots are
selectable. Holding types, slot counts, capital moves, and removals are
read from dated province history. A holding with no recorded historical
holder has no generated ruler in this implementation.

`history/titles/k_france.txt` sets `WEST_FRANCIA` at the early dates and
resets the title name in 987.7.3. The title loader now honors dated
`reset_name = yes` and resolves the default `k_france` localization.
The installed data produces West Francia in 769 and France in 1066.

The selected portrait uses dated character data when available. Karl's
769 character history supplies `aj00bb00000`, while the era bookmark
thumbnail supplies `aj00bc00000`. Both use the installed culture and era
portrait definitions. The original engine's scripted portrait property
selection remains pending.
