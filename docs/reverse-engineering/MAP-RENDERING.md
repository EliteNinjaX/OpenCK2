# CK2 terrain renderer investigation

## Reference build

REA 6.8.0 with Ghidra 12.1.4 decompiled the local Windows Steam executable
at VA `0x1408e2100` (RVA `0x8e2100`). The image SHA-256 is
`a0cc8e92287ac900b0552f5cca20df5acedf4773244923a77b15bcdbe143b13d`.
The REA evidence ID is
`ev_59e7a0f32012a13320a726c79ea2056b690d7adcb4c26feae863ef2e780edbdc`.

The function references the topology bitmap, world normals, terrain atlas,
land and water color maps, water normals, tree tint, river textures, strait
texture, and reflection texture. The original installed data explains their
roles:

| Installed file | Finding |
| --- | --- |
| `common/defines.lua` | The frontend camera ends at (790, 500, 600) and looks at (790, 0, 1000). |
| `gfx/FX/pdxmap.lua` | The terrain shader samples a four-by-four repeating tile atlas, applies the terrain color overlay, and combines terrain and height normals for lighting. |
| `gfx/FX/pdxwater.lua` | The water pass uses moving normal/noise samples, water tint, shoreline height, reflection, and refraction. |
| `gfx/FX/tree.lua`, `map/default.map` | Trees render in their own pass and use tint and seasonal data. The map points to a separate tree definition bitmap. |
| `gfx/FX/river.lua` | The river pass shades a textured channel. |

## OpenCK2 implementation

`src/game/map_view.c` projects the CK2 heightfield with a perspective camera.
The menu uses the original final camera coordinates and a tuned field of
view. The campaign camera is wider and farther north so York remains easy
to select. The terrain pass reads the original BC3 atlas, indexed terrain,
color map, and world normals. Water is animated using two moving samples of CK2's noise texture and
its water-normal detail. The CPU pass reads the installed underwater
terrain and reflection cubemap, uses the shader's 93.7 shoreline threshold,
and computes a view-dependent Fresnel reflection. The river bitmap supplies the river mask. Forest
crowns are scattered in the original tree cells with the installed pine
diffuse texture and tree tint.

The map records the source pixel for each projected screen pixel. Province
hit testing and the selection outline use those same samples, so water
animation and perspective do not shift province selection.

The original GPU renderer, camera motion, individual tree meshes, screen-space water
refraction, full river geometry, straits, and seasonal
effects remain outside this implementation. The current comparison is
visual, with no pixel-identical claim.


Realms mode draws borders using both the historical realm assignment and
the underlying province map. Labels use Mapfont and derive their size,
angle, and position from visible territory pixels. Collision placement
keeps prominent labels visible where space permits. CK2's curved-label
layout and dynamic changes from conquest remain pending.

## Iceland and the northwest edge

**Installed data and OpenCK2 reproduction.** The local 3072 × 2048
`provinces.bmp` contains all four Iceland counties. Join the RGB values in
`map/definition.csv` for IDs 1, 2, 1616, and 1617 to the corresponding
pixels in `provinces.bmp`, then compare the same coordinates in
`topology.bmp` and `rivers.bmp`. Count a point as above the waterline
when its topology value is at least 94. This gives the following counts
on the local Steam install:

| County ID | Above-water pixels | Of those, river index 254 |
| --- | ---: | ---: |
| 1 (Vestisland) | 4,519 | 4,519 |
| 2 (Austisland) | 5,708 | 5,708 |
| 1616 (Nordland) | 6,309 | 6,309 |
| 1617 (Sudurland) | 5,513 | 2,701 |

OpenCK2 interpreted river index 254 as ocean, discarded the terrain
elevation there, and shaded those coordinates as water. The Realms pass
still read the county IDs from the province bitmap. The resulting
combination explains the flat, tinted land and the visible change in
shading across southern Iceland. The terrain pass now uses the topology
waterline for elevation and surface classification, with river indices
used for channel coloring. This is a CPU approximation of the
terrain/water split in `pdxmap.lua` and `pdxwater.lua`.

The angled cutoff beside Iceland is the projected northwest edge of the
map. OpenCK2 previously filled everything beyond it with a flat blue-gray
color. `interface/mapitems.gfx` declares the installed `frame3072.xac`
actor. The map now resolves that actor by name and draws its original
528 triangles and 1024 × 256 BC3 diffuse texture with the map camera.
Perspective-correct UVs preserve the wood pattern on the bevel and inner
wall. The depth test keeps the frame in front of the water where they
overlap and removes those pixels from province selection. The former
flat border serves as a fallback when the local model or its texture
cannot be loaded.

The current frame transform is inferred from the model coordinates and
the 3072 × 2048 map. It uses approximately 100 map units per model unit
and places the base of the inner wall at water level. Shading uses the
mesh normals and diffuse texture. The original normal and specular maps
and the precise native transform are still pending. A bounded REA/Ghidra
search for `frame3072` timed out in the earlier investigation. The next
native probe is to trace the actor's render submission and material setup.

At the 769 bookmark all four county histories name the same holder.
OpenCK2 currently labels equal-rank county titles separately. The vanilla
reference shows a prominent Vestisland label at close zoom. The native
primary-title and label selection logic still needs investigation.

## Setup toolbar and image quality

**Installed interface definitions.** `interface/frontend.gui` supplies
the position and order of the eight setup map buttons.
`interface/alternate_start.gfx` resolves the large Shattered World and
Random World buttons and ornaments to the installed DDS art. Terrain and
Realms are interactive. The other six modes and the two world buttons
show the shared red “Not developed yet” tooltip on hover.

**Renderer comparison.** The artwork is read from the local CK2
installation. The visual differences come from how it is sampled and
composited:

- The main menu, setup and campaign maps are rendered into a fixed
  1600 × 900 CPU raster. SDL scales it to the window, so a 1920 × 1080
  or larger output still starts from the same raster. The interface
  also uses fixed logical coordinates and bitmap fonts.
- The former water pass evaluated all animated noise at whole map
  pixels. Zoom magnified each sample into a visible block. The current
  pass retains the fractional world hit, and bilinearly samples
  repeating water noise, water normals, underwater detail, and terrain
  atlas tiles. Province IDs and terrain type IDs still use integer
  lookup.
- Vanilla's installed `pdxmap.lua` and `pdxwater.lua` specify linear
  sampling and mip filtering for many layers. OpenCK2 still uses a
  CPU heightfield, fixed-resolution reflection faces, simple river
  coloring, pixel-difference province outlines, and approximate
  lighting. Zoomed-out detail can still alias, and smooth continuous
  borders and refraction remain missing.

For a repeatable visual check, open the 769 bookmark through
Single Player → New Game → Custom Game Setup and pan northwest in
Realms mode. Check the four Iceland counties, coast, and wooden corner,
then hover over a world button and a deferred map mode. For a rendering
comparison, capture Iceland and Northumbria at matching camera coverage
and output size. Compare the shoreline, province borders, water detail,
and UI independently before changing mip selection or render resolution.

The local OpenCK2 run showed all four Iceland counties with relief, the
wooden northwest corner, and the red tooltips over Shattered World and
a deferred map mode. The comparison uses the supplied vanilla
screenshots and the local CK2 files.

## Interface model loading

`src/game/models.c` looks up model names inside `objectTypes` in
top-level `interface/*.gfx` files. It reads `EMFXActorType` and
`pdxmesh` declarations through the existing asset resolver so mounted
content participates in path lookup. The loader imports positions,
normals, primary UVs, and triangle indices from little-endian XAC
and Clausewitz binary mesh files. It also reads the declared scale and
the first diffuse texture reference. The format reference for the
Clausewitz binary properties is the
[IO PDX Mesh reader](https://github.com/ross-g/io_pdx_mesh/blob/master/pdx_data.py).

A local sweep extracted unique model names from the shipped `.gfx`
declarations and loaded each by name against the base Steam installation.
All 314 XAC actor names and all 56 PDX mesh names in the sweep returned
static geometry. Separate runtime checks panned to Iceland and zoomed
in to verify the frame's corner and visible inner wall. The sweep covers
base model geometry. It does not exercise installed DLC or mod variants,
texture compatibility for every model, or the appearance of other actors
in game.

Next steps for interface compatibility:

1. Preserve material bindings per mesh part, then support normal and
   specular textures, shader selection, and level-of-detail switching.
2. Recover skeletons, skin weights, XSM and Clausewitz animations, actor
   attachments, and scene placement. Verify one unit, settlement, and
   shield against vanilla before broadening rendering support.
3. Extend interface coverage beyond models. Audit the existing sprite,
   portrait, font, and GUI loaders against shipped declarations and
   implement missing widget layout, interaction, and engine bindings.

## Current map checkpoint

The main menu and setup/campaign views consume local CK2 terrain,
topology, normal, river, tree, water and reflection assets. Setup
uses the same projected samples for province picking, selection
outlines and the Realms overlay. Historical titles and holders drive
realm colors and labels. Iceland's height-based shoreline fix and
the original 3D wooden frame are in place. The loader decoded
static geometry for 370 base model names. The frame is the model
verified in the running map.

The history resolver now chooses one province history file for both
Realms and holdings. The [Călărași audit](PROVINCE-HOLDINGS.md) records
the Coconi fix and the scan of 1,740 provinces at all 11 bookmarks.
The [portrait and ruler panel notes](PORTRAITS-AND-RULER-PANEL.md)
record the authored portrait fix, generated fallback, and paused
vanilla portrait parity work.

Remaining map parity includes native GPU lighting and filtering,
water refraction, full river and strait geometry, the exact frame
transform and materials, curved realm labels, and live realm updates.
The software map still renders to a fixed logical raster. Match the
reference camera, zoom, mode and resolution before evaluating further
changes.
