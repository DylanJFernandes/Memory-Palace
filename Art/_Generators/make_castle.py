"""Gothic memory palace for precalculus.

The plan is the curriculum. Walking east through the building walks forward
through the course:

  west rose window   the unit circle, seen on entry and from the whole nave
  nave bays 1-8      function families, in teaching order
  crossing           the coordinate plane / vectors (the origin, literally)
  north transept     trigonometry
  south transept     complex numbers and polar form
  choir bays 9-11    identities, inverse trig, matrices
  apse (5 facets)    the conic sections, radiating from one focus

Each window is exported as its own object named for its locus, so the game can
treat it as an interaction anchor.
"""

import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bl_kit as K
import bl_gothic as G

OUT = os.path.join(K.ART_ROOT, "Castle")

# ----------------------------------------------------------------- dimensions
BAY = 6.0            # bay length along the nave axis
NAVE_HW = 6.0        # half width of the central vessel (pier line at y = +-6)
AISLE = 5.0          # aisle depth, outer wall at y = +-11
AISLE_Y = NAVE_HW + AISLE
WALL_T = 0.8

PIER_H = 9.0         # arcade springing
AISLE_WALL_H = 15.0
AISLE_VAULT_RISE = 4.5
CLEAR_Z0 = 17.5      # clerestory sill: must clear the aisle lean-to roof
CLEAR_H = 8.0
NAVE_SPRING = CLEAR_Z0 + CLEAR_H
NAVE_RISE = 5.0

NAVE_BAYS = 8
CHOIR_BAYS = 3
CROSS_LEN = 12.0
TRANSEPT_Y = 22.0
APSE_R = 9.0
APSE_FACETS = 5

NAVE_X0 = 0.0
NAVE_X1 = NAVE_X0 + NAVE_BAYS * BAY          # 48
CROSS_X1 = NAVE_X1 + CROSS_LEN               # 60
CHOIR_X1 = CROSS_X1 + CHOIR_BAYS * BAY       # 78

TOWER_W = 7.0
TOWER_H = 34.0
SPIRE_H = 14.0

# ---------------------------------------------------------------------- loci
NAVE_LOCI = [
    "01_Functions", "02_DomainRange", "03_Transformations", "04_Polynomials",
    "05_RationalAsymptotes", "06_Exponentials", "07_Logarithms",
    "08_SequencesSeries",
]
CHOIR_LOCI = ["09_TrigIdentities", "10_InverseTrig", "11_Matrices"]
APSE_LOCI = ["12_Circle", "13_Ellipse", "14_Parabola", "15_Hyperbola",
             "16_Degenerate"]

MATS = [
    ("M_CastleStone", (0.52, 0.50, 0.46, 1.0)),
    ("M_CastleFloor", (0.34, 0.32, 0.31, 1.0)),
    ("M_CastleRoof", (0.20, 0.22, 0.26, 1.0)),
]
GLASS = ("M_CastleGlass", (0.18, 0.34, 0.62, 1.0))

STONE, FLOOR, ROOF = 0, 1, 2


class Build(object):
    """Collects geometry into one bmesh per exported object."""

    def __init__(self):
        self.parts = {}
        self.glass = []       # (name, bmesh)

    def bm(self, key):
        if key not in self.parts:
            self.parts[key] = bmesh.new()
        return self.parts[key]

    def new_glass(self, name):
        bm = bmesh.new()
        self.glass.append((name, bm))
        return bm


def yrot(deg):
    return Matrix.Rotation(math.radians(deg), 4, "Z")


# ------------------------------------------------------------------ elements
def wall_bay(b, key, x_centre, y, height, opening, glass_name=None,
             flip=False, thickness=WALL_T, z0=0.0):
    """One bay of wall in the XZ plane at a given y, with one arched opening."""
    outer = G.rect_outline(BAY, height, centre=(0.0, z0 + height * 0.5))
    mx = Matrix.Translation(Vector((x_centre, y, 0.0)))
    if flip:
        mx = mx @ yrot(180)
    G.panel(b.bm(key), outer, opening, thickness, mat=STONE, n=72, matrix=mx)
    if glass_name:
        gb = b.new_glass(glass_name)
        G.fill_poly(gb, opening, mat=0, matrix=mx @ Matrix.Translation(
            Vector((0.0, thickness * 0.5, 0.0))))


def cross_wall(b, key, x, y_centre, width, height, opening, glass_name=None,
               thickness=WALL_T):
    """Wall in the YZ plane at a given x (transept sides, facade, apse)."""
    outer = G.rect_outline(width, height, centre=(0.0, height * 0.5))
    mx = Matrix.Translation(Vector((x, y_centre, 0.0))) @ yrot(-90)
    G.panel(b.bm(key), outer, opening, thickness, mat=STONE, n=72, matrix=mx)
    if glass_name:
        gb = b.new_glass(glass_name)
        G.fill_poly(gb, opening, mat=0, matrix=mx @ Matrix.Translation(
            Vector((0.0, thickness * 0.5, 0.0))))


ARCADE_SPAN = 4.3

def archivolt_only(b, x0, y, sign):
    """Arch moulding without a pier, where a wall already closes that line."""
    path = G.arch_path(ARCADE_SPAN + 0.6, 1.0, PIER_H, Vector((1, 0, 0)),
                       Vector((0, 1, 0)),
                       (x0 + BAY * 0.5, y - sign * 0.25, 0.0), n=20)
    G.sweep_rect(b.bm("Arches"), path, Vector((0, 1, 0)), 0.9, 0.5, mat=STONE)




def arcade_bay(b, x0, y, sign):
    """Pier, plus an archivolt standing proud of the arcade wall.

    The wall itself (with the arched opening) is built in build_nave; this is
    the moulding around it. It is offset toward the nave and given a slightly
    wider span so none of its faces are coplanar with the opening reveal,
    which would z-fight.
    """
    G.pier(b.bm("Piers"), (x0, y), 0.85, PIER_H, shafts=8, mat=STONE)
    path = G.arch_path(ARCADE_SPAN + 0.6, 1.0, PIER_H, Vector((1, 0, 0)),
                       Vector((0, 1, 0)),
                       (x0 + BAY * 0.5, y - sign * 0.25, 0.0), n=20)
    G.sweep_rect(b.bm("Arches"), path, Vector((0, 1, 0)), 0.9, 0.5, mat=STONE)


def buttress(b, x, y, depth, height, sign):
    """Stepped buttress standing off the aisle wall."""
    bm = b.bm("Buttresses")
    y0 = y + sign * (WALL_T * 0.5)
    G.box(bm, (x - 0.9, min(y0, y0 + sign * depth), 0.0),
          (x + 0.9, max(y0, y0 + sign * depth), height * 0.62), mat=STONE)
    G.box(bm, (x - 0.7, min(y0, y0 + sign * depth * 0.65), height * 0.62),
          (x + 0.7, max(y0, y0 + sign * depth * 0.65), height), mat=STONE)
    G.spire(bm, (x, y0 + sign * depth * 0.32), 0.95, height, 3.0, sides=4,
            mat=ROOF)


def flying_buttress(b, x, sign):
    """Arc carrying nave thrust out to the aisle buttress."""
    inner = Vector((x, sign * (NAVE_HW + 0.4), CLEAR_Z0 + 1.0))
    outer = Vector((x, sign * (AISLE_Y + 1.8), AISLE_WALL_H * 0.72))
    pts = []
    for i in range(13):
        t = i / 12.0
        p = inner.lerp(outer, t)
        p.z += 2.2 * math.sin(t * math.pi) + 1.4 * t
        pts.append(p)
    G.sweep_rect(b.bm("Buttresses"), pts, Vector((1, 0, 0)), 0.7, 1.1,
                 mat=STONE)


def vault_bay(b, cx, cy, sx, sy, spring, rise):
    G.groin_vault(b.bm("Vaults"), (cx, cy), sx, sy, spring, rise, point=1.0,
                  res=12, mat=STONE)



def doorway_bay(b, key, x_centre, y, height, opening, flip=False,
                thickness=WALL_T, width=BAY):
    """Wall in the XZ plane whose opening reaches the floor."""
    outline = G.doorway_outline(width, height, opening)
    mx = Matrix.Translation(Vector((x_centre, y, 0.0)))
    if flip:
        mx = mx @ yrot(180)
    G.solid_panel(b.bm(key), outline, thickness, mat=STONE, matrix=mx)


def doorway_cross(b, key, x, y_centre, width, height, opening,
                  thickness=WALL_T):
    """Wall in the YZ plane whose opening reaches the floor."""
    outline = G.doorway_outline(width, height, opening)
    mx = Matrix.Translation(Vector((x, y_centre, 0.0))) @ yrot(-90)
    G.solid_panel(b.bm(key), outline, thickness, mat=STONE, matrix=mx)


def solid_bay(b, key, x, y_centre, width, height, thickness=WALL_T):
    outline = G.rect_outline(width, height, centre=(0.0, height * 0.5))
    mx = Matrix.Translation(Vector((x, y_centre, 0.0))) @ yrot(-90)
    G.solid_panel(b.bm(key), outline, thickness, mat=STONE, matrix=mx)


def build_crossing_walls(b):
    """The four crossing arches, and the aisle ends that meet the transept.

    Nothing existed at y = +-NAVE_HW between x = NAVE_X1 and CROSS_X1, so the
    nave looked straight into the transept roof void: the large black gap.
    """
    cx = (NAVE_X1 + CROSS_X1) * 0.5
    for sign in (-1.0, 1.0):
        # great crossing arch, nave vessel into transept arm
        doorway_bay(b, "Transept", cx, sign * NAVE_HW, CLEAR_Z0,
                    G.arch_outline(7.0, 0.0, PIER_H + 1.0, point=1.0),
                    flip=(sign < 0), width=CROSS_LEN)
        # solid wall above it, up to the eave
        outline = G.rect_outline(CROSS_LEN, CLEAR_H,
                                 centre=(0.0, CLEAR_Z0 + CLEAR_H * 0.5))
        mx = Matrix.Translation(Vector((cx, sign * NAVE_HW, 0.0)))
        if sign < 0:
            mx = mx @ yrot(180)
        G.solid_panel(b.bm("Transept"), outline, WALL_T, mat=STONE, matrix=mx)

        # aisle ends opening into the transept
        eave = CLEAR_Z0 + CLEAR_H
        for ex in (NAVE_X1, CROSS_X1):
            doorway_cross(b, "Transept", ex, sign * (AISLE_Y + NAVE_HW) * 0.5,
                          AISLE, eave,
                          G.arch_outline(3.0, 0.0, PIER_H - 1.0, point=1.0))
        # choir aisle east end, closed. Widened past the aisle so it overlaps
        # both the apse chord edge and the aisle outer wall rather than
        # meeting them on a hairline.
        solid_bay(b, "Transept", CHOIR_X1, sign * (AISLE_Y + NAVE_HW) * 0.5,
                  AISLE + 2.0, eave)


# ----------------------------------------------------------------- assemblies
def build_nave(b, x0, bays, loci, first_pier=True, closing_pier=True):
    for i in range(bays):
        bx = x0 + i * BAY
        name = loci[i] if i < len(loci) else "bay%d" % i
        for sign in (-1.0, 1.0):
            y = sign * AISLE_Y
            side = "N" if sign > 0 else "S"
            wall_bay(b, "Walls", bx + BAY * 0.5, y, AISLE_WALL_H,
                     G.arch_outline(2.8, 3.2, 4.2, point=1.0),
                     glass_name="SM_Glass_%s_%s" % (name, side),
                     flip=(sign < 0))
            # clerestory above the arcade
            wall_bay(b, "Walls", bx + BAY * 0.5, sign * NAVE_HW, CLEAR_H,
                     G.arch_outline(3.0, CLEAR_Z0 + 1.0, 3.4, point=1.0),
                     glass_name="SM_Glass_%s_%s_Clerestory" % (name, side),
                     flip=(sign < 0), z0=CLEAR_Z0)
            # arcade wall up to the clerestory sill, pierced by the arch.
            # The opening reaches the floor, so this cannot use panel().
            doorway_bay(b, "Walls", bx + BAY * 0.5, sign * NAVE_HW, CLEAR_Z0,
                        G.arch_outline(ARCADE_SPAN, 0.0, PIER_H, point=1.0),
                        flip=(sign < 0))
            if i > 0 or first_pier:
                arcade_bay(b, bx, sign * NAVE_HW, sign)
            else:
                # pier omitted: the facade / crossing pier occupies this line
                archivolt_only(b, bx, sign * NAVE_HW, sign)
            buttress(b, bx, y, 2.4, AISLE_WALL_H * 0.95, sign)
            flying_buttress(b, bx + BAY * 0.5, sign)
            vault_bay(b, bx + BAY * 0.5, sign * (NAVE_HW + AISLE * 0.5),
                      BAY, AISLE, PIER_H, AISLE_VAULT_RISE)
        vault_bay(b, bx + BAY * 0.5, 0.0, BAY, NAVE_HW * 2.0,
                  NAVE_SPRING, NAVE_RISE)
    if closing_pier:
        for sign in (-1.0, 1.0):
            G.pier(b.bm("Piers"), (x0 + bays * BAY, sign * NAVE_HW), 0.85,
                   PIER_H, mat=STONE)


def build_facade(b):
    """West front: portal below, rose window above, flanking towers."""
    width = AISLE_Y * 2.0
    # lower stage with the great portal
    portal = G.arch_outline(5.0, 0.0, 4.5, point=1.0)
    doorway_cross(b, "Facade", 0.0, 0.0, width, 13.0, portal)
    # Upper stage carrying the rose. Its silhouette follows the actual
    # section of the building -- aisle roofs, clerestory, nave gable -- so the
    # west front reads as a gable rather than a blank slab hiding the roof.
    eave = CLEAR_Z0 + CLEAR_H
    ridge = NAVE_SPRING + NAVE_RISE + 5.0
    z0 = 13.0
    outer = [
        Vector((-AISLE_Y, z0)), Vector((-AISLE_Y, AISLE_WALL_H)),
        Vector((-NAVE_HW, CLEAR_Z0)), Vector((-NAVE_HW, eave)),
        Vector((0.0, ridge)),
        Vector((NAVE_HW, eave)), Vector((NAVE_HW, CLEAR_Z0)),
        Vector((AISLE_Y, AISLE_WALL_H)), Vector((AISLE_Y, z0)),
    ]
    rose_c = (0.0, 21.5)
    rose = G.circle_outline(4.2, centre=rose_c, n=72)
    mx = Matrix.Translation(Vector((0.0, 0.0, 0.0))) @ yrot(-90)
    G.panel(b.bm("Facade"), outer, rose, WALL_T, mat=STONE, n=96, matrix=mx,
            centre=rose_c)
    gb = b.new_glass("SM_Glass_00_RoseWindow_UnitCircle")
    G.fill_poly(gb, rose, mat=0,
                matrix=mx @ Matrix.Translation(Vector((0.0, WALL_T * 0.5, 0.0))))
    # rose tracery: 12 spokes, one per clock position on the unit circle
    tb = b.bm("Facade")
    for i in range(12):
        a = i / 12.0 * math.tau
        p0 = Vector((0.0, rose_c[0] + math.cos(a) * 0.6,
                     rose_c[1] + math.sin(a) * 0.6))
        p1 = Vector((0.0, rose_c[0] + math.cos(a) * 4.1,
                     rose_c[1] + math.sin(a) * 4.1))
        G.sweep_rect(tb, [p0, p1], Vector((1, 0, 0)), 0.22, 0.5, mat=STONE)

    for sign in (-1.0, 1.0):
        ty = sign * (AISLE_Y + TOWER_W * 0.5)   # abut the facade exactly
        tb = b.bm("Towers")
        G.box(tb, (-1.0, ty - TOWER_W * 0.5, 0.0),
              (TOWER_W - 1.0, ty + TOWER_W * 0.5, TOWER_H), mat=STONE)
        for lvl in (12.0, 20.0, 28.0):
            cross_wall(b, "Towers", -1.05, ty, TOWER_W * 0.8, 6.0,
                       G.arch_outline(1.8, lvl + 0.8, 2.4, point=1.0),
                       thickness=0.5)
        G.spire(tb, (TOWER_W * 0.5 - 1.0, ty), TOWER_W * 0.72, TOWER_H,
                SPIRE_H, sides=8, mat=ROOF)


def build_transept(b):
    cx = (NAVE_X1 + CROSS_X1) * 0.5
    for sign in (-1.0, 1.0):
        arm = "North" if sign > 0 else "South"
        locus = "Trigonometry" if sign > 0 else "ComplexPolar"
        # end wall with a great window
        cross_wall_y = sign * TRANSEPT_Y
        outer = G.rect_outline(CROSS_LEN, 26.0, centre=(0.0, 13.0))
        opening = G.arch_outline(6.0, 6.0, 7.0, point=1.0)
        mx = Matrix.Translation(Vector((cx, cross_wall_y, 0.0)))
        if sign < 0:
            mx = mx @ yrot(180)
        G.panel(b.bm("Transept"), outer, opening, WALL_T, mat=STONE, n=72,
                matrix=mx)
        gb = b.new_glass("SM_Glass_%s_%s" % (arm, locus))
        G.fill_poly(gb, opening, mat=0,
                    matrix=mx @ Matrix.Translation(Vector((0, WALL_T * 0.5, 0))))

        # side walls of the arm, from the aisle line out to the end
        span = TRANSEPT_Y - AISLE_Y
        for ex, tag in ((NAVE_X1, "W"), (CROSS_X1, "E")):
            outer = G.rect_outline(span, NAVE_SPRING,
                                   centre=(0.0, NAVE_SPRING * 0.5))
            op = G.arch_outline(3.0, 4.0, 5.0, point=1.0)
            mxx = (Matrix.Translation(
                Vector((ex, sign * (AISLE_Y + span * 0.5), 0.0))) @ yrot(-90))
            G.panel(b.bm("Transept"), outer, op, WALL_T, mat=STONE, n=72,
                    matrix=mxx)
            # these were the only openings left unglazed: the leak test found
            # rays escaping straight through them
            gb = b.new_glass("SM_Glass_%s_%s_Side%s" % (arm, locus, tag))
            G.fill_poly(gb, op, mat=0, matrix=mxx @ Matrix.Translation(
                Vector((0.0, WALL_T * 0.5, 0.0))))
        vault_bay(b, cx, sign * (AISLE_Y + span * 0.5), CROSS_LEN, span,
                  NAVE_SPRING, NAVE_RISE)
    # the crossing itself: the origin of the coordinate plane
    vault_bay(b, cx, 0.0, CROSS_LEN, NAVE_HW * 2.0, NAVE_SPRING,
              NAVE_RISE + 1.5)
    for sx in (NAVE_X1, CROSS_X1):
        for sy in (-NAVE_HW, NAVE_HW):
            G.pier(b.bm("Piers"), (sx, sy), 1.15, PIER_H + 2.0, shafts=8,
                   mat=STONE)


def build_apse(b):
    """Polygonal east end: one facet per conic section."""
    cx = CHOIR_X1
    step = math.pi / APSE_FACETS          # facets must tile the full 180 deg
    bm = b.bm("Apse")
    for i in range(APSE_FACETS):
        a = -math.pi * 0.5 + step * (i + 0.5)
        # a chord's midpoint lies on the apothem, not the circumradius. Placing
        # flat facets out on radius R leaves an angular slot between each pair.
        apothem = APSE_R * math.cos(step * 0.5)
        mid = Vector((cx + math.cos(a) * apothem, math.sin(a) * apothem, 0.0))
        # 8% over the exact chord so neighbours overlap instead of
        # meeting on a hairline that rays can slip through
        facet_w = 2.0 * APSE_R * math.sin(step * 0.5) * 1.08
        outer = G.rect_outline(facet_w, NAVE_SPRING,
                               centre=(0.0, NAVE_SPRING * 0.5))
        opening = G.arch_outline(facet_w * 0.45, 4.0, 6.5, point=1.0)
        mx = (Matrix.Translation(mid)
              @ Matrix.Rotation(a + math.pi * 0.5, 4, "Z") @ yrot(180))
        G.panel(bm, outer, opening, WALL_T, mat=STONE, n=72, matrix=mx)
        gb = b.new_glass("SM_Glass_%s" % APSE_LOCI[i])
        G.fill_poly(gb, opening, mat=0,
                    matrix=mx @ Matrix.Translation(Vector((0, WALL_T * 0.5, 0))))
    # semi-dome over the apse: belongs with the vaults, which get solidified
    bm = b.bm("Vaults")
    res = 14
    grid = []
    for i in range(res + 1):
        u = i / res
        row = []
        for j in range(res + 1):
            a = -math.pi * 0.5 + math.pi * (j / res)
            r = APSE_R * math.cos(u * math.pi * 0.5)
            z = NAVE_SPRING + (NAVE_RISE - 1.5) * math.sin(u * math.pi * 0.5)
            row.append(bm.verts.new((cx + math.cos(a) * r,
                                     math.sin(a) * r, z)))
        grid.append(row)
    for i in range(res):
        for j in range(res):
            try:
                f = bm.faces.new((grid[i][j], grid[i][j + 1],
                                  grid[i + 1][j + 1], grid[i + 1][j]))
                f.material_index = STONE
            except ValueError:
                pass


def build_floor(b):
    bm = b.bm("Floor")
    G.box(bm, (-1.5, -AISLE_Y - 0.5, -0.4), (CHOIR_X1, AISLE_Y + 0.5, 0.0),
          mat=FLOOR)
    cx0, cx1 = NAVE_X1, CROSS_X1
    G.box(bm, (cx0, -TRANSEPT_Y - 0.5, -0.4), (cx1, TRANSEPT_Y + 0.5, 0.0),
          mat=FLOOR)
    res = 16
    ring = []
    for j in range(res + 1):
        a = -math.pi * 0.5 + math.pi * (j / res)
        ring.append(bm.verts.new((CHOIR_X1 + math.cos(a) * APSE_R,
                                  math.sin(a) * APSE_R, 0.0)))
    hub = bm.verts.new((CHOIR_X1, 0.0, 0.0))
    for j in range(res):
        try:
            f = bm.faces.new((hub, ring[j], ring[j + 1]))
            f.material_index = FLOOR
        except ValueError:
            pass



def quad(bm, a, b, c, d, mat, up=True):
    try:
        f = bm.faces.new([bm.verts.new(v) for v in (a, b, c, d)])
    except ValueError:
        return
    f.material_index = mat
    f.normal_update()
    # roof planes are open surfaces seen from outside: normals point up
    if up and f.normal.z < 0.0:
        f.normal_flip()


def build_roofs(b):
    """Pitched roofs hiding the extrados of the vaults, as a real church has."""
    bm = b.bm("Roof")
    eave = CLEAR_Z0 + CLEAR_H              # top of the clerestory wall
    ridge = NAVE_SPRING + NAVE_RISE + 5.0  # clear of the nave vault crown
    aisle_top = AISLE_WALL_H
    aisle_high = CLEAR_Z0          # meets the clerestory sill exactly

    for x0, x1 in ((NAVE_X0, NAVE_X1), (CROSS_X1, CHOIR_X1)):
        # nave gable
        quad(bm, (x0, -NAVE_HW, eave), (x1, -NAVE_HW, eave),
             (x1, 0.0, ridge), (x0, 0.0, ridge), ROOF)
        quad(bm, (x0, 0.0, ridge), (x1, 0.0, ridge),
             (x1, NAVE_HW, eave), (x0, NAVE_HW, eave), ROOF)
        # aisle lean-tos
        for sign in (-1.0, 1.0):
            quad(bm, (x0, sign * AISLE_Y, aisle_top),
                 (x1, sign * AISLE_Y, aisle_top),
                 (x1, sign * NAVE_HW, aisle_high),
                 (x0, sign * NAVE_HW, aisle_high), ROOF)

    # transept arms: gable running the other way
    cx = (NAVE_X1 + CROSS_X1) * 0.5
    for sign in (-1.0, 1.0):
        y0, y1 = sign * AISLE_Y, sign * TRANSEPT_Y
        quad(bm, (NAVE_X1, y0, eave), (NAVE_X1, y1, eave),
             (cx, y1, ridge), (cx, y0, ridge), ROOF)
        quad(bm, (cx, y0, ridge), (cx, y1, ridge),
             (CROSS_X1, y1, eave), (CROSS_X1, y0, eave), ROOF)
        for ye in (y1, y0):
            try:
                f = bm.faces.new([bm.verts.new(v) for v in (
                    (NAVE_X1, ye, eave), (CROSS_X1, ye, eave), (cx, ye, ridge))])
                f.material_index = ROOF
            except ValueError:
                pass

    # crossing roof over the square
    quad(bm, (NAVE_X1, -NAVE_HW, eave), (CROSS_X1, -NAVE_HW, eave),
         (CROSS_X1, 0.0, ridge), (NAVE_X1, 0.0, ridge), ROOF)
    quad(bm, (NAVE_X1, 0.0, ridge), (CROSS_X1, 0.0, ridge),
         (CROSS_X1, NAVE_HW, eave), (NAVE_X1, NAVE_HW, eave), ROOF)

    # corner patches between the crossing roof and the transept arm roofs
    for sign in (-1.0, 1.0):
        quad(bm, (NAVE_X1, sign * NAVE_HW, eave), (CROSS_X1, sign * NAVE_HW, eave),
             (CROSS_X1, sign * AISLE_Y, eave), (NAVE_X1, sign * AISLE_Y, eave), ROOF)

    # gable closing the choir roof against the apse
    try:
        f = bm.faces.new([bm.verts.new(v) for v in (
            (CHOIR_X1, -APSE_R - 0.4, eave), (CHOIR_X1, APSE_R + 0.4, eave),
            (CHOIR_X1, 0.0, ridge))])
        f.material_index = ROOF
    except ValueError:
        pass

    # conical cap over the apse semi-dome
    res = 16
    apex = bm.verts.new((CHOIR_X1, 0.0, ridge))
    ring = []
    for j in range(res + 1):
        a = -math.pi * 0.5 + math.pi * (j / res)
        ring.append(bm.verts.new((CHOIR_X1 + math.cos(a) * (APSE_R + 0.4),
                                  math.sin(a) * (APSE_R + 0.4), eave)))
    for j in range(res):
        try:
            f = bm.faces.new((ring[j], ring[j + 1], apex))
            f.material_index = ROOF
        except ValueError:
            pass



def render_interior(path, objs, loc, look, lens=20.0):
    """Perspective view from standing eye height, for checking the walk."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    sh = scene.display.shading
    sh.light = "STUDIO"
    sh.color_type = "MATERIAL"
    sh.show_shadows = True
    sh.show_cavity = True
    cam_data = bpy.data.cameras.new("InteriorCam")
    cam_data.lens = lens
    cam = bpy.data.objects.new("InteriorCam", cam_data)
    scene.collection.objects.link(cam)
    cam.location = Vector(loc)
    d = (Vector(look) - Vector(loc)).normalized()
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)



def solidify(obj, thickness):
    """Give an open surface real thickness so it is a closed solid.

    Vaults, roof planes and glass are built as single sheets. In Unreal a
    single-sided face is culled from behind, which reads as a hole in the
    building, so they get thickness instead.
    """
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    m = obj.modifiers.new("Solidify", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0.0
    bpy.ops.object.modifier_apply(modifier=m.name)


def open_edges(obj):
    """Edges with only one face: these are literal holes in the surface."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    n = sum(1 for e in bm.edges if len(e.link_faces) < 2)
    bm.free()
    return n


# ---------------------------------------------------------------------- main
def main():
    K.reset_scene()
    mats = [K.material(n, c) for n, c in MATS]
    glass_mat = K.material(*GLASS)

    b = Build()
    build_floor(b)
    # west end closed by the facade, east end by the crossing piers
    build_nave(b, NAVE_X0, NAVE_BAYS, NAVE_LOCI, first_pier=False,
               closing_pier=False)
    build_transept(b)
    build_crossing_walls(b)
    build_nave(b, CROSS_X1, CHOIR_BAYS, CHOIR_LOCI, first_pier=False)
    build_apse(b)
    build_facade(b)
    build_roofs(b)

    # sheet geometry that must be thickened into solids
    SHEET = {"Vaults": 0.35, "Roof": 0.30, "Floor": 0.30, "Apse": 0.0}

    objs, report = [], []
    for key in sorted(b.parts):
        obj = K.object_from_bm("SM_Castle_" + key, b.parts[key], mats,
                               smooth=False)
        if key in SHEET and SHEET[key] > 0.0:
            solidify(obj, SHEET[key])
        K.recalc_outside(obj)
        objs.append(obj)
        report.append((obj.name, K.tri_count(obj), open_edges(obj)))

    glass_objs = []
    for name, gbm in b.glass:
        obj = K.object_from_bm(name, gbm, glass_mat, smooth=False)
        solidify(obj, 0.06)
        glass_objs.append(obj)

    os.makedirs(OUT, exist_ok=True)
    for obj in objs:
        K.export_fbx(obj, os.path.join(OUT, obj.name + ".fbx"))
    # all window glass in one file; the objects stay separately named inside
    bpy.ops.object.select_all(action="DESELECT")
    for obj in glass_objs:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = glass_objs[0]
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(OUT, "SM_Castle_Glass.fbx"), use_selection=True,
        object_types={"MESH"}, mesh_smooth_type="FACE", use_triangles=False,
        bake_space_transform=False, add_leaf_bones=False,
        apply_scale_options="FBX_SCALE_NONE", global_scale=1.0,
        axis_forward="-Z", axis_up="Y", path_mode="AUTO")

    K.render_preview(os.path.join(OUT, "_preview_castle_ext.png"),
                     objs, elevation=18.0, azimuth=36.0)
    K.render_preview(os.path.join(OUT, "_preview_castle_top.png"),
                     objs, elevation=84.0, azimuth=0.0)
    K.render_preview(os.path.join(OUT, "_preview_castle_west.png"),
                     objs, elevation=10.0, azimuth=258.0)
    # player eye height, standing in the nave looking east toward the apse
    render_interior(os.path.join(OUT, "_preview_castle_interior.png"),
                    objs + glass_objs, (8.0, 0.0, 1.7), (80.0, 0.0, 6.0))
    render_interior(os.path.join(OUT, "_preview_castle_crossing.png"),
                    objs + glass_objs, (54.0, 0.0, 1.7), (54.0, 24.0, 12.0))
    K.save_blend(os.path.join(OUT, "Castle_Source.blend"))

    print("CASTLE_REPORT_BEGIN")
    total_open = 0
    for n, t, oe in report:
        total_open += oe
        print("  %-28s %7d tris   open edges: %d" % (n, t, oe))
    print("  TOTAL OPEN EDGES: %d" % total_open)
    print("  glass objects: %d" % len(glass_objs))
    print("  footprint: %.0f x %.0f m, nave crown %.0f m, spires %.0f m"
          % (CHOIR_X1 + APSE_R, TRANSEPT_Y * 2, NAVE_SPRING + NAVE_RISE,
             TOWER_H + SPIRE_H))
    print("CASTLE_REPORT_END")


main()
