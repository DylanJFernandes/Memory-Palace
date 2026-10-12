"""Tropical vegetation kit - 10 assets, wind mask in vertex colour."""

import json
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bl_kit as K
import bl_shapes as S

OUT = os.path.join(K.ART_ROOT, "Vegetation")
UP = Vector((0.0, 0.0, 1.0))
GOLDEN = 2.39996323

LEAF = ("M_PlantLeaf", (0.19, 0.55, 0.16, 1.0))
LEAF_DEEP = ("M_PlantLeafDeep", (0.13, 0.38, 0.12, 1.0))
FLOWER = ("M_TropicalFlower", (0.86, 0.17, 0.12, 1.0))
GRASS = ("M_PlantGrass", (0.32, 0.60, 0.20, 1.0))
BEACH = ("M_BeachGrass", (0.66, 0.68, 0.33, 1.0))
STEM = ("M_VineStem", (0.30, 0.34, 0.17, 1.0))


def side_of(direction):
    s = direction.cross(UP)
    if s.length < 1e-5:
        s = Vector((1.0, 0.0, 0.0))
    return s.normalized()


def finish(name, bm, mats, height=None, wind_lo=0.0):
    obj = K.object_from_bm(name, bm, mats, smooth=True)
    K.recalc_outside(obj)
    K.drop_to_floor(obj)
    if height:
        K.scale_to_height(obj, height)
        K.drop_to_floor(obj)
    K.vertical_gradient(obj, name="WindMask", lo=wind_lo)
    return obj


# --------------------------------------------------------------------- bush
def bush(name, height, leaves, leaf_len, seed, squash=0.72, mat=LEAF,
         spread=1.0):
    """Leaves distributed over an ellipsoid shell by golden-angle spiral."""
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    radius = height * 0.42 * spread
    for i in range(leaves):
        u = (i + 0.5) / leaves
        phi = math.acos(1.0 - 1.72 * u)        # bias to the upper dome
        theta = i * GOLDEN
        d = Vector((math.sin(phi) * math.cos(theta),
                    math.sin(phi) * math.sin(theta),
                    math.cos(phi) * squash))
        if d.length < 1e-6:
            continue
        d.normalize()
        root = Vector((0.0, 0.0, height * 0.30)) + d * (radius * 0.35)
        out = (d * 0.55 + UP * 0.50).normalized()
        S.add_blade(bm, root, out, side_of(out),
                    leaf_len * rng.uniform(0.80, 1.15),
                    halfwidth=leaf_len * rng.uniform(0.38, 0.52),
                    droop=rng.uniform(0.35, 0.70),
                    curve=rng.uniform(-0.16, 0.16), segs=3, mat=0,
                    taper=2.4, uv=uv, fold=0.30)
    return finish(name, bm, K.material(*mat), height=height)


# --------------------------------------------------------------------- fern
def fern(name, height, fronds, seed, droop=0.30):
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    for i in range(fronds):
        az = i * GOLDEN + rng.uniform(-0.2, 0.2)
        elev = math.radians(rng.uniform(48.0, 82.0))
        d = Vector((math.cos(az) * math.cos(elev),
                    math.sin(az) * math.cos(elev),
                    math.sin(elev))).normalized()
        ln = height * rng.uniform(0.72, 1.05)
        S.add_frond(bm, Vector((0.0, 0.0, height * 0.06)), d, side_of(d), ln,
                    leaflets=int(rng.uniform(26, 36)),
                    leaflet_len=ln * rng.uniform(0.15, 0.21),
                    droop=droop * rng.uniform(0.8, 1.25),
                    mat_leaf=0, rng=rng, uv=uv,
                    spine_width=0.012, scale=1.0)
    return finish(name, bm, K.material(*LEAF_DEEP), height=height)


# ---------------------------------------------------------------- broadleaf
def broadleaf(name, height, leaves, seed):
    """Few, very large elephant-ear style leaves on short stems."""
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    for i in range(leaves):
        az = i * GOLDEN + rng.uniform(-0.25, 0.25)
        elev = math.radians(rng.uniform(34.0, 76.0))
        d = Vector((math.cos(az) * math.cos(elev),
                    math.sin(az) * math.cos(elev),
                    math.sin(elev))).normalized()
        stem_len = height * rng.uniform(0.30, 0.52)
        base = Vector((0.0, 0.0, height * 0.04))
        tip = base + d * stem_len
        S.add_tube(bm, [base, base + d * stem_len * 0.5, tip],
                   [0.022, 0.018, 0.013], segs=6, mat=1, rng=rng, uv=uv)
        blade_dir = (d * 0.55 + UP * 0.45).normalized()
        ln = height * rng.uniform(0.40, 0.58)
        S.add_blade(bm, tip, blade_dir, side_of(blade_dir), ln,
                    halfwidth=ln * rng.uniform(0.34, 0.46),
                    droop=rng.uniform(0.35, 0.62),
                    curve=rng.uniform(-0.1, 0.1), segs=6, mat=0,
                    taper=2.6, uv=uv, fold=0.30)
    return finish(name, bm, [K.material(*LEAF), K.material(*STEM)],
                  height=height)


# ------------------------------------------------------------------- flower
def flower_cluster(name, height, seed):
    """Green base foliage with red/orange/yellow heads; hue in vertex colour."""
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    col = bm.loops.layers.float_color.new("FlowerTint")

    for i in range(34):
        az = i * GOLDEN
        elev = math.radians(rng.uniform(22.0, 72.0))
        d = Vector((math.cos(az) * math.cos(elev),
                    math.sin(az) * math.cos(elev),
                    math.sin(elev))).normalized()
        ln = height * rng.uniform(0.34, 0.58)
        S.add_blade(bm, Vector((0.0, 0.0, height * 0.05)), d, side_of(d), ln,
                    halfwidth=ln * rng.uniform(0.14, 0.22),
                    droop=rng.uniform(0.25, 0.5), segs=3, mat=0,
                    taper=1.9, uv=uv, fold=0.40)

    tints = [(0.88, 0.14, 0.10), (0.95, 0.48, 0.07), (0.98, 0.83, 0.16)]
    for i in range(9):
        az = i * GOLDEN
        r = height * rng.uniform(0.10, 0.30)
        centre = Vector((math.cos(az) * r, math.sin(az) * r,
                         height * rng.uniform(0.62, 0.98)))
        tint = tints[i % len(tints)]
        start = len(bm.faces)
        petal_len = height * rng.uniform(0.11, 0.17)
        for p in range(6):
            pa = p / 6.0 * math.tau + rng.uniform(-0.1, 0.1)
            pd = Vector((math.cos(pa), math.sin(pa), 0.55)).normalized()
            S.add_blade(bm, centre, pd, side_of(pd), petal_len,
                        halfwidth=petal_len * 0.40, droop=-0.18, segs=2,
                        mat=1, taper=1.4, uv=uv, fold=0.25)
        bm.faces.ensure_lookup_table()
        for f in bm.faces[start:]:
            for loop in f.loops:
                loop[col] = (tint[0], tint[1], tint[2], 1.0)

    return finish(name, bm, [K.material(*LEAF), K.material(*FLOWER)],
                  height=height)


# -------------------------------------------------------------------- grass
def grass_clump(name, height, blades, seed, mat=GRASS, lean=0.35, width=0.11):
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    for i in range(blades):
        az = rng.uniform(0.0, math.tau)
        r = rng.uniform(0.0, height * 0.22)
        root = Vector((math.cos(az) * r, math.sin(az) * r, 0.0))
        tilt = rng.uniform(0.08, lean)
        d = Vector((math.cos(az) * tilt, math.sin(az) * tilt, 1.0)).normalized()
        ln = height * rng.uniform(0.62, 1.05)
        S.add_blade(bm, root, d, side_of(d), ln,
                    halfwidth=ln * width * rng.uniform(0.7, 1.3),
                    droop=rng.uniform(0.18, 0.55),
                    curve=rng.uniform(-0.2, 0.2), segs=4, mat=0,
                    taper=2.2, uv=uv, fold=0.45)
    return finish(name, bm, K.material(*mat), height=height)


# --------------------------------------------------------------------- vine
def vine(name, length, seed):
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    n = 26
    pts, radii = [], []
    p1, p2 = rng.uniform(0, math.tau), rng.uniform(0, math.tau)
    for i in range(n + 1):
        t = i / n
        pts.append(Vector((
            math.sin(t * 3.1 + p1) * length * 0.13,
            math.sin(t * 2.2 + p2) * length * 0.10,
            length * t,
        )))
        radii.append(length * 0.011 * (1.0 - 0.45 * t))
    S.add_tube(bm, pts, radii, segs=7, mat=1, rng=rng, uv=uv)

    for i in range(30):
        t = 0.05 + 0.92 * (i / 29.0)
        idx = min(int(t * n), n - 1)
        base = pts[idx]
        az = i * GOLDEN
        d = Vector((math.cos(az), math.sin(az), rng.uniform(-0.35, 0.15)))
        d.normalize()
        ln = length * rng.uniform(0.045, 0.075)
        S.add_blade(bm, base, d, side_of(d), ln,
                    halfwidth=ln * rng.uniform(0.32, 0.46),
                    droop=rng.uniform(0.2, 0.5), segs=2, mat=0,
                    taper=1.7, uv=uv, fold=0.3)
    return finish(name, bm, [K.material(*LEAF), K.material(*STEM)],
                  height=length)


def main():
    K.reset_scene()
    builders = [
        lambda: bush("SM_Veg_01_BushLarge", 1.5, 150, 0.40, 11),
        lambda: bush("SM_Veg_02_BushSmall", 0.6, 70, 0.21, 22, spread=1.1),
        lambda: fern("SM_Veg_03_FernLarge", 1.2, 11, 33),
        lambda: fern("SM_Veg_04_FernSmall", 0.45, 9, 44, droop=0.24),
        lambda: broadleaf("SM_Veg_05_Broadleaf", 1.8, 9, 55),
        lambda: flower_cluster("SM_Veg_06_FlowerCluster", 0.6, 66),
        lambda: grass_clump("SM_Veg_07_TallGrass", 0.8, 46, 77),
        lambda: grass_clump("SM_Veg_08_ShortGrass", 0.25, 38, 88,
                            lean=0.45, width=0.085),
        lambda: grass_clump("SM_Veg_09_BeachGrass", 0.5, 40, 99, mat=BEACH,
                            lean=0.55, width=0.07),
        lambda: vine("SM_Veg_10_Vine", 3.0, 110),
    ]

    made, objs = [], []
    for build in builders:
        obj = build()
        objs.append(obj)
        info = K.export_fbx(obj, os.path.join(OUT, obj.name + ".fbx"))
        made.append({
            "name": obj.name,
            "tris": K.tri_count(obj),
            "size_cm": [round(d * 100) for d in obj.dimensions],
            "slots": [m.name for m in obj.data.materials],
            "bytes": info["bytes"],
        })

    K.arrange_row(objs, gap=1.3)
    K.render_preview(os.path.join(OUT, "_preview_vegetation.png"), objs,
                     elevation=12.0, azimuth=32.0)
    K.save_blend(os.path.join(OUT, "Vegetation_Source.blend"))

    print("KIT_REPORT_BEGIN")
    print(json.dumps({"out": OUT, "assets": made}, indent=1))
    print("KIT_REPORT_END")


main()
