"""Material body. Prepended with sdlib.py and `TARGET = "<name>"`.

One material per execute_code call: the plugin's main-thread dispatcher has a
120s cap and a full set (build + 1k compute + 3 saves) runs close to it.
"""

OUTDIR = "C:/Users/ethan/Documents/GitHub/Memory-Palace/Art/Textures"

SPECS = {
    "PalmBark": {
        # vertical fibre grain is what makes bark read as bark
        "noises": [("fibers_2", 1.0), ("clouds_2", 0.42), ("perlin_noise_1", 0.26)],
        "colors": [(0.16, 0.10, 0.06), (0.47, 0.38, 0.29)],
        "contrast": (0.16, 0.88), "normal": 4.0, "rough": (0.62, 0.96),
    },
    "WeatheredWood": {
        "noises": [("wood_fibers_1", 1.0), ("scratches_2", 0.30), ("clouds_2", 0.28)],
        "colors": [(0.26, 0.23, 0.19), (0.66, 0.63, 0.56)],
        "contrast": (0.20, 0.88), "normal": 2.6, "rough": (0.58, 0.94),
    },
    "Stone": {
        "noises": [("clouds_2", 1.0), ("cells_1", 0.40), ("dirt_1", 0.26)],
        "colors": [(0.21, 0.21, 0.23), (0.64, 0.64, 0.65)],
        "contrast": (0.22, 0.84), "normal": 3.2, "rough": (0.52, 0.90),
    },
    "Moss": {
        "noises": [("bnw_spots_1", 1.0), ("clouds_1", 0.48), ("perlin_noise_1", 0.32)],
        "colors": [(0.05, 0.15, 0.04), (0.30, 0.54, 0.18)],
        "contrast": (0.24, 0.90), "normal": 2.2, "rough": (0.80, 1.0),
    },
    "OrganicDetail": {
        "noises": [("gaussian_spots_1", 1.0), ("clouds_1", 0.38), ("perlin_noise_1", 0.20)],
        "colors": [(0.50, 0.43, 0.36), (0.94, 0.90, 0.81)],
        "contrast": (0.28, 0.82), "normal": 1.8, "rough": (0.34, 0.72),
    },
}


def build(name, spec):
    g = Graph("T_" + name)

    # ---- height: stack of library noises, lerped together
    first, _ = spec["noises"][0]
    h = g.inst(first)
    for ident, w in spec["noises"][1:]:
        h = g.mix(g.inst(ident), h, w)

    lvl = g.atomic("levels",
                   levelinlow=spec["contrast"][0],
                   levelinhigh=spec["contrast"][1])
    connect(h, lvl, "input1")

    # feed `levels` straight on: grayscaleconversion's default channel
    # weights are zero, which blanked every downstream mask
    hgt = lvl

    # ---- normal
    nrm = g.atomic("normal", intensity=spec["normal"])
    connect(hgt, nrm, "input1")

    # ---- base colour: two uniforms lerped through the height mask
    ca, cb = spec["colors"]
    ua = g.atomic("uniform", colorswitch=True,
                  outputcolor=(ca[0], ca[1], ca[2], 1.0))
    ub = g.atomic("uniform", colorswitch=True,
                  outputcolor=(cb[0], cb[1], cb[2], 1.0))
    base = g.atomic("blend", opacitymult=1.0)
    connect(ub, base, "source")
    connect(ua, base, "destination")
    connect(hgt, base, "opacity")

    # ---- roughness / AO / metallic
    r0, r1 = spec["rough"]
    rough = g.atomic("levels", leveloutlow=r0, levelouthigh=r1)
    connect(hgt, rough, "input1")

    blur = g.atomic("blur", intensity=8.0)
    connect(hgt, blur, "input1")
    ao = g.atomic("levels", leveloutlow=0.52, levelouthigh=1.0)
    connect(blur, ao, "input1")

    metal = g.atomic("uniform", colorswitch=False, outputcolor=0.0)

    # ---- ORM pack (R=AO, G=Roughness, B=Metallic)
    orm = g.inst("rgba_merge")
    connect(ao, orm, "R")
    connect(rough, orm, "G")
    connect(metal, orm, "B")

    for n in (base, nrm, orm):
        g.out(n)
    g.set_size(10, fmt=0)
    g.compute()

    written = {}
    for suffix, node in (("BaseColor", base), ("Normal", nrm), ("ORM", orm)):
        fp = "%s/T_%s_%s.png" % (OUTDIR, name, suffix)
        written[suffix] = save_node(node, fp)
    return written


log_reset()
log("TARGET=%s" % TARGET)
try:
    res = build(TARGET, SPECS[TARGET])
    log("RESULT %s %s" % (TARGET, res))
except Exception:
    log("EXC " + traceback.format_exc()[-1400:])
log("DONE")
