"""Gothic architectural primitives.

The load-bearing idea here is `panel()`: a wall slab with a hole in it, built
as a radial ring of quads between the outer rectangle and the opening outline.
Booleans in bmesh are fragile; ray-casting both outlines from a shared centre
gives clean quad topology with no boolean at all, and works for any opening
shape that is star-shaped about that centre (arches, circles, foils all are).
"""

import math

import bmesh
from mathutils import Matrix, Vector

TAU = math.tau


# ------------------------------------------------------------------ outlines
def arch_outline(span, sill, straight, point=1.0, n=28):
    """Two-centred (pointed) arch opening as a closed 2D loop in (x, z).

    `point` 0 = semicircular, 1 = equilateral gothic. The arcs are real
    circle segments: the right-hand arc is centred at (-c, springing) with
    radius hs + c, which puts the springing exactly on the jambs.
    """
    hs = span * 0.5
    c = point * hs
    R = hs + c
    spring = sill + straight
    pts = [Vector((-hs, sill)), Vector((-hs, spring))]
    for i in range(1, n + 1):
        x = -hs + hs * (i / n)
        pts.append(Vector((x, spring + math.sqrt(max(R * R - (x - c) ** 2, 0.0)))))
    for i in range(1, n + 1):
        x = hs * (i / n)
        pts.append(Vector((x, spring + math.sqrt(max(R * R - (x + c) ** 2, 0.0)))))
    pts.append(Vector((hs, spring)))
    pts.append(Vector((hs, sill)))
    return pts


def arch_rise(span, point=1.0):
    hs = span * 0.5
    c = point * hs
    return math.sqrt((hs + c) ** 2 - c * c)


def circle_outline(radius, centre=(0.0, 0.0), n=64):
    return [Vector((centre[0] + math.cos(i / n * TAU) * radius,
                    centre[1] + math.sin(i / n * TAU) * radius))
            for i in range(n)]


def rect_outline(w, h, centre=(0.0, 0.0)):
    hw, hh = w * 0.5, h * 0.5
    cx, cz = centre
    return [Vector((cx - hw, cz - hh)), Vector((cx + hw, cz - hh)),
            Vector((cx + hw, cz + hh)), Vector((cx - hw, cz + hh))]


def doorway_outline(width, height, opening):
    """Wall rectangle with a floor-reaching opening notched out of its base.

    panel()'s radial ring degenerates whenever the opening touches the wall
    boundary: straight down, the inner and outer radii coincide, which
    collapses the ring into zero-area slivers. A doorway is therefore built
    as one simple concave polygon and extruded, with no ring at all.

    `opening` must run left jamb -> over the arch -> right jamb, which is the
    order arch_outline() produces.
    """
    hw = width * 0.5
    return ([Vector((-hw, 0.0))] + list(opening) +
            [Vector((hw, 0.0)), Vector((hw, height)), Vector((-hw, height))])


def centroid(poly):
    return sum(poly, Vector((0.0, 0.0))) / len(poly)


def poly_radius(poly, c, theta):
    """Distance from c to the polygon along direction theta."""
    d = Vector((math.cos(theta), math.sin(theta)))
    best = None
    n = len(poly)
    for i in range(n):
        a = poly[i] - c
        e = poly[(i + 1) % n] - a - c
        den = d.x * e.y - d.y * e.x
        if abs(den) < 1e-12:
            continue
        s = (a.x * e.y - a.y * e.x) / den
        t = (a.x * d.y - a.y * d.x) / den
        if s > 1e-9 and -1e-6 <= t <= 1.0 + 1e-6:
            if best is None or s < best:
                best = s
    return best if best is not None else 0.0


# --------------------------------------------------------------------- panel
def panel(bm, outer, opening, thickness, mat=0, n=96, matrix=None,
          centre=None):
    """Wall slab in local XZ, thickness along +Y, with `opening` cut out.

    Both outlines are sampled by ray-casting from a shared centre, so inner
    and outer rings share a parameterisation and connect as clean quads.
    """
    c = Vector(centre) if centre is not None else centroid(opening)
    mx = matrix or Matrix.Identity(4)

    def place(x, y, z):
        return bm.verts.new(mx @ Vector((x, y, z)))

    front_in, front_out, back_in, back_out = [], [], [], []
    for i in range(n):
        th = i / n * TAU
        ri = poly_radius(opening, c, th)
        ro = poly_radius(outer, c, th)
        ix, iz = c.x + math.cos(th) * ri, c.y + math.sin(th) * ri
        ox, oz = c.x + math.cos(th) * ro, c.y + math.sin(th) * ro
        front_in.append(place(ix, 0.0, iz))
        front_out.append(place(ox, 0.0, oz))
        back_in.append(place(ix, thickness, iz))
        back_out.append(place(ox, thickness, oz))

    def strip(a, b, flip=False):
        for i in range(n):
            j = (i + 1) % n
            quad = (a[i], a[j], b[j], b[i])
            try:
                f = bm.faces.new(quad[::-1] if flip else quad)
                f.material_index = mat
            except ValueError:
                pass

    strip(front_in, front_out)            # front face
    strip(back_in, back_out, flip=True)   # back face
    strip(front_in, back_in, flip=True)   # reveal, facing into the opening
    strip(front_out, back_out)            # outer edge
    return front_in


def solid_panel(bm, outer, thickness, mat=0, matrix=None):
    """Wall slab with no opening."""
    mx = matrix or Matrix.Identity(4)
    front = [bm.verts.new(mx @ Vector((p.x, 0.0, p.y))) for p in outer]
    back = [bm.verts.new(mx @ Vector((p.x, thickness, p.y))) for p in outer]
    n = len(outer)
    try:
        f = bm.faces.new(front[::-1])
        f.material_index = mat
    except ValueError:
        pass
    try:
        f = bm.faces.new(back)
        f.material_index = mat
    except ValueError:
        pass
    for i in range(n):
        j = (i + 1) % n
        try:
            f = bm.faces.new((front[i], front[j], back[j], back[i]))
            f.material_index = mat
        except ValueError:
            pass
    return front


def fill_poly(bm, poly, mat=0, matrix=None, y=0.0):
    """Flat face filling a loop — used for window glass."""
    mx = matrix or Matrix.Identity(4)
    vs = [bm.verts.new(mx @ Vector((p.x, y, p.y))) for p in poly]
    try:
        f = bm.faces.new(vs)
        f.material_index = mat
        return f
    except ValueError:
        return None


# --------------------------------------------------------------------- sweep
def sweep_rect(bm, path, up, width, depth, mat=0, cap=True):
    """Sweep a rectangular section along a 3D path — ribs, mouldings, arches."""
    rings = []
    n = len(path)
    for i, p in enumerate(path):
        if i == 0:
            tan = path[1] - path[0]
        elif i == n - 1:
            tan = path[-1] - path[-2]
        else:
            tan = path[i + 1] - path[i - 1]
        tan.normalize()
        side = tan.cross(up)
        if side.length < 1e-6:
            side = tan.cross(Vector((1.0, 0.0, 0.0)))
        side.normalize()
        u = side.cross(tan).normalized()
        hw, hd = width * 0.5, depth * 0.5
        rings.append([
            bm.verts.new(p - side * hw - u * hd),
            bm.verts.new(p + side * hw - u * hd),
            bm.verts.new(p + side * hw + u * hd),
            bm.verts.new(p - side * hw + u * hd),
        ])
    for i in range(n - 1):
        for k in range(4):
            k2 = (k + 1) % 4
            try:
                f = bm.faces.new((rings[i][k], rings[i][k2],
                                  rings[i + 1][k2], rings[i + 1][k]))
                f.material_index = mat
            except ValueError:
                pass
    if cap:
        for ring, rev in ((rings[0], True), (rings[-1], False)):
            try:
                f = bm.faces.new(ring[::-1] if rev else ring)
                f.material_index = mat
            except ValueError:
                pass
    return rings


def arch_path(span, point, spring_z, axis, cross, origin, n=24):
    """3D points following a pointed arch, for sweeping an arcade or rib."""
    hs = span * 0.5
    c = point * hs
    R = hs + c
    pts = []
    for i in range(n + 1):
        t = -1.0 + 2.0 * (i / n)
        x = t * hs
        z = math.sqrt(max(R * R - (abs(x) + c) ** 2, 0.0))
        pts.append(Vector(origin) + axis * x + cross * 0.0 + Vector((0, 0, spring_z + z)))
    return pts


# --------------------------------------------------------------------- vault
def groin_vault(bm, origin, size_x, size_y, spring_z, rise, point=1.0,
                res=14, mat=0):
    """Quadripartite vault: the lower of two crossing pointed barrels.

    A groin vault is literally the intersection of two barrel vaults, so
    taking min() of the two arch profiles gives the correct surface including
    the diagonal groins.
    """
    ox, oy = origin[0], origin[1]

    def prof(t):
        """Normalised pointed arch height for t in [-1, 1]."""
        c = point
        R = 1.0 + c
        return math.sqrt(max(R * R - (abs(t) + c) ** 2, 0.0)) / math.sqrt(
            max(R * R - c * c, 1e-9))

    grid = []
    for i in range(res + 1):
        u = -1.0 + 2.0 * (i / res)
        row = []
        for j in range(res + 1):
            v = -1.0 + 2.0 * (j / res)
            z = spring_z + rise * min(prof(u), prof(v))
            row.append(bm.verts.new((ox + u * size_x * 0.5,
                                     oy + v * size_y * 0.5, z)))
        grid.append(row)
    for i in range(res):
        for j in range(res):
            try:
                f = bm.faces.new((grid[i][j], grid[i][j + 1],
                                  grid[i + 1][j + 1], grid[i + 1][j]))
            except ValueError:
                continue
            f.material_index = mat
            # a vault is an open surface only ever seen from below, so its
            # normals must face down or it renders invisible from inside
            f.normal_update()
            if f.normal.z > 0.0:
                f.normal_flip()
    return grid


# ---------------------------------------------------------------------- pier
def pier(bm, pos, radius, height, shafts=8, shaft_r=None, mat=0, sides=8):
    """Clustered gothic pier: octagonal core ringed by engaged shafts."""
    shaft_r = shaft_r or radius * 0.30
    x, y = pos

    def prism(cx, cy, r, z0, z1, k):
        low, high = [], []
        for i in range(k):
            a = i / k * TAU
            px, py = cx + math.cos(a) * r, cy + math.sin(a) * r
            low.append(bm.verts.new((px, py, z0)))
            high.append(bm.verts.new((px, py, z1)))
        for i in range(k):
            j = (i + 1) % k
            try:
                f = bm.faces.new((low[i], low[j], high[j], high[i]))
                f.material_index = mat
            except ValueError:
                pass
        for ring, rev in ((low, True), (high, False)):
            try:
                f = bm.faces.new(ring[::-1] if rev else ring)
                f.material_index = mat
            except ValueError:
                pass

    base_h, cap_h = radius * 0.5, radius * 0.55
    prism(x, y, radius * 1.35, 0.0, base_h, sides)                  # base
    prism(x, y, radius, base_h, height - cap_h, sides)              # core
    prism(x, y, radius * 1.30, height - cap_h, height, sides)       # capital
    for i in range(shafts):
        a = i / shafts * TAU
        sx, sy = x + math.cos(a) * radius, y + math.sin(a) * radius
        prism(sx, sy, shaft_r, base_h, height - cap_h, 6)


def box(bm, lo, hi, mat=0):
    x0, y0, z0 = lo
    x1, y1, z1 = hi
    v = [bm.verts.new(p) for p in (
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1))]
    for quad in ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
                 (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)):
        try:
            f = bm.faces.new([v[i] for i in quad])
            f.material_index = mat
        except ValueError:
            pass
    return v


def spire(bm, centre, radius, base_z, height, sides=8, mat=0):
    cx, cy = centre
    ring = [bm.verts.new((cx + math.cos(i / sides * TAU) * radius,
                          cy + math.sin(i / sides * TAU) * radius, base_z))
            for i in range(sides)]
    apex = bm.verts.new((cx, cy, base_z + height))
    for i in range(sides):
        j = (i + 1) % sides
        try:
            f = bm.faces.new((ring[i], ring[j], apex))
            f.material_index = mat
        except ValueError:
            pass
    # cap the base, or the cone is an open shell
    try:
        f = bm.faces.new(ring[::-1])
        f.material_index = mat
    except ValueError:
        pass
    return apex
