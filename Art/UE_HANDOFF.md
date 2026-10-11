# Tropical island environment — build brief

All art is imported and organised. Nothing below needs to be modelled or
textured; this is assembly, materials and placement.

---

## 1. What already exists

**46 static meshes** under `/Game/Environment/Island/Meshes/`:

| Folder | Count | Contents |
|---|---|---|
| `Palms/` | 7 | `SM_Palm_01_TallStraight` … `SM_Palm_07_DramaticCurve` — 900/750/850/400/800/250/700 cm |
| `Vegetation/` | 10 | `SM_Veg_01_BushLarge` … `SM_Veg_10_Vine` — bushes, ferns, broadleaf, flower cluster, 3 grasses, vine |
| `Rocks/` | 10 | `SM_Rock_01_BeachSmall` … `SM_Rock_10_Shoreline` — incl. 2 cliff segments and a walk-through arch |
| `Clouds/` | 6 | `SM_Cloud_01_SmallPuffy` … `SM_Cloud_06_SmallWispy` — 8 m to 40 m wide |
| `Beach/` | 12 | coconuts, shells, starfish, driftwood, pebbles, coral, flower, fallen frond |
| `Grass/` | 1 | `SM_GrassClump` — the LOD'd grass card cluster |

**21 textures** under `/Game/Environment/Island/Textures/`, already set to the
correct compression and sRGB. Do not change those settings.

**Empty folders ready for you:** `Materials/`, `Materials/Instances/`,
`Foliage/`, `Blueprints/`, `Maps/`.

Units: 1 Blender metre = 100 uu. Every mesh sits on z = 0, so it places by its
base. Heights are exact to spec.

---

## 2. Material slots → texture sets

**Every mesh has named material slots with nothing assigned.** Slot names are
stable, so you can bind by name.

| Slot name | Texture set to use |
|---|---|
| `M_PalmBark` | `T_PalmBark_*` |
| `M_Driftwood`, `M_VineStem`, `M_FlowerStem` | `T_WeatheredWood_*` |
| `M_RockStone`, `M_RockShore`, `M_Pebble` | `T_Stone_*` |
| `M_RockMoss` | `T_Moss_*` |
| `M_Seashell`, `M_Starfish`, `M_Coral`, `M_Coconut` | `T_OrganicDetail_*` |
| `M_PalmLeaf`, `M_PlantLeaf`, `M_PlantLeafDeep` | `T_LeafCutout_*` |
| `M_PlantGrass`, `M_BeachGrass` | `T_GrassBladeCutout_*` |
| `M_TropicalFlower` | `T_FlowerPetalCutout_*` |
| `M_Cloud` | **no texture** — see §5 |

Each tiling set has `_BaseColor`, `_Normal`, `_ORM`. Cut-out sets have
`_BaseColor` (opacity in the **alpha channel**) and `_Normal`, no ORM.

**ORM channel order: R = AO, G = Roughness, B = Metallic.** Metallic is 0
everywhere; you can ignore B.

---

## 3. Vertex colour channels — use these, don't recompute them

| Channel | On | Meaning |
|---|---|---|
| `WindMask` | all palms, all vegetation | 0 on the trunk/stem, 1 at leaf tips |
| `VertGradient` | all clouds | 0 at the underside, 1 at the top |
| `FlowerTint` | `SM_Veg_06_FlowerCluster` | per-head red / orange / yellow |
| `CoralTint` | `SM_Beach_10_CoralCluster` | per-stem colour |

`WindMask` is built from **radial distance to the stem**, not height. That is
deliberate: palm fronds droop *below* the crown, so a height-based mask would
give the trunk more wind than the leaves. Multiply your wind World Position
Offset by this channel and the trunk stays planted while the fronds move.

`FlowerTint` and `CoralTint` mean you need **one** petal texture and **one**
coral texture. Multiply the texture by the vertex colour; don't author or buy
per-colour variants.

---

## 4. Materials to build

### M_Island_Surface (master, opaque)
For bark, wood, stone, moss, organic detail.
- Base Color ← `_BaseColor`
- Normal ← `_Normal`
- ORM ← `_ORM`, split: R → Ambient Occlusion, G → Roughness, B → Metallic
- Scalar params for roughness min/max remap and a colour tint multiply
- Make instances per slot name in `Materials/Instances/`

### M_Island_Foliage (master, **Masked**, Two Sided)
For leaves, grass blades, petals.
- Blend Mode **Masked**, Two Sided **on**, Shading Model **Subsurface**
- Base Color ← `_BaseColor` RGB, multiplied by Vertex Color (for the tint channels)
- Opacity Mask ← `_BaseColor` **alpha**
- Normal ← `_Normal`
- Roughness: constant ~0.55 (cut-outs ship no ORM)
- Subsurface colour: a warm green, masked by `WindMask` so roots stay opaque
- World Position Offset: your wind function × `WindMask`

### M_Island_Cloud (master, Translucent or Masked)
- No textures. Lerp a white top colour to a blue-grey underside using
  `VertGradient`, add fresnel for rim brightness.
- These are **mesh** clouds for mid-distance silhouette. Keep UE's volumetric
  clouds and sky atmosphere for the actual sky; these complement, not replace.

---

## 5. Tiling rates are baked into the UVs

Do not rescale UVs — the textures were authored for these rates.

- **Palm trunks** — cylindrical, wrapping exactly once around, tiling 2–4.5×
  vertically by tree height. `T_PalmBark` is seamless on both axes, so there is
  no vertical seam up the trunk.
- **Rocks** — box projection, 1 tile per 2 m.
- **Beach props** — box projection, 1 tile per 0.5 m.
- **Leaves / blades / petals** — one leaf fills UV 0–1. The cut-out textures
  match these outlines exactly, so the alpha lands correctly.

Two beach props — `SM_Beach_11_TropicalFlower` and `SM_Beach_12_FallenFrond` —
use per-petal/per-leaflet 0–1 UVs, not box projection. Use the foliage master
on those two, not the surface master.

---

## 6. The grass cards need specific settings

`SM_GrassClump` is not an ordinary mesh. It carries packed data channels:

| Channel | U | V |
|---|---|---|
| `UV0` | standard texture | standard texture |
| `UV1` | per-card random ID (0–1) | normalised root-to-tip height |
| `UV2` | card pivot X (**centimetres**) | card pivot Y (**centimetres**) |

Required before use:
1. **Use Full Precision UVs = on** (Build Settings). Half precision quantises
   the baked pivot centimetres and the cards will wobble.
2. **Generate Lightmap UVs = off**. If it writes a generated channel over
   `UV1`, the wind and ground-blend break.
3. **Add LOD1 and LOD2 manually.** Set Number of LODs to 3 and import
   `Art/Grass/SM_GrassClump_LOD1.fbx` and `_LOD2.fbx` into those slots. Do
   **not** use Generate LODs — automatic reduction destroys quad card topology.

Card random IDs are identical across all three LODs, so wind phase will not pop
on transition. `UV2` subtracts directly from `Pre-Skinned Local Position` with
no scale node, because it is already in centimetres.

---

## 7. Not done — your side

- **No LODs** on the 45 island meshes. Generate them in-editor; these are
  solid closed meshes, so automatic reduction is fine here (unlike the grass).
- **No collision** on anything. Rocks, cliffs and the arch need it. Palm
  trunks probably want a simple capsule. Foliage and clouds should stay at
  **No Collision**.
- **Cast Shadow** should be **off** on grass and small foliage.
- **Nanite**: leave off for foliage and anything masked. Rocks and cliffs are
  reasonable candidates if you want it.
- No landscape, no water, no lighting setup, no level.

---

## 8. Suggested build order

1. Build the three master materials, then instances per slot name.
2. Assign instances to every mesh's slots (bind by slot name).
3. Set collision and LODs per §7.
4. Grass cards per §6.
5. Landscape with a sand/grass/rock layer blend; Runtime Virtual Texture for
   the ground colour if you want the grass to blend at the roots.
6. Foliage types for scatter: grass clumps and small beach props at high
   density, vegetation mid, palms hand-placed.
7. Clouds placed by hand as distant silhouette, well above the island.

Scale sanity check: palms are 2.5–9 m, cliffs up to 9 m, the rock arch is 10 m
wide and walkable, clouds are 8–40 m. Build the island to suit those.
