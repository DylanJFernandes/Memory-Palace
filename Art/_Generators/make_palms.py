"""Palm tree kit - 7 variants, separate bark / leaf / coconut material slots."""

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

OUT = os.path.join(K.ART_ROOT, "Palms")

BARK = ("M_PalmBark", (0.33, 0.24, 0.16, 1.0))
LEAF = ("M_PalmLeaf", (0.20, 0.52, 0.15, 1.0))
NUT = ("M_Coconut", (0.30, 0.19, 0.11, 1.0))

# name, height m, lean deg, bend, base radius, fronds, frond len, droop, seed,
# crown bias (0 = even, 1 = one-sided), coconuts, twist
VARIANTS = [
    ("SM_Palm_01_TallStraight", 9.0, 3.0, 0.03, 0.26, 13, 3.1, 0.52, 101, 0.0, 0, 0.5),
    ("SM_Palm_02_CurvedBeach", 7.5, 12.0, 0.26, 0.28, 12, 3.0, 0.58, 202, 0.55, 0, 0.8),
    ("SM_Palm_03_Leaning", 8.5, 30.0, 0.10, 0.25, 12, 3.2, 0.50, 303, 0.30, 0, 1.4),
    ("SM_Palm_04_Short", 4.0, 5.0, 0.08, 0.34, 14, 2.4, 0.62, 404, 0.0, 0, 0.4),
    ("SM_Palm_05_Coconut", 8.0, 7.0, 0.14, 0.27, 10, 3.0, 0.55, 505, 0.15, 7, 0.6),
    ("SM_Palm_06_Young", 2.5, 4.0, 0.05, 0.11, 8, 1.5, 0.22, 606, 0.0, 0, 0.3),
    ("SM_Palm_07_DramaticCurve", 7.0, 18.0, 0.42, 0.26, 12, 3.3, 0.60, 707, 0.65, 0, 1.0),
]


def trunk_path(height, lean_deg, bend, segs=16, twist=0.0):
    """Centreline from base to crown. Lean is linear, bend is quadratic."""
    lean = math.radians(lean_deg)
    pts = []
    for i in range(segs + 1):
        t = i / segs
        x = math.sin(lean) * height * t + bend * height * t * t
        y = twist * height * 0.035 * math.sin(t * math.pi * 1.3)
        z = math.cos(lean) * height * t
        # gentle sinuous wobble so the trunk is never a clean arc
        x += math.sin(t * math.pi * 2.1) * height * 0.012
        pts.append(Vector((x, y, z)))
    return pts


def trunk_radii(path, base_r):
    radii = []
    n = len(path) - 1
    for i in range(len(path)):
        t = i / n
        r = base_r * (1.0 - 0.46 * t)
        r *= 1.0 + 0.55 * math.exp(-t * 9.0)       # root flare
        r *= 1.0 + 0.10 * math.exp(-(t - 1.0) ** 2 * 60.0)  # crown swell
        radii.append(r)
    return radii


def build_palm(name, height, lean, bend, base_r, n_fronds, frond_len, droop,
               seed, crown_bias, coconuts, twist):
    rng = random.Random(seed)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")

    path = trunk_path(height, lean, bend, twist=twist)
    radii = trunk_radii(path, base_r)
    S.add_tube(bm, path, radii, segs=12, mat=0, ridge=0.05,
               ridge_freq=max(height * 1.4, 6.0), rng=rng, uv=uv,
               uv_vscale=max(height * 0.5, 2.0), cap_start=True, cap_end=False)

    crown = path[-1]
    tangent = (path[-1] - path[-3]).normalized()

    for i in range(n_fronds):
        base_az = (i / n_fronds) * math.tau
        # bias squashes the azimuth distribution to one side for beach palms
        az = base_az + crown_bias * math.sin(base_az) * 1.1 + rng.uniform(-0.18, 0.18)
        elev = math.radians(rng.uniform(14.0, 62.0) - crown_bias * 12.0)
        horiz = Vector((math.cos(az), math.sin(az), 0.0))
        direction = (horiz * math.cos(elev) + Vector((0, 0, 1)) * math.sin(elev))
        direction = (direction + tangent * 0.25).normalized()
        side = direction.cross(Vector((0.0, 0.0, 1.0)))
        if side.length < 1e-5:
            side = Vector((1.0, 0.0, 0.0))
        side.normalize()

        # fronds emerge slightly inside the crown so they do not float
        root = crown + direction * (base_r * 0.55) - Vector((0, 0, base_r * 0.3))
        ln = frond_len * rng.uniform(0.82, 1.14)
        S.add_frond(bm, root, direction, side, ln,
                    leaflets=int(rng.uniform(34, 46)),
                    leaflet_len=ln * rng.uniform(0.21, 0.27),
                    droop=droop * rng.uniform(0.85, 1.2),
                    mat_leaf=1, rng=rng, uv=uv, scale=1.0)

    if coconuts:
        for _ in range(coconuts):
            az = rng.uniform(0.0, math.tau)
            rad = base_r * rng.uniform(0.7, 1.5)
            pos = crown + Vector((math.cos(az) * rad, math.sin(az) * rad,
                                  -rng.uniform(0.12, 0.45)))
            S.add_displaced_sphere(bm, pos, rng.uniform(0.11, 0.145),
                                   subdiv=2, noise=0.07, seed=rng.randint(0, 9999),
                                   mat=2, squash=0.92)

    mats = [K.material(*BARK), K.material(*LEAF)]
    if coconuts:
        mats.append(K.material(*NUT))
    obj = K.object_from_bm(name, bm, mats, smooth=True)
    K.recalc_outside(obj)
    K.drop_to_floor(obj)
    K.axis_wind_mask(obj, path)   # scale-invariant, so compute before resizing
    K.scale_to_height(obj, height)
    K.drop_to_floor(obj)
    return obj, path


def main():
    K.reset_scene()
    made, objs = [], []
    for args in VARIANTS:
        obj, _ = build_palm(*args)
        objs.append(obj)
        info = K.export_fbx(obj, os.path.join(OUT, obj.name + ".fbx"))
        made.append({
            "name": obj.name,
            "tris": K.tri_count(obj),
            "verts": len(obj.data.vertices),
            "height_cm": round(obj.dimensions.z * 100.0),
            "slots": [m.name for m in obj.data.materials],
            "bytes": info["bytes"],
        })

    K.arrange_row(objs, gap=1.25)
    K.render_preview(os.path.join(OUT, "_preview_palms.png"), objs,
                     elevation=8.0, azimuth=30.0)
    K.save_blend(os.path.join(OUT, "Palms_Source.blend"))

    print("KIT_REPORT_BEGIN")
    print(json.dumps({"out": OUT, "assets": made}, indent=1))
    print("KIT_REPORT_END")


main()
