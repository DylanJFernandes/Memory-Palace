"""Beach decoration kit - 12 scatter props."""

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

OUT = os.path.join(K.ART_ROOT, "Beach")
UP = Vector((0.0, 0.0, 1.0))
GOLDEN = 2.39996323

NUT = ("M_Coconut", (0.29, 0.18, 0.10, 1.0))
SHELL = ("M_Seashell", (0.94, 0.88, 0.76, 1.0))
STAR = ("M_Starfish", (0.93, 0.45, 0.12, 1.0))
WOOD = ("M_Driftwood", (0.58, 0.53, 0.46, 1.0))
PEBBLE = ("M_Pebble", (0.52, 0.51, 0.49, 1.0))
CORAL = ("M_Coral", (0.92, 0.46, 0.52, 1.0))
FLOWER = ("M_TropicalFlower", (0.90, 0.22, 0.30, 1.0))
LEAF = ("M_PalmLeaf", (0.21, 0.52, 0.16, 1.0))


def side_of(d):
    s = d.cross(UP)
    if s.length < 1e-5:
        s = Vector((1.0, 0.0, 0.0))
    return s.normalized()


def finish(name, bm, mats, width=None, height=None, flatten=None,
           project=True):
    obj = K.object_from_bm(name, bm, mats, smooth=True)
    K.recalc_outside(obj)
    if flatten:
        for v in obj.data.vertices:
            v.co.z *= flatten
        obj.data.update()
    if width:
        K.scale_to_width(obj, width)
    if height:
        K.scale_to_height(obj, height)
    K.drop_to_floor(obj)
    # box projection overwrites the layer named UVMap, which is where the
    # blade/frond builders already wrote per-petal 0-1 UVs -- skip it for
    # anything that needs an alpha cut-out to map correctly
    if project:
        K.box_uv(obj, scale=2.0)
    return obj


# ------------------------------------------------------------------ simple
def coconut(name, width, seed, count=1):
    rng = random.Random(seed)
    bm = bmesh.new()
    for i in range(count):
        a = i * GOLDEN
        r = 0.0 if count == 1 else 0.42
        S.add_displaced_sphere(
            bm, Vector((math.cos(a) * r, math.sin(a) * r, 0.0)),
            rng.uniform(0.46, 0.54), subdiv=3, noise=0.085,
            seed=seed + i * 13, mat=0, squash=0.88, freq=3.1)
    return finish(name, bm, K.material(*NUT), width=width)


def pebbles(name, width, seed, count=9):
    rng = random.Random(seed)
    bm = bmesh.new()
    for i in range(count):
        a = i * GOLDEN
        r = rng.uniform(0.0, 0.85)
        sz = rng.uniform(0.13, 0.30)
        S.add_displaced_sphere(
            bm, Vector((math.cos(a) * r, math.sin(a) * r, sz * 0.32)),
            sz, subdiv=2, noise=0.17, seed=seed + i * 7, mat=0,
            squash=0.62, freq=2.6)
    return finish(name, bm, K.material(*PEBBLE), width=width)


# ------------------------------------------------------------------ shells
def spiral_shell(name, width, seed):
    """Conch: logarithmic spiral swept with a shrinking tube radius."""
    rng = random.Random(seed)
    bm = bmesh.new()
    n, turns = 64, 2.6
    pts, radii = [], []
    for i in range(n + 1):
        t = i / n
        a = t * turns * math.tau
        decay = math.exp(-2.1 * t)
        pts.append(Vector((math.cos(a) * decay, math.sin(a) * decay,
                           t * 0.95 - 0.25)))
        radii.append(0.40 * decay + 0.012)
    S.add_tube(bm, pts, radii, segs=14, mat=0, rng=rng,
               ridge=0.07, ridge_freq=turns * 3.0)
    return finish(name, bm, K.material(*SHELL), width=width)


def scallop_shell(name, width, seed, ribs=11):
    """Fan shell: polar grid with radial ribs and a cupped profile."""
    bm = bmesh.new()
    rings, sectors = 7, 26
    grid = []
    for i in range(rings + 1):
        rr = i / rings
        row = []
        for j in range(sectors + 1):
            th = (-0.5 + j / sectors) * math.pi * 1.22
            rib = 0.055 * math.sin(th * ribs) * rr
            z = 0.42 * (1.0 - rr * rr) * 0.55 + rib
            row.append(bm.verts.new((math.sin(th) * rr,
                                     math.cos(th) * rr * 0.92 - 0.35,
                                     z)))
        grid.append(row)
    for i in range(rings):
        for j in range(sectors):
            try:
                f = bm.faces.new((grid[i][j], grid[i][j + 1],
                                  grid[i + 1][j + 1], grid[i + 1][j]))
                f.material_index = 0
            except ValueError:
                pass
    return finish(name, bm, K.material(*SHELL), width=width)


def starfish(name, width, seed, arms=5):
    rng = random.Random(seed)
    bm = bmesh.new()
    S.add_displaced_sphere(bm, Vector(), 0.34, subdiv=3, noise=0.09,
                           seed=seed, mat=0, squash=0.55, freq=2.4)
    for i in range(arms):
        a = i / arms * math.tau + rng.uniform(-0.05, 0.05)
        d = Vector((math.cos(a), math.sin(a), 0.0))
        pts, radii = [], []
        for k in range(7):
            t = k / 6.0
            pts.append(d * (0.22 + t * 0.78) + UP * (0.06 * (1.0 - t)))
            radii.append(0.21 * (1.0 - t) ** 0.8 + 0.012)
        S.add_tube(bm, pts, radii, segs=9, mat=0, rng=rng)
    return finish(name, bm, K.material(*STAR), width=width, flatten=0.55)


# --------------------------------------------------------------- driftwood
def driftwood(name, length, seed, forks=0):
    rng = random.Random(seed)
    bm = bmesh.new()
    n = 18
    p1, p2 = rng.uniform(0, math.tau), rng.uniform(0, math.tau)
    pts, radii = [], []
    for i in range(n + 1):
        t = i / n
        pts.append(Vector((
            (t - 0.5) * length,
            math.sin(t * 2.3 + p1) * length * 0.07,
            math.sin(t * 1.7 + p2) * length * 0.045 + length * 0.09,
        )))
        radii.append(length * (0.085 - 0.035 * abs(t - 0.45)) *
                     (1.0 + 0.10 * math.sin(t * 9.0 + p1)))
    S.add_tube(bm, pts, radii, segs=12, mat=0, rng=rng)

    for f in range(forks):
        base_i = rng.randint(4, n - 4)
        base = pts[base_i]
        a = rng.uniform(0, math.tau)
        d = Vector((math.cos(a) * 0.7, math.sin(a) * 0.7,
                    rng.uniform(0.3, 0.8))).normalized()
        blen = length * rng.uniform(0.22, 0.40)
        bp = [base + d * (blen * k / 4.0) +
              Vector((0, 0, -0.05 * blen * (k / 4.0) ** 2)) for k in range(5)]
        br = [radii[base_i] * 0.55 * (1.0 - k / 5.0) + 0.006 for k in range(5)]
        S.add_tube(bm, bp, br, segs=8, mat=0, rng=rng)
    return finish(name, bm, K.material(*WOOD), width=length)


# -------------------------------------------------------------------- coral
def coral_branch(bm, rng, base, direction, length, radius, depth, mat, col=None,
                 tint=None):
    n = 6
    pts, radii = [], []
    bend = Vector((rng.uniform(-0.3, 0.3), rng.uniform(-0.3, 0.3), 0.0))
    for k in range(n + 1):
        t = k / n
        pts.append(base + direction * (length * t) + bend * (length * t * t))
        radii.append(radius * (1.0 - 0.62 * t) + 0.004)
    start = len(bm.faces)
    S.add_tube(bm, pts, radii, segs=8, mat=mat, rng=rng)
    if col is not None and tint is not None:
        bm.faces.ensure_lookup_table()
        for f in bm.faces[start:]:
            for loop in f.loops:
                loop[col] = (tint[0], tint[1], tint[2], 1.0)
    if depth <= 0:
        return
    for _ in range(rng.randint(2, 3)):
        a = rng.uniform(0, math.tau)
        spread = rng.uniform(0.45, 0.85)
        nd = (direction + Vector((math.cos(a) * spread,
                                  math.sin(a) * spread, 0.25))).normalized()
        coral_branch(bm, rng, pts[-1], nd, length * rng.uniform(0.55, 0.75),
                     radius * 0.62, depth - 1, mat, col, tint)


def coral(name, height, seed, stems=1, tints=None):
    rng = random.Random(seed)
    bm = bmesh.new()
    col = bm.loops.layers.float_color.new("CoralTint") if tints else None
    for i in range(stems):
        a = i * GOLDEN
        r = 0.0 if stems == 1 else rng.uniform(0.12, 0.42)
        base = Vector((math.cos(a) * r, math.sin(a) * r, 0.0))
        tint = tints[i % len(tints)] if tints else None
        coral_branch(bm, rng, base,
                     Vector((rng.uniform(-0.2, 0.2), rng.uniform(-0.2, 0.2),
                             1.0)).normalized(),
                     0.42 * rng.uniform(0.85, 1.15), 0.055, 2, 0, col, tint)
    return finish(name, bm, K.material(*CORAL), height=height)


# ------------------------------------------------------------------ flower
def single_flower(name, height, seed):
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    S.add_tube(bm, [Vector((0, 0, 0)), Vector((0, 0, 0.55)),
                    Vector((0, 0, 0.78))], [0.022, 0.018, 0.014],
               segs=6, mat=1, rng=rng, uv=uv)
    centre = Vector((0.0, 0.0, 0.78))
    for p in range(7):
        pa = p / 7.0 * math.tau + rng.uniform(-0.08, 0.08)
        pd = Vector((math.cos(pa), math.sin(pa), 0.42)).normalized()
        S.add_blade(bm, centre, pd, side_of(pd), 0.34,
                    halfwidth=0.15, droop=-0.22, segs=3, mat=0,
                    taper=1.3, uv=uv, fold=0.28)
    S.add_displaced_sphere(bm, centre + UP * 0.03, 0.055, subdiv=2,
                           noise=0.1, seed=seed, mat=1, squash=0.7)
    return finish(name, bm, [K.material(*FLOWER), K.material("M_FlowerStem",
                                                             (0.26, 0.42, 0.18, 1.0))],
                  height=height, project=False)


def fallen_frond(name, length, seed):
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    d = Vector((1.0, 0.12, 0.10)).normalized()
    S.add_frond(bm, Vector((0.0, 0.0, 0.10)), d, side_of(d), length,
                leaflets=42, leaflet_len=length * 0.23, droop=0.16,
                mat_leaf=0, rng=rng, uv=uv, spine_width=0.030, scale=1.0)
    return finish(name, bm, K.material(*LEAF), width=length, project=False)


def main():
    K.reset_scene()
    builders = [
        lambda: coconut("SM_Beach_01_Coconut", 0.30, 11),
        lambda: coconut("SM_Beach_02_CoconutCluster", 0.52, 22, count=3),
        lambda: spiral_shell("SM_Beach_03_SeashellLarge", 0.40, 33),
        lambda: scallop_shell("SM_Beach_04_SeashellSmall", 0.15, 44),
        lambda: starfish("SM_Beach_05_Starfish", 0.30, 55),
        lambda: driftwood("SM_Beach_06_DriftwoodLog", 2.00, 66),
        lambda: driftwood("SM_Beach_07_DriftwoodSmall", 0.80, 77, forks=3),
        lambda: pebbles("SM_Beach_08_Pebbles", 0.60, 88),
        lambda: coral("SM_Beach_09_CoralFormation", 0.80, 99),
        lambda: coral("SM_Beach_10_CoralCluster", 1.00, 110, stems=5,
                      tints=[(0.95, 0.42, 0.50), (0.99, 0.66, 0.30),
                             (0.62, 0.48, 0.92), (0.35, 0.78, 0.74)]),
        lambda: single_flower("SM_Beach_11_TropicalFlower", 0.25, 121),
        lambda: fallen_frond("SM_Beach_12_FallenFrond", 2.50, 132),
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

    K.arrange_row(objs, gap=1.35)
    K.render_preview(os.path.join(OUT, "_preview_beach.png"), objs,
                     elevation=20.0, azimuth=34.0)
    K.save_blend(os.path.join(OUT, "Beach_Source.blend"))

    print("KIT_REPORT_BEGIN")
    print(json.dumps({"out": OUT, "assets": made}, indent=1))
    print("KIT_REPORT_END")


main()
