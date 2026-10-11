"""Alpha cut-out texture sets: leaf, grass blade, flower petal.

Rendered from real geometry rather than authored procedurally, so the
silhouette matches the per-leaf 0-1 UVs the meshes already carry. Each shape
is built lying in XY filling the 0..1 square, then rendered twice with an
orthographic camera framing exactly that square:

  BaseColor  emission of a vertex-colour tint, transparent film -> RGBA cutout
  Normal     emission of the surface normal, remapped to 0..1

A flat shape in XY has its tangent space aligned to world axes, so the world
normal is directly usable as a tangent-space normal map.
"""

import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bl_kit as K

OUT = os.path.join(K.ART_ROOT, "Textures")
RES = 1024


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def emission_vertex_color(name):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    attr = nt.nodes.new("ShaderNodeVertexColor")
    attr.layer_name = "Tint"
    emit = nt.nodes.new("ShaderNodeEmission")
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(attr.outputs["Color"], emit.inputs["Color"])
    nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])
    return mat


def emission_normal(name):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    geo = nt.nodes.new("ShaderNodeNewGeometry")
    mul = nt.nodes.new("ShaderNodeVectorMath")
    mul.operation = "MULTIPLY"
    mul.inputs[1].default_value = (0.5, 0.5, 0.5)
    add = nt.nodes.new("ShaderNodeVectorMath")
    add.operation = "ADD"
    add.inputs[1].default_value = (0.5, 0.5, 0.5)
    emit = nt.nodes.new("ShaderNodeEmission")
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(geo.outputs["Normal"], mul.inputs[0])
    nt.links.new(mul.outputs["Vector"], add.inputs[0])
    nt.links.new(add.outputs["Vector"], emit.inputs["Color"])
    nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])
    return mat


def shape_mesh(name, outline, dome, tint_fn, rows=48, cols=24):
    """Build a flat shape filling 0..1 in XY.

    `outline(v)` returns half-width at length fraction v; `dome(u, v)` the Z
    bulge that gives the normal map its shape.
    """
    bm = bmesh.new()
    col = bm.loops.layers.float_color.new("Tint")
    grid = []
    for i in range(rows + 1):
        v = i / rows
        hw = max(outline(v), 1e-5)
        row = []
        for j in range(cols + 1):
            s = j / cols              # 0..1 across the width
            x = 0.5 + (s - 0.5) * 2.0 * hw
            row.append(bm.verts.new((x, v, dome(s, v))))
        grid.append(row)

    for i in range(rows):
        for j in range(cols):
            try:
                f = bm.faces.new((grid[i][j], grid[i][j + 1],
                                  grid[i + 1][j + 1], grid[i + 1][j]))
            except ValueError:
                continue
            for loop, (s, v) in zip(f.loops,
                                    ((j / cols, i / rows),
                                     ((j + 1) / cols, i / rows),
                                     ((j + 1) / cols, (i + 1) / rows),
                                     (j / cols, (i + 1) / rows))):
                loop[col] = tint_fn(s, v)

    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    for p in mesh.polygons:
        p.use_smooth = True
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def render(obj, mat, path, transparent):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = RES
    scene.render.resolution_y = RES
    scene.render.film_transparent = transparent
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA" if transparent else "RGB"
    scene.view_settings.view_transform = "Standard"  # no filmic on data maps

    obj.data.materials.clear()
    obj.data.materials.append(mat)

    cam_data = bpy.data.cameras.new("C")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = 1.0
    cam = bpy.data.objects.new("C", cam_data)
    scene.collection.objects.link(cam)
    cam.location = (0.5, 0.5, 4.0)
    cam.rotation_euler = (0.0, 0.0, 0.0)   # straight down -Z over the unit square
    scene.camera = cam

    os.makedirs(os.path.dirname(path), exist_ok=True)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)


# ------------------------------------------------------------------- shapes
def leaf_tint(s, v):
    edge = abs(s - 0.5) * 2.0
    rib = 1.0 - min(abs(s - 0.5) * 14.0, 1.0)
    g = 0.50 - 0.16 * edge + 0.10 * (1.0 - v)
    return (0.14 + 0.10 * rib, g, 0.11 + 0.05 * rib, 1.0)


def grass_tint(s, v):
    edge = abs(s - 0.5) * 2.0
    return (0.26 + 0.10 * v, 0.56 - 0.12 * edge + 0.10 * v, 0.16, 1.0)


def petal_tint(s, v):
    edge = abs(s - 0.5) * 2.0
    return (0.90, 0.22 + 0.42 * (1.0 - v) * (1.0 - edge), 0.26 + 0.10 * v, 1.0)


def taper(v, a, b, amp):
    """Leaf-style profile: peaks at a/(a+b), draws to a point at both ends."""
    v = min(max(v, 1e-4), 1.0 - 1e-4)
    peak = a / (a + b)
    norm = (peak ** a) * ((1.0 - peak) ** b)
    return amp * (v ** a) * ((1.0 - v) ** b) / norm


def veins(s, v, n=7):
    """Faint lateral vein lines angled toward the tip."""
    return 1.0 - min(abs(math.sin((v * n - abs(s - 0.5) * 2.2) * math.pi)) ** 18, 1.0)


def leaf_tint2(s, v):
    edge = abs(s - 0.5) * 2.0
    rib = 1.0 - min(abs(s - 0.5) * 16.0, 1.0)
    vn = veins(s, v) * 0.5
    g = 0.52 - 0.18 * edge + 0.08 * (1.0 - v) - 0.07 * vn
    return (0.13 + 0.09 * rib, g, 0.10 + 0.05 * rib, 1.0)


SHAPES = {
    # palm/plant leaflet: widest around a third up, drawn to a sharp tip
    "LeafCutout": dict(
        outline=lambda v: taper(v, 0.55, 1.00, 0.30),
        dome=lambda s, v: 0.13 * math.sin(min(v, 1.0) * math.pi) * (1.0 - abs(s - 0.5) * 2.0) ** 1.4,
        tint=leaf_tint2),
    # grass blade: narrow, near-parallel sides, taper held until late
    "GrassBladeCutout": dict(
        outline=lambda v: taper(v, 0.22, 0.85, 0.115),
        dome=lambda s, v: 0.10 * (1.0 - v) * (1.0 - abs(s - 0.5) * 2.0) ** 1.2,
        tint=grass_tint),
    # petal: broad and blunt, widest past halfway
    "FlowerPetalCutout": dict(
        outline=lambda v: taper(v, 0.95, 0.55, 0.40),
        dome=lambda s, v: 0.16 * math.sin(min(v, 1.0) * math.pi) * (1.0 - abs(s - 0.5) * 2.0) ** 1.1,
        tint=petal_tint),
}


def main():
    report = {}
    for name, spec in SHAPES.items():
        reset()
        obj = shape_mesh(name, spec["outline"], spec["dome"], spec["tint"])
        base = "%s/T_%s_BaseColor.png" % (OUT, name)
        nrm = "%s/T_%s_Normal.png" % (OUT, name)
        render(obj, emission_vertex_color("M_" + name + "_C"), base, True)
        render(obj, emission_normal("M_" + name + "_N"), nrm, False)
        report[name] = {
            "BaseColor": os.path.exists(base),
            "Normal": os.path.exists(nrm),
            "tris": sum(len(p.vertices) - 2 for p in obj.data.polygons),
        }
    print("CUTOUT_REPORT", report)


main()
