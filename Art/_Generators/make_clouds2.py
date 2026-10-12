"""Stylized cloud kit - 6 meshes per the tropical island brief.

Metaballs fuse into one surface, so no sphere intersections are visible. The
key constraint: lobe spacing must stay below lobe radius or the field never
merges and the cloud renders as a row of separate balls. Spacing is therefore
derived from the radius, and the flat/tall profile is applied afterwards by
scaling Z rather than by spreading the lobes apart.
"""

import json
import math
import os
import random
import sys

import bpy

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bl_kit as K

OUT = os.path.join(K.ART_ROOT, "Clouds")

# name, width m, height m, masses, target tris, seed, depth ratio, layered
VARIANTS = [
    ("SM_Cloud_01_SmallPuffy", 8.0, 4.2, 4, 900, 11, 0.42, False),
    ("SM_Cloud_02_MediumPuffy", 16.0, 7.4, 6, 1400, 22, 0.40, False),
    ("SM_Cloud_03_LargeCumulus", 30.0, 18.5, 8, 2400, 33, 0.44, True),
    ("SM_Cloud_04_LongHorizontal", 40.0, 6.8, 11, 1800, 44, 0.14, False),
    ("SM_Cloud_05_DramaticFantasy", 35.0, 24.0, 9, 2600, 55, 0.40, True),
    ("SM_Cloud_06_SmallWispy", 12.0, 2.4, 6, 900, 66, 0.18, False),
]


def blobs_for(rng, width, height, depth, layered):
    """Pack lobes through an ellipsoid volume.

    A single row of lobes spaced at roughly one radius produces visible ribs,
    so lobe radius is tied to the *thinnest* axis and the volume is filled with
    many heavily-overlapping lobes instead.
    """
    rx, ry, rz = width * 0.5, depth * 0.5, height * 0.5
    # lobes as fat as the thin axis allows: big lobes fuse smoothly, small ones
    # leave the grape-cluster gaps that random packing produced
    r0 = 0.78 * min(ry, rz)
    tiers = max(2 if layered else 1, int(rz / (r0 * 0.75)))

    out = []
    for tier in range(tiers):
        tz = r0 * 0.85 + tier * r0 * 0.78
        shrink = 1.0 - (tier / max(tiers, 1)) * (0.52 if layered else 0.72)
        span = max((rx - r0 * 0.7) * shrink * 2.0, r0)
        n = max(2, int(span / (r0 * 0.62)) + 1)
        for i in range(n):
            t = i / max(n - 1, 1)
            # chain spacing stays well under r0, so the field never breaks
            out.append((
                ((t - 0.5) * span + rng.uniform(-0.12, 0.12) * r0,
                 rng.uniform(-0.6, 0.6) * max(ry - r0 * 0.7, 0.0),
                 tz + rng.uniform(-0.15, 0.15) * r0),
                r0 * rng.uniform(0.86, 1.22) * (0.80 + 0.30 * math.sin(t * math.pi)),
            ))
    return out


def scale_axis(obj, axis, target):
    idx = "xyz".index(axis)
    vals = [v.co[idx] for v in obj.data.vertices]
    cur = max(max(vals) - min(vals), 1e-6)
    f = target / cur
    for v in obj.data.vertices:
        v.co[idx] *= f
    obj.data.update()


def build_cloud(name, width, height, masses, target_tris, seed, depth_ratio,
                layered):
    rng = random.Random(seed)
    depth = width * depth_ratio
    tag = name.split("_")[2]

    mb = bpy.data.metaballs.new("MB" + tag)
    r0 = 0.52 * min(depth, height) * 0.5
    mb.resolution = max(r0 * 0.20, 0.06)
    mb.threshold = 0.6
    mb_obj = bpy.data.objects.new("MB" + tag, mb)
    bpy.context.scene.collection.objects.link(mb_obj)
    for co, r in blobs_for(rng, width, height, depth, layered):
        el = mb.elements.new()
        el.type = "BALL"
        el.co = co
        el.radius = r

    bpy.ops.object.select_all(action="DESELECT")
    mb_obj.select_set(True)
    bpy.context.view_layer.objects.active = mb_obj
    bpy.ops.object.convert(target="MESH")
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name

    before = K.tri_count(obj)
    if before > target_tris:
        mod = obj.modifiers.new("Decimate", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = target_tris / before
        bpy.ops.object.modifier_apply(modifier=mod.name)

    for poly in obj.data.polygons:
        poly.use_smooth = True
    obj.data.materials.append(K.material("M_Cloud", (0.94, 0.96, 1.0, 1.0)))

    # profile is applied here, after fusion, so it cannot break the mesh apart
    K.scale_to_width(obj, width)
    # safe now: the surface is already fused, so a mild Z correction
    # cannot separate lobes the way pre-fusion spreading did
    scale_axis(obj, "z", height)
    K.drop_to_floor(obj)
    K.spherical_uv(obj)
    K.vertical_gradient(obj, name="VertGradient")  # 0 underside, 1 top
    return obj


def main():
    K.reset_scene()
    made, objs = [], []
    for args in VARIANTS:
        obj = build_cloud(*args)
        objs.append(obj)
        info = K.export_fbx(obj, os.path.join(OUT, obj.name + ".fbx"))
        made.append({
            "name": obj.name,
            "tris": K.tri_count(obj),
            "size_cm": [round(d * 100) for d in obj.dimensions],
            "loose_parts": None,
            "bytes": info["bytes"],
        })

    K.arrange_row(objs, gap=1.2)
    K.render_preview(os.path.join(OUT, "_preview_clouds6.png"), objs,
                     elevation=10.0, azimuth=30.0)
    K.save_blend(os.path.join(OUT, "Clouds6_Source.blend"))

    print("KIT_REPORT_BEGIN")
    print(json.dumps({"out": OUT, "assets": made}, indent=1))
    print("KIT_REPORT_END")


main()
