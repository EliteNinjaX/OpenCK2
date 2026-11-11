# Province history and the missing Coconi city

## Reference and identification

Reference files are from the local Steam CK2 3.3.5.1 installation. The
provided vanilla map labels the province north of the lower Danube
**Calarasi** and shows it inside Bulgaria. The province bitmap and
`map/definition.csv` give that area ID **1642**, RGB **85,27,21**.
The green patch in OpenCK2 was the terrain showing through an absent
realm assignment. A holding icon was also missing when the province
history could not be associated with the county.

| Original input | Finding |
| --- | --- |
| `history/provinces/1642 - Calarasi.txt` | `c_calarasi` has three slots. `b_calarasi = castle` and `b_coconi = city` are active. Other listed candidates are comments. One slot is intentionally unused in 769. |
| `history/provinces/1642 - Tutrakan.txt` | A second file claims the same province ID and names `c_tutrakan`, sets one slot, and places `b_hvamm` there as a tribal holding. |
| `common/landed_titles/landed_titles.txt`, `history/titles/c_calarasi.txt` | The hierarchy has `c_calarasi` with child titles `b_calarasi` and `b_coconi`. `c_tutrakan` is absent. The active 769 county holder is 145950 under `d_wallachia`. There is no standalone `history/titles/b_coconi.txt`; its absence does not erase the city defined in province history. |
| `common/holding_types/00_holding_types.txt`, `common/buildings/00_buildings.txt` | City is a holding type. City buildings such as `ct_wall_1` are upgrade definitions. The base holding is created by `b_coconi = city`. |
| `localisation/HolyFury.csv` | `PROV1642` resolves to Calarasi, and `b_coconi` resolves to Coconi. |
| `gfx/interface/placeholder_city.tga`, `province_new_settlement.tga` | Separate city and unused-slot art is available in the installation. |

## Root cause and implementation

OpenCK2's Realms and holdings readers independently selected the last
filename with the requested numeric ID. In the base install,
`1642 - Tutrakan.txt` sorts after `1642 - Calarasi.txt`. Realms
discarded the county mapping because the resulting title was undefined.
The holding reader picked the same wrong file. This was a file-selection
bug. City art, localization, and the dated holding parser were usable.

The province map now selects a single history path, which both readers
use. Higher mounted layers retain priority. Within a layer, the
filename matching the active `map/definition.csv` name wins. A lone
file can still use a different historical spelling. In an ambiguous
same-layer tie with neither filename matching the map name, the
first listed candidate remains selected. This is OpenCK2's resolver
policy. The supplied vanilla screenshot confirms the resulting county
identity in this case. The native engine's general collision and mod
precedence rules remain unverified. A separate Proton/WineD3D run reached vanilla
769 Custom Game Setup, but severe map rendering artifacts prevented
a reliable direct Coconi inspection in that session. The vanilla
comparison uses the supplied screenshot for the county and realm
and the installed data for the holding.

In OpenCK2 at 769, clicking Călărași now selects the county within
Bulgaria. Its County Holdings panel shows the castle, the Coconi city,
and the unused third slot. Hovering the city displayed **City: Coconi**.
The renderer also wraps holding slots onto a second row when capacity
exceeds seven, so modded capacities within the supported limit no
longer draw multiple slots at the same coordinates. No audited base
province required that second row.

## Full-map scan

The base-install scan covered all **1,740 mapped provinces with
history** at all **11 dates** in `common/bookmarks/00_bookmarks.txt`
(19,140 province/date checks). Twenty-one province IDs had duplicate
history files. Each had one filename matching the map definition.
The previous alphabetical last-file rule chose a different file for
six IDs:

| ID | Map province |
| --- | --- |
| 446 | Morava |
| 722 | Al 'Aqabah |
| 1642 | Călărași |
| 1652 | Borovichi |
| 1676 | Kromy |
| 1681 | Kursk |

Five of the six old selections pointed to a county title absent from
the shipped hierarchy. Morava used a different history file without
losing its county title. After the fix, every audited province had
bitmap pixels, a defined county, and at least one populated holding.
Every populated holding matched its parent county in the shipped
landed titles and had an exact localization key. An independent
read of the loose histories agreed with the C loader on active holding
IDs and types across the scanned dates, including the trailing dot in
Marmaros's `895.1.1.` date token. At 769, the audit found 958 cities
and 1,570 counties with at least one intentionally unused slot.

This is a data audit and a targeted runtime display check. It did
not visually inspect every tile, validate vanilla's generated holding
owners or building levels, or sweep DLC/mod combinations.

## Reproduce

```sh
make holding-audit check-holdings CFLAGS='-O2 -Werror'
CK2="${CK2_DIR:-$HOME/.steam/steam/steamapps/common/Crusader Kings II}"
./bin/holding-audit "$CK2" 769.1.1
```

The audit emits province ID, map name, selected history file, county
title, slot capacity, and active holding IDs/types. To repeat the
comparison, run it at each `date` in `common/bookmarks/00_bookmarks.txt`.
Join each emitted county and holding against `common/landed_titles/`,
each holding against `localisation/`, and each province ID against its
RGB in `map/definition.csv` and `map/provinces.bmp`. The synthetic
test verifies canonical selection, renamed mod override precedence,
and dated addition and removal of holdings.
