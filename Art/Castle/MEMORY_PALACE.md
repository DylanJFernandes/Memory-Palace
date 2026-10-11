# The Precalculus Memory Palace

A gothic cathedral where **the plan is the curriculum**. Walking east from the
door walks forward through the course, in teaching order, and the building's
own geometry carries the mathematics rather than decorating it.

Footprint 87 × 44 m · nave crown 30 m · spires 48 m · ~61k triangles.

---

## Why gothic, specifically

This is not a style choice. Four features of a gothic cathedral are already
the shapes precalculus needs:

| Feature | What it already is |
|---|---|
| Rose window | a circle divided into 12 — **the unit circle** |
| Latin-cross crossing | two perpendicular axes meeting at a point — **the origin** |
| Polygonal apse | facets radiating from one point — **conics from a focus** |
| Repeating nave bays | an ordered, countable sequence of identical loci |

A memory palace needs loci that are distinct, ordered, and always traversed in
the same sequence. A nave arcade is exactly that, and it has been used for
precisely this purpose since antiquity.

---

## The walk

```
        N transept                      
        TRIGONOMETRY                    
             |                          
  [rose]  1 2 3 4 5 6 7 8  [CROSSING]  9 10 11  (apse)
  entry   <--- nave bays --->  origin   choir    conics
             |                          
        S transept                      
        COMPLEX / POLAR                 
```

### Entry — the rose window
`SM_Glass_00_RoseWindow_UnitCircle`

Twelve stone spokes, modelled, at the twelve standard angles. The player sees
it on entry and again every time they turn back west, so the unit circle is
the one thing in the building they cannot avoid. Put the angle values on the
spokes and the coordinates in the outer ring.

### Nave — the function families, bays 1→8

Each bay has **four** windows: two aisle (low, reachable, player-facing) and
two clerestory (high, visible from the centre of the nave). Suggested split —
aisle windows pose, clerestory windows confirm.

| Bay | Locus | The idea the bay should teach |
|---|---|---|
| 1 | Functions | input → output, the vertical line test |
| 2 | Domain & Range | what you may feed in, what can come out |
| 3 | Transformations | shift, stretch, reflect |
| 4 | Polynomials | degree, end behaviour, roots |
| 5 | Rational & Asymptotes | where it breaks, what it approaches |
| 6 | Exponentials | repeated multiplication, growth |
| 7 | Logarithms | the inverse — undoing bay 6 |
| 8 | Sequences & Series | the bays themselves are a sequence; sum them |

Bays 6 and 7 are deliberately adjacent: the player should be able to stand
between them and see both windows at once, because the whole point is that
they are inverses.

Bay 8 is the payoff of the walk — by the time the player reaches it they have
passed seven identical bays, which *is* a sequence. Ask for the 8th term, or
the sum, using the bays they just walked.

### The crossing — the coordinate plane
Four great piers, and the only point in the building where two axes meet.

Stand at the crossing and the nave runs east–west, the transept north–south.
This is the origin, and it is the one place where the player's own position is
a coordinate pair. Vectors, quadrants, and distance belong here — ideally as
something the player solves by *walking to a point* rather than by answering.

### North transept — Trigonometry
`SM_Glass_North_Trigonometry` — one great window.
The unit circle again, now at ground level and walkable. The rose window at
the far west end is visible down the length of the building from here: the
same circle, twice, at both ends of the axis.

### South transept — Complex numbers & polar
`SM_Glass_South_ComplexPolar`
Directly opposite trigonometry, because polar form *is* trigonometry wearing a
different coat. The symmetry of the transept is the argument.

### Choir — bays 9 to 11
Identities, inverse trig, matrices. Narrower, further in, after the crossing:
the material that assumes everything before it.

### Apse — the conic sections
Five facets, five windows: `12_Circle`, `13_Ellipse`, `14_Parabola`,
`15_Hyperbola`, `16_Degenerate`.

The apse facets radiate from a single centre, which is the actual definition
the conics share — each is a locus of points related to a focus. The player
stands at that centre and turns. Eccentricity increases as they sweep from
circle to hyperbola.

---

## Teaching through movement, not menus

The reason to build this as a space rather than a quiz is that the player
should solve things **by moving**:

- Walk to the coordinate the crossing asks for.
- Face the angle the rose window names.
- Pace out a period along the nave bays.
- Stand at the apse focus and pick the facet whose eccentricity matches.

Anything that can be answered standing still could have been a worksheet.

---

## Files

| File | Contents |
|---|---|
| `SM_Castle_Floor/Piers/Arches/Walls/Vaults` | the structure |
| `SM_Castle_Facade/Towers/Transept/Apse` | west front, towers, arms, east end |
| `SM_Castle_Buttresses/Roof` | buttresses, flying buttresses, roofs |
| `SM_Castle_Glass.fbx` | **56 separately named window objects**, one per locus |
| `Castle_Source.blend` | full Blender scene |

Material slots: `M_CastleStone`, `M_CastleFloor`, `M_CastleRoof`, and
`M_CastleGlass` on the windows.

The glass file is the important one for gameplay — every window is its own
object named for its locus (`SM_Glass_04_Polynomials_N`), so the game can bind
an interaction, a texture, or a puzzle to each by name without hand-tagging.

---

## Scale and clearances

Nave 12 m wide, aisles 5 m, arcade springs at 9 m, nave vault crown 30 m. All
walkable at human scale; the portal and the arcade arches are open passages.
Built in metres, so it imports to Unreal at 1 uu = 1 cm with scale 1.

## Not done

- **No interior stairs.** The towers are shells with belfry openings and
  spires; there is no way up them yet. A spiral stair would be a good locus
  for exponential growth if you want one later.
- **No tracery** in the lancet windows — they are plain pointed openings. Only
  the rose window has bars.
- No doors, furniture, altar, or floor pattern.
- No LODs, no lightmap UVs.

## Collision — important

The meshes are imported with **simple collision removed** and
`CollisionTraceFlag = CTF_UseComplexAsSimple`, i.e. the player collides with
the actual triangles.

This is not optional. Unreal's importer generates a **convex hull** as simple
collision, and the convex hull of a wall with a doorway in it is a solid slab:
the great portal, every arcade arch and every passage becomes impassable while
still *looking* open. If you ever re-import these meshes, strip the simple
collision and set complex-as-simple again or the building will be unwalkable.

The trade-off is that per-triangle collision is more expensive than primitives.
If that ever matters, replace it with hand-placed box collision that leaves the
openings clear — not with generated hulls.

## Watertight

Verified two ways: **0 open edges** (every mesh is a closed solid), and a
**ray leak test** — 900 rays cast in random directions from twelve interior
points, counting any that escape to the sky. Aisles, transepts, choir and
apse are fully sealed. The only rays that escape go west through the great
portal, which is the entrance and is meant to be open. If you later fit a
door, the building becomes completely closed.

The leak test is the useful one: open-edge counting cannot find a *missing
wall*, only a torn one. Three of the worst holes here were regions with no
geometry at all, which the edge audit reported as perfectly clean.


Every one of the eleven meshes is a closed solid — **0 open edges** across the
whole building, verified by counting edges with fewer than two faces. There
are no single-sided sheets, so nothing is culled away into a hole when viewed
from behind in Unreal.

What this required, for reference if you edit the generator:

- The arcade is a **wall with arched openings**, not free-standing arches.
  Originally nothing existed between the arch tops and the clerestory sill,
  which left the nave open to the aisle roof void.
- Vaults, roofs, floor and glass are **solidified** (0.3 m, glass 0.06 m);
  they are authored as single sheets and would otherwise be one-sided.
- The apse semi-dome is grouped with `Vaults` so it gets that thickness.
- `spire()` caps its base; 22 buttress pinnacles and 2 tower spires were open
  cones.
- The facade's upper stage reaches above the roof ridge, the apse wall reaches
  the eave, and the transept side walls reach the eave — all three previously
  stopped short and left a band of daylight.
- Untextured — flat material colours only, same as the island kit before
  Substance. The stone set in `Art/Textures/T_Stone_*` will suit the walls.
