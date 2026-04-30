# Insydium MeshTools Reference

MeshTools generators from Insydium. Each takes a child polygon object and applies a procedural operation. All are generator objects — place a mesh (e.g. Cube) as a child.

All MeshTools generators share common sections:
- **Selection Source**: Selection Map (link), Texture Source (Shader/Texture Tag), Tolerance
- **Selections tab**: Toggle polygon/edge/point selection output with display colors
- **Advanced > Optimize**: Polygons, Points, Unused Points, Tolerance (default 0.01)
- **Fields** (440000245): FieldList for falloff control

---

## mtSubDivider (1057183)

Subdivides selected polygons. Multiple subdivision algorithms available.

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10029 | Mode | Int32 | 0 | 0=Bilinear, 4=Bilinear Offset, 2=Catmull-Clark, 1=Shards, 3=Whirl |
| 10010 | Subdivision | Int32 | 1 | Number of subdivision iterations |
| 10030 | Strength | Float | 0.5 | Subdivision strength |
| 10031 | Detail Control | SplineData | — | Spline curve for detail falloff |
| 10035 | Offset Strength | Float | 0.5 | |
| 10036 | Offset Axis | Int32 | 2 | 0=U, 1=V, 2=Random |
| 10037 | U | Float | 0.5 | U offset amount |
| 10038 | V | Float | 0.5 | V offset amount |
| 10032 | Seed | Int32 | 51248 | Random seed |
| 10018 | Hide Subdivided | Bool | False | |
| 10019 | Hide Non-Subdivided | Bool | False | |

### Selection Source
| ID | Name | Type | Default |
|----|------|------|---------|
| 10005 | Selection Map | Link | — |
| 10006 | Texture Source | Int32 | 10007 | 10007=Shader, 10008=Texture Tag |
| 10004 | Shader | Link | — |
| 10003 | Texture Tag | Link | — |
| 10028 | Texture Channel | Int32 | 0 | 0=Color, 1=Luminance, 2=Transparency, 6=Bump, 7=Alpha, 11=Displacement, 12=Diffusion |
| 10009 | Tolerance | Float | 0.5 | |

### Example
```python
import c4d

doc = c4d.documents.GetActiveDocument()
sub = c4d.BaseObject(1057183)  # mtSubDivider
sub[10029] = 2   # Catmull-Clark
sub[10010] = 2   # 2 subdivisions
sub[10030] = 1.0 # full strength

cube = c4d.BaseObject(c4d.Ocube)
cube.InsertUnder(sub)
doc.InsertObject(sub)
c4d.EventAdd()
```

---

## mtShellGen (1057182)

Adds thickness (shell) to polygons, with optional bevel on edges.

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10002 | Thickness | Float | 0.0 | Shell thickness |
| 10008 | Max Angle | Float | π/2 | Maximum angle (radians) |
| 10007 | Variation | Float | 0.0 | Thickness variation |
| 10003 | Steps | Int32 | 1 | Shell steps |
| 10001 | Create Cap | Bool | True | Cap open edges |

### Bevel
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10020 | Bevel Edges | Bool | False | Enable edge beveling |
| 10023 | Bevel Mode | Int32 | 0 | 0=Chamfer, 1=Solid |
| 10021 | Offset Mode | Int32 | 0 | 0=Fixed Distance, 1=Radial, 2=Proportional |
| 10027 | Offset | Float | 0.0 | Bevel offset amount |
| 10022 | Subdivision | Int32 | 0 | Bevel subdivisions |
| 10025 | Depth | Float | 0.0 | Bevel depth |
| 10029 | Limit | Bool | False | |
| 10031 | Shape | Int32 | 0 | 0=Round, 1=User, 2=Profile |
| 10026 | Tension | Float | 1.0 | |
| 10032 | User Shape | SplineData | — | Custom bevel profile |
| 10034 | Symmetry | Bool | True | |
| 10036 | Profile Spline | Link | — | External spline for profile |
| 10037 | Profile Plane | Int32 | 0 | 0=XY, 1=XZ, 2=YZ |
| 10035 | Constant Cross Section | Bool | False | |
| 10080 | Mitering | Int32 | 0 | 0=Default, 1=Uniform, 2=Radial, 3=Patch |
| 10081 | Ending | Int32 | 0 | 0=Default, 1=Extend, 2=Inset |
| 10082 | Partial Rounding | Int32 | 0 | 0=None, 1=Full, 2=Convex |
| 10051 | Use Steps | Bool | True | |
| 10052 | Bevel Step Mode | Int32 | 10053 | 10053=All, 10054=Topmost |

### Visibility
| ID | Name | Type | Default |
|----|------|------|---------|
| 10046 | Hide Hull Face | Bool | False |
| 10047 | Hide Outer Face | Bool | False |
| 10048 | Hide Caps | Bool | False |
| 10049 | Hide Rounding | Bool | False |

### Selection Source
| ID | Name | Type | Default |
|----|------|------|---------|
| 10005 | Selection Map | Link | — |
| 10010 | Texture Source | Int32 | 10011 | 10011=Shader, 10012=Texture Tag |
| 10004 | Shader | Link | — |
| 10006 | Texture Tag | Link | — |
| 10069 | Texture Channel | Int32 | 0 | Same channel values as mtSubDivider |
| 10009 | Tolerance | Float | — |

### Example
```python
import c4d

doc = c4d.documents.GetActiveDocument()
shell = c4d.BaseObject(1057182)  # mtShellGen
shell[10002] = 5.0   # 5cm thickness
shell[10001] = True   # create caps
shell[10020] = True   # enable bevel
shell[10027] = 2.0    # bevel offset
shell[10022] = 3      # bevel subdivisions

cube = c4d.BaseObject(c4d.Ocube)
cube.InsertUnder(shell)
doc.InsertObject(shell)
c4d.EventAdd()
```

---

## mtSelect (1057238)

Creates polygon/edge/point selections from texture or selection maps. Useful for feeding selections into other MeshTools generators.

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10032 | Scale | Float | 1.0 | Selection scale |
| 10018 | Hide Generated | Bool | False | |
| 10019 | Hide Non-Generated | Bool | False | |

### Selection Source
| ID | Name | Type | Default |
|----|------|------|---------|
| 10005 | Selection Map | Link | — |
| 10008 | Texture Source | Int32 | 10009 | 10009=Shader, 10010=Texture Tag |
| 10006 | Shader | Link | — |
| 10007 | Texture Tag | Link | — |
| 10033 | Texture Channel | Int32 | 0 | |
| 10011 | Tolerance | Float | 0.5 | |

### Selection Output
| ID | Name | Type |
|----|------|------|
| 10021 | Polygon Selection | Bool |
| 10023 | Edge Selection | Bool |
| 10025 | Point Selection | Bool |

---

## mtPolyScale (1058738)

Scales individual polygons along their normals.

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10006 | Amount | Float | 0.0 | Scale amount |
| 10007 | Variation | Float | 0.0 | Random variation |
| 10008 | Seed | Int32 | 5543 | Random seed |
| 10014 | Hide Scaled | Bool | False | |
| 10015 | Hide Non-Scaled | Bool | False | |

### Selection Source
| ID | Name | Type | Default |
|----|------|------|---------|
| 10000 | Selection Map | Link | — |
| 10003 | Texture Source | Int32 | 0 | 0=Shader, 1=Texture Tag |
| 10001 | Shader | Link | — |
| 10002 | Texture Tag | Link | — |
| 10004 | Texture Channel | Int32 | 0 | |
| 10005 | Tolerance | Float | — |

### Example
```python
import c4d

doc = c4d.documents.GetActiveDocument()
ps = c4d.BaseObject(1058738)  # mtPolyScale
ps[10006] = -0.3  # scale inward

cube = c4d.BaseObject(c4d.Ocube)
cube.InsertUnder(ps)
doc.InsertObject(ps)
c4d.EventAdd()
```

---

## mtPolyFold (1014442213)

Folds/unfolds polygons along edges, creating origami-like effects.

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10026 | Direction | Int32 | — | 0=Clockwise, 1=Anti-Clockwise, 2=Random, 3=Axis, 4=Center |
| 10076 | Axis | Int32 | — | 0=+X, 1=+Y, 2=+Z, 3=-X, 4=-Y, 5=-Z |
| 10108 | Threshold | Float | 0.1745 | ~10° in radians |
| 10019 | Unfold | Float | 0.5 | 0=fully folded, 1=fully unfolded |
| 10109 | Variation | Float | — | |
| 10075 | Remove Offset | Float | 0.1 | |
| 10002 | Point Count | Int32 | 1 | |
| 10003 | Seed | Int32 | — | |
| 10074 | Remove Polygons | Bool | — | |
| 10004 | Fill Unused Polygons | Bool | — | |
| 10013 | Limit | Bool | — | |
| 10015 | Limit Angle | Float | — | |

### Distribution
| ID | Name | Type | Default |
|----|------|------|---------|
| 10007 | Distribution Type | Int32 | — | 0=Random, 1=Selection, 2=Texture |
| 10006 | Selection Map | Link | — |
| 10011 | Texture Source | Int32 | — | 0=Texture Tag, 1=Shader |
| 10008 | Shader | Link | — |
| 10009 | Texture Tag | Link | — |
| 10012 | Texture Channel | Int32 | — | |
| 10010 | Tolerance | Float | — |

### Rotation
| ID | Name | Type | Default |
|----|------|------|---------|
| 10072 | Rotation | Int32 | — | 0=Angle, 1=Percentage |
| 10071 | Angle Percent | Float | 0.32 | |
| 10021 | Angle | Float | ~32° | In radians |

### Scaling
| ID | Name | Type | Default |
|----|------|------|---------|
| 10079 | Scaling | Int32 | — | 0=None, 2=Spline |
| 10077 | Scale Value | Float | — | |
| 10027 | Scale | SplineData | — | |

### Display
| ID | Name | Type | Default |
|----|------|------|---------|
| 10091 | Start Points | Bool | — |
| 10092 | Color | Vector | (0.454, 0.675, 0.263) |
| 10093 | Size | Float | 3.0 |

---

## mtPointProject (102999653)

Projects child object points onto target surfaces.

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10007 | Surfaces | Link List | — | Target surfaces to project onto |
| 10002 | Mode | Int32 | 1 | 0=Parallel +X, 1=Parallel +Y, 2=Parallel +Z, 3=Parallel -X, 4=Parallel -Y, 5=Parallel -Z, 6=Spherical |
| 10003 | Offset | Float | 0.0 | Distance offset from surface |
| 10004 | Blend | Float | 1.0 | Projection blend (0=original, 1=fully projected) |
| 10029 | Distance Falloff | Bool | — | |
| 10030 | Distance | Float | — | Falloff distance |
| 10031 | Blend | Float | — | Falloff blend |
| 10032 | Falloff | SplineData | — | Falloff spline curve |

### Selection Source
| ID | Name | Type | Default |
|----|------|------|---------|
| 10020 | Selection Map | Link | — |
| 10021 | Texture Source | Int32 | 0 | 0=Shader, 1=Texture Tag |
| 10025 | Shader | Link | — |
| 10026 | Texture Tag | Link | — |
| 10027 | Texture Channel | Int32 | — | |
| 10028 | Tolerance | Float | — |

### Example
```python
import c4d

doc = c4d.documents.GetActiveDocument()
proj = c4d.BaseObject(102999653)  # mtPointProject
proj[10002] = 1    # Parallel +Y (project downward)
proj[10004] = 1.0  # full projection

# Child mesh gets projected onto surfaces in the Surfaces list
plane = c4d.BaseObject(c4d.Oplane)
plane.InsertUnder(proj)
doc.InsertObject(proj)
c4d.EventAdd()
```

---

## mtInset (1057181)

Insets polygons (creates inner polygon faces with connecting geometry).

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10006 | Inset Mode | Int32 | 10007 | 10007=Face Percentage, 10008=Fixed Length |
| 10137 | Sampling Mode | Int32 | 0 | 0=Per-Point, 1=Centered |
| 10133 | Divisions | Int32 | 1 | Number of inset divisions |
| 10136 | Shape | SplineData | — | Inset profile shape |
| 10009 | Amount | Float | 0.0 | Inset amount |
| 10010 | Amount | Float | — | Secondary amount (context-dependent) |
| 10100 | Variation | Float | 0.0 | Inset variation |
| 10101 | Offset | Float | 0.0 | Normal offset |
| 10102 | Variation | Float | 0.0 | Offset variation |
| 10134 | Twist | Float | 0.0 | Twist angle |
| 10135 | Variation | Float | 0.0 | Twist variation |
| 10103 | Seed | Int32 | 455 | Random seed |
| 10139 | Respect Ngon | Bool | True | |
| 10104 | Hide Inner Faces | Bool | False | |
| 10105 | Hide Outer Faces | Bool | False | |

### Falloff Options
| ID | Name | Type | Default |
|----|------|------|---------|
| 10117 | Inset Falloff | Bool | True |
| 10118 | Offset Falloff | Bool | True |
| 10138 | Twist Falloff | Bool | True |

### Selection Source
| ID | Name | Type | Default |
|----|------|------|---------|
| 10000 | Selection Map | Link | — |
| 10003 | Texture Source | Int32 | 10004 | 10004=Shader, 10005=Texture Tag |
| 10001 | Shader | Link | — |
| 10002 | Texture Tag | Link | — |
| 10132 | Texture Channel | Int32 | 0 | |
| 10131 | Tolerance | Float | 0.5 | |

### Example
```python
import c4d

doc = c4d.documents.GetActiveDocument()
inset = c4d.BaseObject(1057181)  # mtInset
inset[10006] = 10007  # Face Percentage mode
inset[10009] = 0.3    # 30% inset
inset[10101] = 5.0    # 5cm normal offset (extrude-like)
inset[10133] = 3      # 3 divisions

cube = c4d.BaseObject(c4d.Ocube)
cube.InsertUnder(inset)
doc.InsertObject(inset)
c4d.EventAdd()
```

---

## mtDualGraph (1057327)

Creates a dual graph of the mesh topology — replaces faces with vertices and vertices with faces.

### Parameters
| ID | Name | Type | Default | Notes |
|----|------|------|---------|-------|
| 10000 | Dual Graph | Int32 | — | 0=Disabled, 1=Triangles, 2=Ngons |
| 10001 | Triangulation | Int32 | — | 0=Quads, 1=Quads/Tris |
| 10025 | Fill Holes | Bool | True | Fill holes in dual graph |
| 10017 | Hide Generated | Bool | False | |
| 10018 | Hide Non-Generated | Bool | False | |

### Selection Source
| ID | Name | Type | Default |
|----|------|------|---------|
| 10002 | Selection Map | Link | — |
| 10005 | Texture Source | Int32 | 0 | 0=Shader, 1=Texture Tag |
| 10003 | Shader | Link | — |
| 10004 | Texture Tag | Link | — |
| 10010 | Texture Channel | Int32 | 0 | |
| 10006 | Tolerance | Float | 0.5 | |

### Example
```python
import c4d

doc = c4d.documents.GetActiveDocument()
dg = c4d.BaseObject(1057327)  # mtDualGraph
dg[10000] = 2     # Ngons mode
dg[10025] = True  # fill holes

cube = c4d.BaseObject(c4d.Ocube)
cube.InsertUnder(dg)
doc.InsertObject(dg)
c4d.EventAdd()
```

---

## Chaining MeshTools Generators

MeshTools generators can be nested to create complex procedural effects:

```python
import c4d

doc = c4d.documents.GetActiveDocument()

# Inset → Shell chain: inset faces then add thickness
shell = c4d.BaseObject(1057182)  # mtShellGen
shell[10002] = 2.0  # thickness

inset = c4d.BaseObject(1057181)  # mtInset
inset[10006] = 10007  # Face Percentage
inset[10009] = 0.2    # 20% inset

cube = c4d.BaseObject(c4d.Ocube)
cube[c4d.PRIM_CUBE_SUBX] = 3
cube[c4d.PRIM_CUBE_SUBY] = 3
cube[c4d.PRIM_CUBE_SUBZ] = 3

cube.InsertUnder(inset)
inset.InsertUnder(shell)
doc.InsertObject(shell)
c4d.EventAdd()
```

## Quick Reference — Object IDs

| Generator | Type ID |
|-----------|---------|
| mtSubDivider | 1057183 |
| mtShellGen | 1057182 |
| mtSelect | 1057238 |
| mtPolyScale | 1058738 |
| mtPolyFold | 1014442213 |
| mtPointProject | 102999653 |
| mtInset | 1057181 |
| mtDualGraph | 1057327 |
