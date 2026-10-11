"""Stylized grass blade cards with Unreal-packed UV data channels.

Builds, in a dedicated collection:

  SM_GrassClump_LOD0/1/2   card clusters, manually authored LODs
  HP_GrassBlades_Atlas     high-poly flat-laid blades, 3 atlas cells, for baking
  CAM_GrassBake            orthographic top-down camera framing the atlas

UV channel allocation on the card meshes:

  UV0  standard 0-1 texture mapping
  UV1  U = per-card pseudo-random ID (0-1), V = normalized root-to-tip height
  UV2  U = card pivot X, V = card pivot Y   (mesh local space, centimetres)

Unreal flips V on every UV channel at FBX import, and negates Y when it
converts Blender's right-handed space to its own left-handed space. Both are
compensated here, so the values listed above are what the *material* reads in
Unreal, not what the Blender UV editor shows.
"""

import math
import random

import bmesh
import bpy
from mathutils import Vector

SEED = 20261010
CM = 100.0  # Blender metres -> Unreal centimetres
COLL_ROOT = "GrassCards_MCP"
COLL_BAKE = "GrassCards_BakeSource"

# card count per LOD is a subset of the LOD0 card list, so a card that survives
# into a lower LOD keeps the random ID it had in LOD0 -- the shader reads that
# ID for per-card wind phase, and a mismatch would pop on LOD transition.
LOD_SPEC = [
    ("LOD0", list(range(12)), 6, "MI_GrassCard_Near"),
    ("LOD1", [0, 1, 2, 4, 5, 7, 9, 11], 3, "MI_GrassCard_Mid"),
    ("LOD2", [0, 3, 6, 9], 1, "MI_GrassCard_Far"),
]

ATLAS_CELLS = 3
BLADES_PER_CELL = 16


# --------------------------------------------------------------------- utils
def ue_v(value):
    """Blender V to store so Unreal receives `value` after its import V-flip."""
    return 1.0 - value


def ue_xy(x, y):
    """Blender local XY (metres) -> Unreal local XY (centimetres)."""
    return x * CM, -y * CM


def get_collection(name, parent=None):
    coll = bpy.data.collections.get(name)
    if coll is None:
        coll = bpy.data.collections.new(name)
        (parent or bpy.context.scene.collection).children.link(coll)
    return coll


def purge(coll):
    for ob in list(coll.objects):
        data = ob.data
        bpy.data.objects.remove(ob, do_unlink=True)
        if data is not None and data.users == 0:
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)


def get_material(name, rgba):
    mat = bpy.data.materials.get(name)
    if mat is None:
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        if bsdf is not None:
            bsdf.inputs["Base Color"].default_value = rgba
    return mat


def finish(mesh, up_bias=0.75):
    """Smooth-shade and bias vertex normals upward for soft foliage lighting."""
    for poly in mesh.polygons:
        poly.use_smooth = True
    up = Vector((0.0, 0.0, 1.0))
    try:
        normals = [(up * up_bias + v.normal * (1.0 - up_bias)).normalized()
                   for v in mesh.vertices]
        mesh.normals_split_custom_set_from_vertices(normals)
    except Exception as exc:  # API differs across Blender versions
        print("custom normals skipped:", exc)


# ----------------------------------------------------------------- card defs
def build_card_defs():
    """Deterministic clump layout. Rings keep cards spaced for clean LODs."""
    rng = random.Random(SEED)
    defs = []
    for radius, count in ((0.0, 1), (0.105, 4), (0.195, 7)):
        for i in range(count):
            angle = (i / count) * math.tau + rng.uniform(-0.18, 0.18)
            falloff = 1.0 - 0.3 * (radius / 0.195)
            px = math.cos(angle) * radius + rng.uniform(-0.012, 0.012)
            py = math.sin(angle) * radius + rng.uniform(-0.012, 0.012)
            defs.append({
                "pivot": (px, py),
                # cards face outward from the clump centre, heavily jittered so
                # the cluster reads from every camera direction
                "yaw": angle + math.pi * 0.5 + rng.uniform(-0.6, 0.6),
                "height": rng.uniform(0.27, 0.43) * falloff,
                "width": rng.uniform(0.17, 0.26),
                "bend": rng.uniform(0.03, 0.09),
                "lean": rng.uniform(-0.09, 0.09),
                "rid": rng.random(),
            })
    return defs


def build_card_lod(name, cards, segs, material):
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    uv0 = bm.loops.layers.uv.new("UV0Texture")
    uv1 = bm.loops.layers.uv.new("UV1IdHeight")
    uv2 = bm.loops.layers.uv.new("UV2Pivot")

    for card in cards:
        px, py = card["pivot"]
        pux, puy = ue_xy(px, py)
        cos_y, sin_y = math.cos(card["yaw"]), math.sin(card["yaw"])
        half = card["width"] * 0.5

        rows = []
        for i in range(segs + 1):
            t = i / segs
            # forward lean plus quadratic bend gives the card real 3D volume
            fwd = card["bend"] * t * t + card["lean"] * t
            row = []
            for side in (-1.0, 1.0):
                lx, ly = side * half, fwd
                wx = lx * cos_y - ly * sin_y + px
                wy = lx * sin_y + ly * cos_y + py
                row.append(bm.verts.new((wx, wy, card["height"] * t)))
            rows.append((row, t))

        for i in range(segs):
            (lo, t0), (hi, t1) = rows[i], rows[i + 1]
            face = bm.faces.new((lo[0], lo[1], hi[1], hi[0]))
            for loop, cu, ct in zip(face.loops, (0.0, 1.0, 1.0, 0.0),
                                    (t0, t0, t1, t1)):
                loop[uv0].uv = (cu, ct)
                loop[uv1].uv = (card["rid"], ue_v(ct))
                loop[uv2].uv = (pux, ue_v(puy))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    finish(mesh)
    mesh.materials.append(material)

    obj = bpy.data.objects.new(name, mesh)
    return obj


# ------------------------------------------------------- high-poly bake source
def build_bake_atlas(name):
    """Flat-laid blades in `ATLAS_CELLS` 1x1 cells, for an ortho top-down bake.

    Vertex colour layers carry the masks the spec wants baked out, so each map
    is an emission bake of one channel rather than a hand-authored material.
    """
    rng = random.Random(SEED + 7)
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    c_grad = bm.loops.layers.float_color.new("RootTipGradient")
    c_edge = bm.loops.layers.float_color.new("EdgeToCentre")
    c_id = bm.loops.layers.float_color.new("BladeID")

    segs = 10
    for cell in range(ATLAS_CELLS):
        x0 = cell * 1.0
        for _ in range(BLADES_PER_CELL):
            rx = x0 + rng.uniform(0.12, 0.88)
            ry = rng.uniform(0.01, 0.10)
            length = rng.uniform(0.55, 0.92)
            theta = rng.uniform(-0.55, 0.55)
            curve = rng.uniform(-0.30, 0.30)
            w0 = rng.uniform(0.028, 0.052)
            bulge = rng.uniform(0.012, 0.028)
            bid = rng.random()

            rows = []
            for i in range(segs + 1):
                t = i / segs
                taper = max((1.0 - t) ** 0.7, 0.0)
                # drift sideways with a quadratic curve for a natural arc
                cx = rx + math.sin(theta) * length * t + curve * t * t
                cy = ry + math.cos(theta) * length * t
                hw = w0 * taper
                z = bulge * taper
                rows.append((
                    [bm.verts.new((cx - hw, cy, 0.0)),
                     bm.verts.new((cx, cy, z)),
                     bm.verts.new((cx + hw, cy, 0.0))],
                    t,
                ))

            for i in range(segs):
                (lo, t0), (hi, t1) = rows[i], rows[i + 1]
                for a in (0, 1):
                    face = bm.faces.new((lo[a], lo[a + 1], hi[a + 1], hi[a]))
                    corners = ((a, t0), (a + 1, t0), (a + 1, t1), (a, t1))
                    for loop, (col, ct) in zip(face.loops, corners):
                        edge = 1.0 if col == 1 else 0.0
                        loop[c_grad] = (ct, ct, ct, 1.0)
                        loop[c_edge] = (edge, edge, edge, 1.0)
                        loop[c_id] = (bid, bid, bid, 1.0)

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    finish(mesh, up_bias=0.35)
    mesh.materials.append(get_material("M_GrassBakeSource", (0.18, 0.42, 0.12, 1.0)))
    return bpy.data.objects.new(name, mesh)


def build_bake_camera(name):
    cam = bpy.data.cameras.new(name)
    cam.type = "ORTHO"
    # frames the full ATLAS_CELLS x 1 strip
    cam.ortho_scale = float(ATLAS_CELLS)
    obj = bpy.data.objects.new(name, cam)
    obj.location = (ATLAS_CELLS * 0.5, 0.5, 2.0)
    obj.rotation_euler = (0.0, 0.0, 0.0)  # looking straight down -Z
    return obj


# ---------------------------------------------------------------------- main
def run():
    root = get_collection(COLL_ROOT)
    bake = get_collection(COLL_BAKE, parent=root)
    purge(root)
    purge(bake)

    cards = build_card_defs()
    colors = {
        "MI_GrassCard_Near": (0.16, 0.40, 0.10, 1.0),
        "MI_GrassCard_Mid": (0.20, 0.44, 0.13, 1.0),
        "MI_GrassCard_Far": (0.24, 0.47, 0.17, 1.0),
    }

    report = {"lods": [], "atlas_cells": ATLAS_CELLS}
    for suffix, indices, segs, mat_name in LOD_SPEC:
        subset = [cards[i] for i in indices]
        obj = build_card_lod(
            "SM_GrassClump_%s" % suffix, subset, segs,
            get_material(mat_name, colors[mat_name]),
        )
        root.objects.link(obj)
        report["lods"].append({
            "name": obj.name,
            "cards": len(subset),
            "segments": segs,
            "verts": len(obj.data.vertices),
            "tris": sum(len(p.vertices) - 2 for p in obj.data.polygons),
            "material": mat_name,
            "ids": [round(c["rid"], 4) for c in subset],
        })

    atlas = build_bake_atlas("HP_GrassBlades_Atlas")
    bake.objects.link(atlas)
    cam = build_bake_camera("CAM_GrassBake")
    bake.objects.link(cam)
    report["bake"] = {
        "atlas": atlas.name,
        "verts": len(atlas.data.vertices),
        "tris": sum(len(p.vertices) - 2 for p in atlas.data.polygons),
        "blades": ATLAS_CELLS * BLADES_PER_CELL,
        "camera": cam.name,
        "vertex_colors": [l.name for l in atlas.data.color_attributes],
    }
    return report


print("GRASS_REPORT_BEGIN")
import json as _json
print(_json.dumps(run(), indent=1))
print("GRASS_REPORT_END")
