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
