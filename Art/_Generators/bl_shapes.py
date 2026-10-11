"""Reusable organic geometry primitives for the tropical island kits.

Everything builds into a caller-supplied bmesh and tags faces with a material
index, so a single mesh can carry separate bark / leaf / shell slots.
"""

import math
import random

import bmesh
from mathutils import Matrix, Vector

TAU = math.tau


# ----------------------------------------------------------------- framing
def frame_from_tangent(tangent, hint=None):
    """Orthonormal basis around a tangent, avoiding the degenerate case."""
    t = tangent.normalized()
    up = hint or Vector((0.0, 0.0, 1.0))
    if abs(t.dot(up)) > 0.94:
        up = Vector((1.0, 0.0, 0.0))
    x = t.cross(up).normalized()
    y = t.cross(x).normalized()
    return x, y


def smoothstep(t):
    t = min(max(t, 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


# -------------------------------------------------------------------- tube
def add_tube(bm, points, radii, segs=10, mat=0, cap_start=True, cap_end=True,
             ridge=0.0, ridge_freq=9.0, jitter=0.0, rng=None, uv=None,
             uv_vscale=1.0):
    """Sweep a closed ring along a polyline. Returns the ring vertex lists."""
    rng = rng or random.Random(0)
    n = len(points)
    rings = []
    total = 0.0
    lengths = [0.0]
    for i in range(1, n):
        total += (points[i] - points[i - 1]).length
        lengths.append(total)

    for i, (p, r) in enumerate(zip(points, radii)):
        if i == 0:
            tan = points[1] - points[0]
        elif i == n - 1:
            tan = points[-1] - points[-2]
        else:
            tan = points[i + 1] - points[i - 1]
        x, y = frame_from_tangent(tan)
        v = lengths[i] / max(total, 1e-6)
        ring = []
        for s in range(segs):
            a = s / segs * TAU
            # horizontal ridging reads as bark banding
            rr = r * (1.0 + ridge * math.sin(v * ridge_freq * TAU))
            if jitter:
                rr *= 1.0 + rng.uniform(-jitter, jitter)
            co = p + x * (math.cos(a) * rr) + y * (math.sin(a) * rr)
            ring.append(bm.verts.new(co))
        rings.append((ring, v))

    for i in range(n - 1):
        r0, v0 = rings[i]
        r1, v1 = rings[i + 1]
        for s in range(segs):
            s2 = (s + 1) % segs
            try:
                f = bm.faces.new((r0[s], r0[s2], r1[s2], r1[s]))
            except ValueError:
                continue
            f.material_index = mat
            if uv is not None:
                us = s / segs
                us2 = (s + 1) / segs
                for loop, (uu, vv) in zip(f.loops,
                                          ((us, v0 * uv_vscale),
                                           (us2, v0 * uv_vscale),
                                           (us2, v1 * uv_vscale),
                                           (us, v1 * uv_vscale))):
                    loop[uv].uv = (uu, vv)

    if cap_start:
        try:
            f = bm.faces.new(list(reversed(rings[0][0])))
            f.material_index = mat
        except ValueError:
            pass
    if cap_end:
        try:
            f = bm.faces.new(rings[-1][0])
            f.material_index = mat
        except ValueError:
            pass
    return rings


# ------------------------------------------------------------------- blade
def add_blade(bm, root, direction, side, length, halfwidth, droop=0.0,
              curve=0.0, segs=4, mat=0, taper=1.0, uv=None, uv_cell=(0.0, 1.0),
              fold=0.0):
    """A tapered leaf blade: quad strip narrowing to a point.

    `fold` lifts the centre line into a shallow V, which keeps thin leaves from
    looking like flat paper when lit.
    """
    d = direction.normalized()
    s = side.normalized()
    up = s.cross(d).normalized()
    u0, u1 = uv_cell

    stations = []
    for i in range(segs + 1):
        t = i / segs
        pos = root + d * (length * t)
        pos += up * (-droop * t * t * length)
        pos += s * (curve * t * t * length)
        hw = halfwidth * max((1.0 - t ** taper), 0.0)
        stations.append((pos, hw, t))

    cols = []
    for pos, hw, t in stations:
        lift = up * (fold * hw)
        cols.append([
            bm.verts.new(pos - s * hw),
            bm.verts.new(pos + lift),
            bm.verts.new(pos + s * hw),
        ])

    for i in range(segs):
        for c in (0, 1):
            a, b = cols[i][c], cols[i][c + 1]
            cc, dd = cols[i + 1][c + 1], cols[i + 1][c]
            try:
                f = bm.faces.new((a, b, cc, dd))
            except ValueError:
                continue
            f.material_index = mat
            if uv is not None:
                t0, t1 = stations[i][2], stations[i + 1][2]
                ua = u0 + (u1 - u0) * (c * 0.5)
                ub = u0 + (u1 - u0) * ((c + 1) * 0.5)
                for loop, (uu, vv) in zip(f.loops, ((ua, t0), (ub, t0),
                                                    (ub, t1), (ua, t1))):
                    loop[uv].uv = (uu, vv)
    return cols


# ------------------------------------------------------------------- frond
def add_frond(bm, root, direction, side, length, leaflets=22, leaflet_len=0.42,
              droop=0.55, mat_leaf=1, rng=None, uv=None, spine_width=0.022,
              scale=1.0):
    """Palm/fern frond: drooping rachis with paired leaflets.

    Leaflet length and angle are jittered so no two fronds read identically.
    """
    rng = rng or random.Random(0)
    d = direction.normalized()
    s = side.normalized()
    up = s.cross(d).normalized()

    spine = []
    segs = max(leaflets // 2, 6)
    for i in range(segs + 1):
        t = i / segs
        pos = root + d * (length * t) + up * (-droop * length * t * t)
        spine.append((pos, t))

    # rachis as a narrow tapered strip
    cols = []
    for pos, t in spine:
        hw = spine_width * scale * (1.0 - 0.75 * t)
        cols.append((bm.verts.new(pos - s * hw), bm.verts.new(pos + s * hw), t))
    for i in range(segs):
        a0, a1, t0 = cols[i]
        b0, b1, t1 = cols[i + 1]
        try:
            f = bm.faces.new((a0, a1, b1, b0))
            f.material_index = mat_leaf
            if uv is not None:
                for loop, (uu, vv) in zip(f.loops, ((0.0, t0), (0.06, t0),
                                                    (0.06, t1), (0.0, t1))):
                    loop[uv].uv = (uu, vv)
        except ValueError:
            pass

    for i in range(leaflets):
        t = 0.06 + 0.92 * (i / max(leaflets - 1, 1))
        # interpolate along the spine rather than snapping to a station, so
        # leaflet spacing stays even however coarse the rachis is
        fi = min(t * segs, segs - 1e-4)
        i0 = int(fi)
        frac = fi - i0
        pos = spine[i0][0].lerp(spine[min(i0 + 1, segs)][0], frac)

        # leaflets shorten toward base and tip and sweep gently backward
        env = math.sin(min(t, 1.0) * math.pi) ** 0.40
        ln = leaflet_len * scale * env * rng.uniform(0.88, 1.12)
        sweep = rng.uniform(0.22, 0.38)
        dr = rng.uniform(0.25, 0.55)
        for sign in (-1.0, 1.0):
            lateral = (s * sign * (1.0 - sweep) + d * sweep).normalized()
            add_blade(bm, pos, lateral, d, ln,
                      halfwidth=ln * rng.uniform(0.16, 0.23),
                      droop=dr, curve=rng.uniform(-0.1, 0.1), segs=2,
                      mat=mat_leaf, taper=2.0, uv=uv, uv_cell=(0.12, 1.0),
                      fold=0.30)
    return spine


# ------------------------------------------------------------------ blobby
def add_displaced_sphere(bm, center, radius, subdiv=2, noise=0.22, seed=0,
                         mat=0, squash=1.0, freq=2.2):
    """Icosphere pushed around by layered sine noise -> believable stone/foliage."""
    rng = random.Random(seed)
    res = bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=radius)
    new_verts = res["verts"]
    # 4.5's create_icosphere returns verts only; walk to the faces it made
    vset = set(new_verts)
    faces = {f for v in new_verts for f in v.link_faces
             if all(fv in vset for fv in f.verts)}
    phases = [(rng.uniform(0, TAU), rng.uniform(0, TAU), rng.uniform(0, TAU))
              for _ in range(3)]
    amps = [noise, noise * 0.5, noise * 0.26]
    scales = [freq, freq * 2.3, freq * 4.7]
    for v in new_verts:
        n = v.co.normalized()
        disp = 0.0
        for (px, py, pz), amp, sc in zip(phases, amps, scales):
            disp += amp * (math.sin(n.x * sc + px)
                           * math.sin(n.y * sc + py)
                           * math.sin(n.z * sc + pz))
        v.co = n * (radius * (1.0 + disp))
        v.co.z *= squash
        v.co += center
    for f in faces:
        f.material_index = mat
    return {"verts": new_verts, "faces": list(faces)}


def add_lofted_cliff(bm, width, depth, height, layers=20, radial=24, seed=0,
                     steps=5, mat=0, ledge=0.13, taper=0.42):
    """Sedimentary cliff: stacked rings whose radius shrinks in plateaus.

    A noised cube just reads as a melted box. Stepping the taper and bumping
    the radius at the start of each step gives actual horizontal strata.
    """
    rng = random.Random(seed)
    p1, p2, p3 = (rng.uniform(0, TAU) for _ in range(3))
    # irregular strata breaks; evenly spaced ones read as a stack of pancakes
    breaks = sorted(rng.uniform(0.10, 0.94) for _ in range(max(steps - 1, 1)))
    lean_x, lean_y = rng.uniform(-0.10, 0.10), rng.uniform(-0.08, 0.08)
    # strong low-harmonic lobes keep the plan shape off-circular
    a1, a2 = rng.uniform(0.22, 0.34), rng.uniform(0.11, 0.18)
    elong = rng.uniform(1.0, 1.45)

    rings = []
    for i in range(layers + 1):
        t = i / layers
        k = sum(1 for b in breaks if t >= b)
        last = breaks[k - 1] if k else 0.0
        shrink = 1.0 - taper * (k / max(len(breaks), 1)) - 0.05 * (t - last)
        lip = 1.0 + ledge * math.exp(-(t - last) * 14.0)
        ring = []
        for j in range(radial):
            th = j / radial * TAU
            rr = (1.0
                  + a1 * math.sin(2.0 * th + p1 + t * 0.9)
                  + a2 * math.sin(3.0 * th + p2 - t * 0.6)
                  + 0.08 * math.sin(7.0 * th + p3)
                  + 0.04 * math.sin(13.0 * th + p1 * 2.0))
            ring.append(bm.verts.new((
                math.cos(th) * width * 0.5 * elong * shrink * rr * lip
                + lean_x * width * t,
                math.sin(th) * depth * 0.5 * shrink * rr * lip
                + lean_y * depth * t,
                height * t,
            )))
        rings.append(ring)

    for i in range(layers):
        for j in range(radial):
            j2 = (j + 1) % radial
            try:
                f = bm.faces.new((rings[i][j], rings[i][j2],
                                  rings[i + 1][j2], rings[i + 1][j]))
                f.material_index = mat
            except ValueError:
                pass
    for ring, flip in ((rings[0], True), (rings[-1], False)):
        try:
            f = bm.faces.new(list(reversed(ring)) if flip else ring)
            f.material_index = mat
        except ValueError:
            pass
    return rings


def add_rounded_box(bm, center, size, seed=0, noise=0.12, subdiv=3, mat=0):
    """Subdivided cube, noised and smoothed: layered cliff-ish mass."""
    rng = random.Random(seed)
    pre = set(bm.verts)
    res = bmesh.ops.create_cube(bm, size=1.0)
    cube_verts = set(res["verts"])
    cube_edges = [e for e in bm.edges if all(v in cube_verts for v in e.verts)]
    bmesh.ops.subdivide_edges(bm, edges=cube_edges, cuts=subdiv,
                              use_grid_fill=True)

    # anything that did not exist before this call belongs to this box
    mine = [v for v in bm.verts if v not in pre]
    mset = set(mine)
    sx, sy, sz = size
    phases = [rng.uniform(0, TAU) for _ in range(4)]
    for v in mine:
        p = v.co.copy()
        v.co = Vector((p.x * sx, p.y * sy, p.z * sz))
        # strata: banding in Z plus lateral wobble
        band = math.sin(v.co.z * 2.6 + phases[0]) * 0.5 + 0.5
        d = (noise * (0.4 + 0.6 * band)
             * math.sin(v.co.x * 1.9 + phases[1])
             * math.sin(v.co.y * 2.1 + phases[2]))
        lateral = Vector((p.x, p.y, 0.0))
        if lateral.length > 1e-6:
            v.co += lateral.normalized() * d * max(sx, sy)
        v.co.z += 0.35 * d * sz * math.sin(v.co.x * 1.3 + phases[3])
        v.co += center

    faces = [f for f in bm.faces if all(fv in mset for fv in f.verts)]
    for f in faces:
        f.material_index = mat
    return {"verts": mine, "faces": faces}
