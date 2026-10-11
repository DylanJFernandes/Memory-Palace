"""Rocks and coastal cliffs - 10 stylized assets, smooth-transition geometry."""

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

OUT = os.path.join(K.ART_ROOT, "Rocks")

STONE = ("M_RockStone", (0.47, 0.47, 0.50, 1.0))
STONE_WARM = ("M_RockShore", (0.55, 0.52, 0.47, 1.0))
MOSS = ("M_RockMoss", (0.21, 0.42, 0.16, 1.0))


def finish(name, bm, mats, width=None, height=None, smooth=True, seed=0,
           noise=0.0, freq=1.6):
    obj = K.object_from_bm(name, bm, mats, smooth=smooth)
    K.recalc_outside(obj)
    if noise:
        K.noisify(obj, noise, freq=freq, seed=seed)
    if width:
        K.scale_to_width(obj, width)
    if height:
        K.scale_to_height(obj, height)
    K.drop_to_floor(obj)
    K.box_uv(obj, scale=0.5)
    return obj


def simple_rock(name, width, seed, subdiv=4, noise=0.26, squash=0.78,
                surf=0.035, mat=STONE, stretch=(1.0, 1.0, 1.0)):
    bm = bmesh.new()
    res = S.add_displaced_sphere(bm, Vector(), 1.0, subdiv=subdiv, noise=noise,
                                 seed=seed, mat=0, squash=squash, freq=2.0)
    # non-uniform stretch so no two rocks read as the same potato
    for v in res["verts"]:
        v.co.x *= stretch[0]
        v.co.y *= stretch[1]
        v.co.z *= stretch[2]
    return finish(name, bm, K.material(*mat), width=width, seed=seed,
                  noise=surf, freq=2.4)


def mossy_rock(name, width, seed):
    """Stone with moss on the upward faces - two slots, no texture needed."""
    rng = random.Random(seed)
    bm = bmesh.new()
    res = S.add_displaced_sphere(bm, Vector(), 1.0, subdiv=4, noise=0.24,
                                 seed=seed, mat=0, squash=0.8, freq=2.1)
    for f in res["faces"]:
        n = f.normal
        patch = math.sin(f.calc_center_median().x * 3.1 + seed) * \
            math.sin(f.calc_center_median().y * 2.7)
        if n.z > 0.30 and patch > -0.25:
            f.material_index = 1
    return finish(name, bm, [K.material(*STONE), K.material(*MOSS)],
                  width=width, seed=seed, noise=0.03, freq=2.6)


def flat_rock(name, width, seed):
    bm = bmesh.new()
    res = S.add_displaced_sphere(bm, Vector(), 1.0, subdiv=4, noise=0.20,
                                 seed=seed, mat=0, squash=0.36, freq=1.8)
    # shave the crown into a usable standing surface
    for v in res["verts"]:
        if v.co.z > 0.20:
            v.co.z = 0.20 + (v.co.z - 0.20) * 0.18
    return finish(name, bm, K.material(*STONE), width=width, seed=seed,
                  noise=0.02, freq=2.2)


def cliff(name, width, height, seed, depth_ratio=0.55, steps=5, taper=0.42):
    bm = bmesh.new()
    S.add_lofted_cliff(bm, width, width * depth_ratio, height,
                       layers=24, radial=26, seed=seed, steps=steps,
                       mat=0, taper=taper)
    return finish(name, bm, K.material(*STONE), width=width, height=height,
                  seed=seed, noise=height * 0.006, freq=1.3)


def rock_arch(name, width, seed):
    """Semicircular span, thick enough to walk under."""
    rng = random.Random(seed)
    bm = bmesh.new()
    n = 34
    pts, radii = [], []
    for i in range(n + 1):
        a = math.pi * (i / n)
        # slightly elliptical so the opening is taller than a true half-circle
        pts.append(Vector((-math.cos(a) * width * 0.5,
                           math.sin(a * 0.5) * width * 0.06,
                           math.sin(a) * width * 0.52)))
        # smooth thickness variation: per-ring randomness banded the tube
        thick = (0.15 + 0.06 * math.sin(a)
                 + 0.025 * math.sin(a * 3.0 + 1.1)
                 + 0.015 * math.sin(a * 5.0 + 2.3))
        radii.append(width * thick)
    S.add_tube(bm, pts, radii, segs=20, mat=0, rng=rng)
    return finish(name, bm, K.material(*STONE), width=width, seed=seed,
                  noise=width * 0.012, freq=1.1)


def rock_cluster(name, width, seed, count=5):
    rng = random.Random(seed)
    bm = bmesh.new()
    for i in range(count):
        a = rng.uniform(0.0, math.tau)
        r = rng.uniform(0.0, 0.42)
        size = rng.uniform(0.45, 0.95)
        S.add_displaced_sphere(
            bm, Vector((math.cos(a) * r, math.sin(a) * r, size * 0.38)),
            size, subdiv=3, noise=0.24, seed=seed + i * 17, mat=0,
            squash=0.74, freq=2.3)
    return finish(name, bm, K.material(*STONE), width=width, seed=seed,
                  noise=0.02, freq=2.8)


def shoreline_rock(name, width, seed):
    """Wave-eroded: low noise, strong horizontal smoothing."""
    bm = bmesh.new()
    S.add_displaced_sphere(bm, Vector(), 1.0, subdiv=4, noise=0.17,
                           seed=seed, mat=0, squash=0.52, freq=1.5)
    return finish(name, bm, K.material(*STONE_WARM), width=width, seed=seed,
                  noise=0.012, freq=1.4)


def main():
    K.reset_scene()
    builders = [
        lambda: simple_rock("SM_Rock_01_BeachSmall", 0.5, 11, subdiv=3,
                            noise=0.16, squash=0.80, surf=0.02,
                            stretch=(1.1, 0.9, 1.0)),
        lambda: simple_rock("SM_Rock_02_BeachMedium", 1.5, 22, subdiv=4,
                            noise=0.24, squash=0.76, surf=0.03,
                            stretch=(1.25, 0.82, 1.0)),
        lambda: simple_rock("SM_Rock_03_CoastalBoulder", 3.5, 33, subdiv=5,
                            noise=0.32, squash=0.84, surf=0.05,
                            stretch=(1.0, 0.78, 1.12)),
        lambda: mossy_rock("SM_Rock_04_Mossy", 2.0, 44),
        lambda: flat_rock("SM_Rock_05_FlatFormation", 4.0, 55),
        lambda: cliff("SM_Rock_06_CliffSegment", 8.0, 5.0, 66, steps=5,
                      taper=0.26),
        lambda: cliff("SM_Rock_07_CliffTall", 6.0, 9.0, 77, depth_ratio=0.75,
                      steps=6, taper=0.34),
        lambda: rock_arch("SM_Rock_08_Arch", 10.0, 88),
        lambda: rock_cluster("SM_Rock_09_Cluster", 3.0, 99, count=5),
        lambda: shoreline_rock("SM_Rock_10_Shoreline", 5.0, 110),
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
    K.render_preview(os.path.join(OUT, "_preview_rocks.png"), objs,
                     elevation=16.0, azimuth=34.0)
    K.save_blend(os.path.join(OUT, "Rocks_Source.blend"))

    print("KIT_REPORT_BEGIN")
    print(json.dumps({"out": OUT, "assets": made}, indent=1))
    print("KIT_REPORT_END")


main()
