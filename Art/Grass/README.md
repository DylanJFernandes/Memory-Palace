# Grass blade cards — Blender source

Generated from the stylized-grass pipeline spec (section 1, *DCC & Blender
Modeling Pipeline*). Everything here is source art; the imported Unreal asset
lives at `/Game/Environment/Grass/SM_GrassClump`.

## Files

| File | What it is |
|---|---|
| `SM_GrassClump_LOD0.fbx` | 12 cards, 6 segments each — 168 verts / 144 tris |
| `SM_GrassClump_LOD1.fbx` | 8 cards, 3 segments each — 64 verts / 48 tris |
| `SM_GrassClump_LOD2.fbx` | 4 cards, 1 segment each — 16 verts / 8 tris |
| `GrassCards_Source.blend` | Full Blender scene: card LODs, blade bake atlas, ortho bake camera |

Clump bounds in Unreal: roughly 50 × 48 × 34 cm.

## UV channel contract

These are the values the **Unreal material** reads. They are not what the
Blender UV editor shows — see *Coordinate compensation* below.

| Channel | U | V |
|---|---|---|
| `UV0` | standard 0–1 texture mapping | standard 0–1, tip at V=0 |
| `UV1` | per-card pseudo-random ID, 0–1 | normalized root-to-tip height, 0 = root, 1 = tip |
| `UV2` | card pivot X, mesh local space, **centimetres** | card pivot Y, mesh local space, **centimetres** |

**Card IDs are stable across LODs.** The LOD1 and LOD2 card sets are subsets of
the LOD0 set, and a surviving card keeps the ID it had in LOD0. This is what
stops wind phase from popping on an LOD transition, so if you regenerate the
mesh, preserve that property.

## Coordinate compensation (read before editing the generator)

Two transforms happen on FBX import and both are pre-compensated in the
generator, so the table above is true in Unreal:

1. **Unreal flips V on every UV channel.** The generator stores `1 - value` in
   the V of `UV1` and `UV2` so the flip lands on the intended number.
2. **Unreal negates Y** converting Blender's right-handed space to its
   left-handed space. The baked pivot Y in `UV2.V` is negated to match.

Pivots are baked in **centimetres**, not metres, so they subtract directly from
`Pre-Skinned Local Position` without a scale node.

If the height gradient ever reads inverted in the material (tips dark instead of
roots), the import flip assumption is wrong for your settings — fix it with a
single `OneMinus` on that component rather than re-exporting.

## Required manual steps in Unreal

The MCP import tool does not expose these, so they have to be set by hand.

1. **Manual LODs.** The asset currently has LOD0 only. Open the Static Mesh
   Editor → *LOD Settings* → set *Number of LODs* to 3, then under each LOD's
   *LOD Import* pick `SM_GrassClump_LOD1.fbx` and `SM_GrassClump_LOD2.fbx`.
   Do **not** use *Generate LODs* — automatic reduction wrecks quad card
   topology, which is the whole reason these are hand-authored.
2. **Use Full Precision UVs** — Build Settings. Half-precision UVs quantize the
   baked pivot centimetres badly. This matters.
3. **Generate Lightmap UVs = off** — Build Settings. Verify it did not write a
   generated lightmap channel over `UV1`; if `UV1` looks like an unwrapped
   layout rather than a gradient, that is what happened. Disable and reimport.
4. **Cast Shadow = off** on the mesh / foliage type.
5. **Material slots.** Each LOD ships its own slot name — `MI_GrassCard_Near`,
   `_Mid`, `_Far` — so you can bind progressively cheaper shaders per LOD.

Already applied via MCP: collision removed, Nanite disabled.

## Bake source

`HP_GrassBlades_Atlas` is 48 high-poly blades laid flat in three 1×1 cells,
with `CAM_GrassBake` framing the strip orthographically from above. The three
cells are the atlas the distant billboard tier indexes with PerInstanceRandom at
0.0 / 0.333 / 0.666.

Each blade carries a curved cross-section so a normal bake has shape, plus three
corner colour layers so the spec's masks are a one-channel emission bake rather
than hand-authored material work:

- `RootTipGradient` — root-to-tip 0–1
- `EdgeToCentre` — centre-line mask, drives the specular highlight
- `BladeID` — per-blade random constant

## Regenerating

The generator is parameterized at the top of the script: `LOD_SPEC` controls
card counts and segment counts per LOD, `build_card_defs()` the clump layout,
`ATLAS_CELLS` / `BLADES_PER_CELL` the bake atlas density.
