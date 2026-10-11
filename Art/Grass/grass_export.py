"""Export the grass card LODs to FBX for Unreal, and save a .blend source copy."""

import json
import os

import bpy

OUT_DIR = r"C:\Users\ethan\Documents\GitHub\Memory-Palace\Art\Grass"
NAMES = ["SM_GrassClump_LOD0", "SM_GrassClump_LOD1", "SM_GrassClump_LOD2"]

os.makedirs(OUT_DIR, exist_ok=True)


def select_only(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


report = {"exported": [], "out_dir": OUT_DIR}

for name in NAMES:
    obj = bpy.data.objects.get(name)
    if obj is None:
        report["exported"].append({"name": name, "error": "object missing"})
        continue

    select_only(obj)
    path = os.path.join(OUT_DIR, name + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={"MESH"},
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_triangles=False,
        bake_space_transform=False,
        add_leaf_bones=False,
        apply_scale_options="FBX_SCALE_NONE",
        global_scale=1.0,
        axis_forward="-Z",
        axis_up="Y",
        path_mode="AUTO",
    )
    report["exported"].append({
        "name": name,
        "path": path,
        "bytes": os.path.getsize(path) if os.path.exists(path) else 0,
        "uv_layers": [l.name for l in obj.data.uv_layers],
        "material_slots": [s.name for s in obj.material_slots],
    })

# a standalone source file, saved as a copy so the open session keeps its own path
blend_path = os.path.join(OUT_DIR, "GrassCards_Source.blend")
bpy.ops.wm.save_as_mainfile(filepath=blend_path, copy=True)
report["blend"] = blend_path
report["blend_bytes"] = os.path.getsize(blend_path) if os.path.exists(blend_path) else 0

print("EXPORT_REPORT_BEGIN")
print(json.dumps(report, indent=1))
print("EXPORT_REPORT_END")
