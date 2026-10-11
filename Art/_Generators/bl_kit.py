"""Shared helpers for generating the Memory Palace art kits in background Blender.

Export contract agreed with the Unreal side: FBX, Y-up / -Z forward, scale 1,
transforms applied, one material slot per mesh.
"""

import math
import os

import bmesh
import bpy
from mathutils import Vector

ART_ROOT = r"C:\Users\ethan\Documents\GitHub\Memory-Palace\Art"


# ------------------------------------------------------------------ scene
def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0


def material(name, rgba):
    mat = bpy.data.materials.get(name)
    if mat is None:
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        if bsdf is not None:
            bsdf.inputs["Base Color"].default_value = rgba
            bsdf.inputs["Roughness"].default_value = 0.85
        # Workbench MATERIAL shading reads diffuse_color, not the BSDF input
        mat.diffuse_color = rgba
    return mat


# ------------------------------------------------------------------- mesh
def object_from_bm(name, bm, mats, smooth=True):
    """`mats` is a single material or an ordered list matching material_index."""
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    for poly in mesh.polygons:
        poly.use_smooth = smooth
    for mat in (mats if isinstance(mats, (list, tuple)) else [mats]):
        mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def drop_to_floor(obj):
    """Sit the mesh on z=0 so Unreal places it by its base."""
    lowest = min(v.co.z for v in obj.data.vertices)
    for v in obj.data.vertices:
        v.co.z -= lowest
    obj.data.update()
    return obj


def noisify(obj, amp, freq=1.6, seed=0, octaves=3, axis_bias=None):
    """Push vertices along their normals with layered sine noise.

    Cheap stand-in for a displacement texture: enough to break up a silhouette
    without the hard facets that a low-poly displace modifier leaves behind.
    """
    import math
    import random
    rng = random.Random(seed)
    mesh = obj.data
    layers = [(rng.uniform(0, math.tau), rng.uniform(0, math.tau),
               rng.uniform(0, math.tau), freq * (2.0 ** i), amp / (1.9 ** i))
              for i in range(octaves)]
    for v in mesh.vertices:
        d = 0.0
        for px, py, pz, sc, a in layers:
            d += a * (math.sin(v.co.x * sc + px)
                      * math.sin(v.co.y * sc + py)
                      * math.sin(v.co.z * sc + pz))
        n = v.normal.copy()
        if axis_bias is not None:
            n = (n + axis_bias).normalized()
        v.co += n * d
    mesh.update()
    return obj


def scale_to_height(obj, target_z):
    """Uniformly scale so total height matches the brief.

    Fronds and leaves push the silhouette above the trunk, so construction
    height is never the finished height -- normalise at the end instead.
    """
    zs = [v.co.z for v in obj.data.vertices]
    cur = max(max(zs) - min(zs), 1e-6)
    s = target_z / cur
    for v in obj.data.vertices:
        v.co *= s
    obj.data.update()
    return s


def scale_to_width(obj, target_x):
    xs = [v.co.x for v in obj.data.vertices]
    cur = max(max(xs) - min(xs), 1e-6)
    s = target_x / cur
    for v in obj.data.vertices:
        v.co *= s
    obj.data.update()
    return s


def recalc_outside(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def arrange_row(objs, gap=1.4):
    """Lay objects out along X with a gap, for contact-sheet renders."""
    x = 0.0
    for obj in objs:
        w = max(obj.dimensions.x, 0.2)
        obj.location.x = x + w * 0.5
        x += w * gap
    return objs


def bevel(bm, amount, segments=1, only_sharp=False):
    edges = [e for e in bm.edges
             if not only_sharp or e.calc_face_angle(0.0) > math.radians(25.0)]
    if not edges:
        return
    bmesh.ops.bevel(bm, geom=edges, offset=amount, segments=segments,
                    profile=0.5, affect="EDGES", clamp_overlap=True)


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# -------------------------------------------------------------- attributes
def vertical_gradient(obj, name="VertGradient", lo=None, hi=None, invert=False):
    """Bake a 0-1 vertical ramp into a corner colour layer.

    Vertex colours survive FBX without the V-flip that affects UV channels,
    so this is the safe place for data Unreal needs to read back.
    """
    mesh = obj.data
    zs = [v.co.z for v in mesh.vertices]
    lo = min(zs) if lo is None else lo
    hi = max(zs) if hi is None else hi
    span = max(hi - lo, 1e-6)

    layer = mesh.color_attributes.new(name=name, type="FLOAT_COLOR",
                                      domain="CORNER")
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            vi = mesh.loops[li].vertex_index
            t = min(max((mesh.vertices[vi].co.z - lo) / span, 0.0), 1.0)
            if invert:
                t = 1.0 - t
            layer.data[li].color = (t, t, t, 1.0)
    return layer


def axis_wind_mask(obj, path, name="WindMask", power=0.85):
    """0 on the trunk, 1 at the leaf tips, from distance to the trunk centreline.

    A plain Z ramp is wrong for palms because fronds droop below the crown, so
    the mask is built from radial distance to the stem instead.
    """
    mesh = obj.data
    dists = []
    for v in mesh.vertices:
        dists.append(min((v.co - p).length for p in path))
    hi = max(dists) or 1.0

    layer = mesh.color_attributes.new(name=name, type="FLOAT_COLOR",
                                      domain="CORNER")
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            t = min(max(dists[mesh.loops[li].vertex_index] / hi, 0.0), 1.0) ** power
            layer.data[li].color = (t, t, t, 1.0)
    return layer


def spherical_uv(obj, center=None, name="UVMap"):
    """Cheap spherical projection. Adequate for blobby shapes with flat tints."""
    mesh = obj.data
    if center is None:
        center = sum((v.co for v in mesh.vertices), Vector()) / len(mesh.vertices)
    uv = mesh.uv_layers.get(name) or mesh.uv_layers.new(name=name)
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            d = (mesh.vertices[mesh.loops[li].vertex_index].co - center)
            r = max(d.length, 1e-6)
            uv.data[li].uv = (
                math.atan2(d.y, d.x) / math.tau + 0.5,
                1.0 - math.acos(max(-1.0, min(1.0, d.z / r))) / math.pi,
            )
    return uv


def box_uv(obj, scale=1.0, name="UVMap"):
    """Triplanar-style box projection, so tiling textures land square."""
    mesh = obj.data
    uv = mesh.uv_layers.get(name) or mesh.uv_layers.new(name=name)
    for poly in mesh.polygons:
        n = poly.normal
        ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            if az >= ax and az >= ay:
                u, v = co.x, co.y
            elif ax >= ay:
                u, v = co.y, co.z
            else:
                u, v = co.x, co.z
            uv.data[li].uv = (u * scale, v * scale)
    return uv


# ----------------------------------------------------------------- export
def export_fbx(obj, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={"MESH"},
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_triangles=False,
        colors_type="SRGB",
        bake_space_transform=False,
        add_leaf_bones=False,
        apply_scale_options="FBX_SCALE_NONE",
        global_scale=1.0,
        axis_forward="-Z",
        axis_up="Y",
        path_mode="AUTO",
    )
    return {"path": path, "bytes": os.path.getsize(path)}


def save_blend(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=path, copy=True)
    return path


# ---------------------------------------------------------------- preview
def render_preview(path, objects, azimuth=38.0, elevation=22.0, margin=1.35):
    """Workbench render of the given objects, framed automatically."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 640
    scene.render.film_transparent = False
    shading = scene.display.shading
    shading.light = "STUDIO"
    shading.color_type = "MATERIAL"
    shading.show_shadows = True
    shading.show_cavity = True
    # without this a white cloud on the default light backdrop renders invisible
    shading.background_type = "VIEWPORT"
    shading.background_color = (0.17, 0.19, 0.23)
    shading.show_object_outline = False

    bpy.context.view_layer.update()  # matrix_world is stale right after moving objects
    lo = Vector((1e9, 1e9, 1e9))
    hi = Vector((-1e9, -1e9, -1e9))
    for obj in objects:
        for corner in obj.bound_box:
            w = obj.matrix_world @ Vector(corner)
            lo = Vector((min(lo[i], w[i]) for i in range(3)))
            hi = Vector((max(hi[i], w[i]) for i in range(3)))
    center = (lo + hi) * 0.5
    radius = max((hi - lo).length * 0.5, 0.2)

    cam_data = bpy.data.cameras.new("PreviewCam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = radius * 2.0 * margin
    cam = bpy.data.objects.new("PreviewCam", cam_data)
    bpy.context.scene.collection.objects.link(cam)

    az, el = math.radians(azimuth), math.radians(elevation)
    direction = Vector((math.cos(el) * math.sin(az),
                        -math.cos(el) * math.cos(az),
                        math.sin(el)))
    cam.location = center + direction * (radius * 4.0)
    # a camera looks down its local -Z, so local +Z must point back along the
    # offset -- tracking to -direction aims it at empty space
    cam.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    scene.camera = cam
    print("PREVIEW cam=%s ortho=%.2f center=%s radius=%.2f"
          % ([round(c, 2) for c in cam.location], cam_data.ortho_scale,
             [round(c, 2) for c in center], radius))

    os.makedirs(os.path.dirname(path), exist_ok=True)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    return path
