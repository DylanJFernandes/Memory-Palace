# Tropical island asset kit

45 procedurally generated assets across five categories, built in Blender 4.5
and exported for Unreal. Source `.blend` and a contact-sheet PNG sit beside the
FBX files in each folder.

## Contents

| Folder | Count | Assets |
|---|---|---|
| `Palms/` | 7 | Tall straight, curved beach, leaning, short, coconut, young, dramatic curve |
| `Vegetation/` | 10 | 2 bushes, 2 ferns, broadleaf, flower cluster, 3 grasses, vine |
| `Rocks/` | 10 | 3 beach rocks, mossy, flat formation, 2 cliffs, arch, cluster, shoreline |
| `Clouds/` | 6 | Small puffy, medium puffy, large cumulus, long horizontal, dramatic, wispy |
| `Beach/` | 12 | Coconut ×2, shells ×2, starfish, driftwood ×2, pebbles, coral ×2, flower, fallen frond |

`Clouds/` also holds five earlier clouds (`SM_Cloud_A`–`E`) from a previous
request. They are superseded by `SM_Cloud_01`–`06` and can be deleted.

## Export format

FBX, **Y-up / −Z forward**, scale 1, transforms applied, every mesh dropped to
`z = 0` so it places by its base. Blender metres map to Unreal centimetres at
import scale 1.0.

All dimensions were normalised to the brief after construction — fronds and
leaves push a silhouette past its trunk height, so construction height is never
finished height. Verified: palms land at exactly 900 / 750 / 850 / 400 / 800 /
250 / 700 cm.

## Material slots

Flat colours only (see *Not done* below). Slot names are what Unreal will create:

- Palms — `M_PalmBark`, `M_PalmLeaf`, plus `M_Coconut` on palm 05
- Vegetation — `M_PlantLeaf`, `M_PlantLeafDeep`, `M_TropicalFlower`,
  `M_PlantGrass`, `M_BeachGrass`, `M_VineStem`
- Rocks — `M_RockStone`, `M_RockMoss` (rock 04), `M_RockShore` (rock 10)
- Clouds — `M_Cloud`
- Beach — `M_Coconut`, `M_Seashell`, `M_Starfish`, `M_Driftwood`, `M_Pebble`,
  `M_Coral`, `M_TropicalFlower`, `M_FlowerStem`, `M_PalmLeaf`

## Vertex colour channels

Vertex colours are used rather than extra UV channels because Unreal flips V on
every UV channel at FBX import; vertex colours come through untouched.

| Channel | On | Meaning |
|---|---|---|
| `WindMask` | Palms, vegetation | 0 on the stem, 1 at leaf tips. Drives wind amplitude. |
| `VertGradient` | Clouds | 0 at the underside, 1 at the top. Tint the base bluer. |
| `FlowerTint` | Flower cluster | Per-head red / orange / yellow. |
| `CoralTint` | Coral cluster | Per-stem colour variation. |

Palm and plant wind masks are built from **radial distance to the stem**, not a
Z ramp — a Z ramp is wrong for palms because fronds droop below the crown and
would get less wind than the trunk.

## Triangle budgets

Palms 3.3k–9.9k · vegetation 0.5k–5.7k · rocks 0.3k–5.1k · clouds 0.9k–2.6k ·
beach 0.2k–5.4k. Nothing has LODs; generate them in Unreal or ask and I will
author manual ones as I did for the grass cards.

## Not done — read before planning the look

1. ~~No texture images.~~ **Done** — 8 texture sets (21 maps) now live in
   `Textures/`, covering every material slot except `M_Cloud`, which is driven
   by vertex colour instead. See `Textures/README.md` for slot mapping and
   Unreal import settings. Materials in the FBX files are still flat colours;
   bind the textures on the Unreal side.
2. **UVs are projections, not unwraps.** Cylindrical along palm trunks,
   per-leaf 0–1 on blades and fronds, box projection on rocks and beach props,
   spherical on clouds. Good enough for tiling materials and triplanar; not a
   hand-cut seam layout for painted texture work.
3. **Weakest assets:** `SM_Beach_03_SeashellLarge` and
   `SM_Beach_04_SeashellSmall`. The conch spiral and scallop fan are the two
   shapes procedural generation served worst — worth replacing by hand.
4. **Quality ceiling.** These are solid, varied, correctly-scaled kit pieces.
   They are not hand-sculpted hero assets, and procedural generation will not
   reach Kingdom Hearts polish on silhouette detail no matter how it is tuned.

## Regenerating

Generators are in `_Generators/`. Each runs standalone:

```bash
"C:/Program Files/Blender Foundation/Blender 4.5/blender.exe" --background --factory-startup --python _Generators/make_palms.py
```

`bl_kit.py` holds the export contract, scaling and preview helpers;
`bl_shapes.py` holds the geometry primitives (swept tubes, leaf blades, fronds,
displaced spheres, lofted cliffs). Variant tables at the top of each `make_*.py`
control counts, sizes and seeds — all output is deterministic per seed.
