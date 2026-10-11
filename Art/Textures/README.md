# Texture sets

21 maps, all 1024×1024, 8-bit. Eight sets cover all 45 island-kit assets.
The five tiling sets were authored in Substance Designer over the MCP bridge;
the three cut-outs were rendered from real geometry in Blender.

## The sets

| Set | Covers these material slots | Maps | Seamless |
|---|---|---|---|
| `PalmBark` | `M_PalmBark` | BaseColor, Normal, ORM | both axes |
| `WeatheredWood` | `M_Driftwood`, `M_VineStem`, `M_FlowerStem` | BaseColor, Normal, ORM | both axes |
| `Stone` | `M_RockStone`, `M_RockShore`, `M_Pebble` | BaseColor, Normal, ORM | both axes |
| `Moss` | `M_RockMoss` | BaseColor, Normal, ORM | both axes |
| `OrganicDetail` | `M_Seashell`, `M_Starfish`, `M_Coral`, `M_Coconut` | BaseColor, Normal, ORM | both axes |
| `LeafCutout` | `M_PalmLeaf`, `M_PlantLeaf`, `M_PlantLeafDeep` | BaseColor+alpha, Normal | no — one leaf fills 0–1 |
| `GrassBladeCutout` | `M_PlantGrass`, `M_BeachGrass` | BaseColor+alpha, Normal | no — one blade fills 0–1 |
| `FlowerPetalCutout` | `M_TropicalFlower` | BaseColor+alpha, Normal | no — one petal fills 0–1 |

`M_Cloud` deliberately has no texture. Drive it from the `VertGradient` vertex
colour plus fresnel.

**ORM packing:** R = ambient occlusion, G = roughness, B = metallic (always 0
for these natural materials).

**Cut-out opacity** is in the BaseColor alpha channel, as requested — the PNGs
are RGBA. Cut-outs ship no ORM; drive foliage roughness from a constant.

## Unreal import settings

| Map | Compression | sRGB |
|---|---|---|
| `*_BaseColor` | Default (DXT) | **on** |
| `*_Normal` | Normalmap | off |
| `*_ORM` | Masks | **off** |

Getting sRGB wrong on ORM is the usual cause of roughness reading too bright.

## Tiling rates baked into the meshes

These are fixed by the exported UVs; the textures were built to suit them.

- **Palm trunks** — cylindrical, wrapping exactly once around, tiling 2–4.5×
  vertically by tree height. `PalmBark` is seamless left-to-right as well as
  top-to-bottom, so there is no vertical seam up the trunk.
- **Rocks** — box projection, 1 tile per 2 m.
- **Beach props** — box projection, 1 tile per 0.5 m, four times tighter.

## Use the vertex colours for variation

`FlowerTint` and `CoralTint` already carry per-head and per-stem hue. Multiply
the petal / coral texture by the vertex colour in the material rather than
authoring a texture per colour — one map serves every variant.

## Caveats

1. **Palm bark is soft.** Horizontal ring banding is correct for palm (they
   have ring scars, not vertical fissures), but the detail is low-frequency.
   If it reads mushy close up, raise the `fibers_2` weight and the `contrast`
   range in `build2.py`.
2. **`OrganicDetail` is tiling detail only.** Shells, starfish and coral are
   box-projected, so a bespoke painted texture would not align to anything.
   Painted shells need a real seam-cut unwrap first.
3. **Cut-out normals are shallow** — derived from the shape's dome, so they
   give rounding but no surface micro-detail.
4. These are procedural, not hand-painted. They read as coherent stylised
   material, not as authored texture art.

## Regenerating

The three cut-outs are standalone:

```bash
"C:/Program Files/Blender Foundation/Blender 4.5/blender.exe" --background --factory-startup --python Art/_Generators/make_cutouts.py
```

The five tiling sets need **Substance Designer running** with the MCP plugin
loaded (port 9881). Then, from `_Generators/sd/`:

```bash
python run2.py PalmBark Stone WeatheredWood Moss OrganicDetail
```

### Two things that will bite you in Substance

Both are SD 16 incompatibilities in `matthieuhuguet/substance-designer-mcp`
(the plugin targets 15.x), worked around in `sdlib.py`:

- **`SDNode.setPropertyValueFromId` no longer exists.** The plugin calls it
  throughout, so none of its own parameter sets land, and its 79 bundled
  recipes silently build broken graphs. Use `getPropertyFromId(id, category)`
  then `setPropertyValue(prop, value)`.
- **An unknown port ID crashes the whole Python call** with no traceback and
  an empty response, rather than raising. Instance nodes do *not* expose
  `unique_filter_output` — their ports are named after the instanced graph
  (`Fiber`, `Clouds_2`, `R`/`G`/`B`/`A`). `sdlib.connect()` resolves and
  validates both ends before connecting.

Also note the plugin's recipes reference library nodes by hardcoded SD 15
dependency UIDs (`pkg:///perlin_noise?dependency=1563156574`) which do not
resolve in 16. `sdlib.lib_resource()` loads the `.sbs` by absolute path from
`resources/packages/` instead, which is version-independent.

Resolution and bit depth are set in `Graph.set_size()`: `$outputsize` must be
set **absolute** (it defaults to relative-to-parent, where the value is an
exponent offset — writing 10 multiplies resolution by 1024 instead of setting
1024px), and `$format` must be set **per node**, since nodes inherit it from
their input rather than from the graph.
