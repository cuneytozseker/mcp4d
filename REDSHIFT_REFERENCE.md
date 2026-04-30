# Redshift for C4D — Quick Reference

## Key Difference from Octane
Redshift materials in C4D 2025+ use the **Node Material system** (base type 5703) with the Redshift node space.
You do NOT create materials with a custom material type ID — instead create a standard `c4d.BaseMaterial(5703)` and add a Redshift graph to it.
Access node parameters via `maxon` graph API (BeginTransaction → FindChild → SetPortValue → Commit).

**Required imports:**
```python
import c4d, maxon, redshift
```

## Plugin / Object IDs
```python
# From the `redshift` module
Orslight          = 1036751   # RS Light object
Orsenvironment    = 1036757   # RS Environment object
Orssky            = 1036754   # RS Sun & Sky object
Orscloud          = 1065365   # RS Clouds object
Orsproxy          = 1038649   # RS Proxy object
Orsvolume         = 1038655   # RS Volume object

Trscamera         = 1036760   # RS Camera tag
Trsobject         = 1036222   # RS Object tag

VPrsrenderer      = 1036219   # RS VideoPost (render engine)
VPrsposteffects   = 1040189   # RS Post Effects VideoPost

Mrsmaterial       = 1036224   # RS Shader Graph material (legacy ID)
Mrsgraph          = 1036224   # Same as above

GVrsshader        = 1036227   # RS Shader node (Xpresso)
GVrsoutput        = 1036746   # RS Output node (Xpresso)

Frsproxyexport    = 1038650   # RS Proxy export filter
Frsproxyimport    = 1038651   # RS Proxy import filter
```

## Material Preset Type IDs (for FilterPluginList)
```python
# These are registered material plugin types, NOT for BaseMaterial() creation
ID_RS_STANDARD     = 1067447   # Redshift - Standard
ID_RS_OPENPBR      = 1067448   # Redshift - OpenPBR
ID_RS_TOON         = 1067449   # Redshift - Toon
ID_RS_SHADER_GRAPH = 1036224   # RS Shader Graph
```

## Node Space
```python
RS_NODESPACE = maxon.Id("com.redshift3d.redshift4c4d.class.nodespace")
# Also available as: maxon.REDSHIFT_NODESPACE (InternedId — must str() before passing to maxon.Id)
```

## Creating Redshift Materials

**IMPORTANT:** Always create with `c4d.BaseMaterial(5703)` (standard material), then add a Redshift graph.
Do NOT use `c4d.BaseMaterial(1067447)` — it creates a material with no usable node space.

```python
import c4d, maxon

doc = c4d.documents.GetActiveDocument()
RS_SPACE = maxon.Id("com.redshift3d.redshift4c4d.class.nodespace")

mat = c4d.BaseMaterial(5703)
mat.SetName("My_RS_Material")
doc.InsertMaterial(mat)

nm = mat.GetNodeMaterialReference()
graph = nm.CreateDefaultGraph(RS_SPACE)
# This creates a graph with: RS Standard Material node → Output node

c4d.EventAdd()
```

## Accessing the Standard Material Node
```python
RS_SPACE = maxon.Id("com.redshift3d.redshift4c4d.class.nodespace")
graph = nm.GetGraph(RS_SPACE)
root = graph.GetViewRoot()  # Use GetViewRoot, NOT GetRoot (deprecated)
children = []
root.GetChildren(children)

rs_mat_node = None
for c in children:
    try:
        aid = c.GetValue("net.maxon.node.attribute.assetid")
        if "standardmaterial" in str(aid):
            rs_mat_node = c
            break
    except:
        pass
```

## Setting Material Parameters
```python
PREFIX = "com.redshift3d.redshift4c4d.nodes.core.standardmaterial"

with graph.BeginTransaction() as transaction:
    port = rs_mat_node.GetInputs().FindChild(f"{PREFIX}.base_color")
    port.SetPortValue(maxon.ColorA64(0.9, 0.2, 0.2, 1.0))  # Red

    port = rs_mat_node.GetInputs().FindChild(f"{PREFIX}.refl_roughness")
    port.SetPortValue(0.15)

    transaction.Commit()

c4d.EventAdd()
```

## RS Standard Material Port Reference

All ports use prefix: `com.redshift3d.redshift4c4d.nodes.core.standardmaterial`

### Diffuse / Base
```
base_color           ColorA64    Base color (default: 0.5, 0.5, 0.5)
base_color_weight    float       Diffuse weight (0-1, default: 1)
diffuse_model        int         Diffuse model (default: 0)
diffuse_roughness    float       Diffuse roughness (default: 0)
metalness            float       Metallic (0=dielectric, 1=metal, default: 0)
```

### Reflection
```
refl_color           ColorA64    Reflection color (default: white)
refl_weight          float       Reflection weight (0-1, default: 1)
refl_roughness       float       Reflection roughness (0=mirror, 1=matte, default: 0.2)
refl_ior             float       Index of refraction (default: 1.5)
refl_aniso           float       Anisotropy (default: 0)
refl_aniso_rotation  float       Anisotropy rotation (default: 0)
refl_samples         int         Reflection samples (default: 16)
```

### Refraction (Transmission)
```
refr_color           ColorA64    Refraction color (default: white)
refr_weight          float       Refraction weight (0=opaque, 1=full glass, default: 0)
refr_roughness       float       Refraction roughness (0=clear, default: 0)
refr_samples         int         Refraction samples (default: 8)
refr_thin_walled     bool        Thin-walled mode (default: false)
refr_abbe            float       Dispersion Abbe number (0=off, default: 0)
```

### Subsurface Scattering
```
ss_depth             float       SSS depth (0=off, default: 0)
ss_scatter_color     ColorA64    Scatter color (default: black)
ss_phase             float       Phase (-1 to 1, default: 0)
ss_samples           int         SSS samples (default: 16)
```

### Multiple Scattering (SSS)
```
ms_color             ColorA64    Color (default: white)
ms_amount            float       Amount (0=off, default: 0)
ms_radius            ColorA64    Radius per channel (default: white)
ms_radius_scale      float       Radius multiplier (default: 1)
ms_phase             float       Phase (default: 0)
ms_mode              int         Mode (default: 2)
ms_samples           int         Samples (default: 64)
```

### Sheen
```
sheen_color          ColorA64    Sheen color (default: white)
sheen_weight         float       Sheen weight (0=off, default: 0)
sheen_roughness      float       Sheen roughness (default: 0.3)
```

### Thin Film
```
thinfilm_ior         float       Thin film IOR (default: 1.5)
thinfilm_thickness   float       Thickness (0=off, default: 0)
```

### Coat
```
coat_color           ColorA64    Coat color (default: white)
coat_weight          float       Coat weight (0=off, default: 0)
coat_roughness       float       Coat roughness (default: 0)
coat_ior             float       Coat IOR (default: 1.5)
coat_bump_input      link        Coat bump (default: None)
```

### Emission
```
emission_color       ColorA64    Emission color (default: white)
emission_weight      float       Emission weight (0=off, default: 0)
```

### Opacity / Overall
```
opacity_color        ColorA64    Opacity (default: white = fully opaque)
overall_color        ColorA64    Overall tint (default: white)
bump_input           link        Bump map input (default: None)
```

### Ray Depth / Advanced
```
depth_override           bool    Override ray depth (default: false)
refl_depth               int     Reflection depth (default: 3)
refr_depth               int     Refraction depth (default: 5)
combined_depth           int     Combined depth (default: 6)
shadow_opacity           float   Shadow opacity (default: 0)
shadow_affected_by_fresnel_mode  int  (default: 2)
refl_isglossiness        bool    Interpret as glossiness (default: false)
refr_isglossiness        bool    Interpret as glossiness (default: false)
refl_enablemultiscattercomp  bool  Multi-scatter GGX (default: true)
```

### Node Asset IDs
```
Standard Material: com.redshift3d.redshift4c4d.nodes.core.standardmaterial
Output:            com.redshift3d.redshift4c4d.node.output
```

## RS Light Object

Create with `c4d.BaseObject(redshift.Orslight)` (or `c4d.BaseObject(1036751)`).
C4D area lights are auto-converted via command `1057470` (Convert All Lights to Redshift).

### Light Type (`[10000]`)
```
0 = Infinite (directional)
1 = Point
2 = Spot
3 = Area
4 = Dome (environment/HDRI)
5 = Photometric IES
6 = Portal
7 = Physical Sun
```

### Area Shape (`[11010]`)
```
0 = Rectangle
1 = Disc
2 = Sphere
3 = Cylinder
4 = Mesh
```

### Key Parameter IDs
```python
# Intensity
light[10000]  # Type (see enum above)
light[11004]  # Intensity (float, default varies by type)
light[11022]  # Exposure EV (float)
light[11005]  # Units

# Color
light[11002]  # Color mode
light[11000]  # Color (Vector)
light[11003]  # Temperature in Kelvin (float)

# Shape (Area lights)
light[11010]  # Area shape (see enum above)
light[11016]  # Size X
light[11017]  # Size Y
light[11018]  # Size Z
light[11023]  # Spread
light[11013]  # Bi-Directional (bool)
light[11014]  # Normalize Intensity (bool)

# Shadow
light[10009]  # Cast Shadow (bool)
light[10010]  # Shadow Transparency
light[10011]  # Shadow Softness
light[10012]  # Shadow Samples

# Dome Light HDRI (type=4)
light[12024]  # Background intensity
light[12000]  # Background texture
light[12001]  # Texture type
light[12002]  # Gamma
light[12013]  # Flip Horizontal
light[12026]  # Rotate on Horizon

# Visibility / Contribution
light[10042]  # Camera visibility
light[10005]  # Diffuse contribution
light[10034]  # Reflection contribution
light[10007]  # Volume contribution

# Caustics
light[10039]  # Cast Caustics (bool)
light[10018]  # Caustic Intensity
```

## RS Environment Object

Create with `c4d.BaseObject(redshift.Orsenvironment)` (or `c4d.BaseObject(1036757)`).

```python
env = c4d.BaseObject(1036757)
env.SetName("RS_Environment")
doc.InsertObject(env)

# Volume Scattering
env[11001]  # Scattering (1/m)
env[11017]  # Viewing Distance (m)
env[11003]  # Anisotropy

# Fog
env[11004]  # Emission
env[11006]  # Height
env[11007]  # Horizon Blur

# Contribution
env[11011]  # Camera
env[11012]  # Reflection
env[11014]  # GI
```

## RS Camera Tag

Create with `c4d.BaseTag(redshift.Trscamera)` (or `c4d.BaseTag(1036760)`).

```python
tag = cam.MakeTag(1036760)  # RS Camera tag

# Exposure (override)
tag[10011]  # Override enabled (bool)
tag[10000]  # Exposure enabled (bool)
tag[10001]  # Film Speed ISO
tag[10002]  # Shutter Time Ratio
tag[10003]  # f-Stop Number
tag[10004]  # Whitepoint

# Tone Mapping
tag[10006]  # Allowed Overexposure
tag[10009]  # Saturation

# Bokeh DOF (override)
tag[11011]  # Override (bool)
tag[11000]  # DOF Enabled (bool)
tag[11001]  # Derive focus from camera (bool)
tag[11002]  # Focus Distance
tag[11003]  # CoC Radius
tag[11004]  # Power

# Bloom
tag[12109]  # Override (bool)
tag[12100]  # Enabled (bool)
tag[12101]  # Threshold
tag[12102]  # Softness
tag[12103]  # Intensity
```

## RS Object Tag

Create with `c4d.BaseTag(redshift.Trsobject)` (or `c4d.BaseTag(1036222)`).
Used for object-level overrides: visibility, tessellation, displacement, matte/shadow catcher.

## Commands
```python
# Material / Conversion
c4d.CallCommand(1057470)   # Convert All Lights to Redshift
c4d.CallCommand(1057469)   # Convert Selected Lights to Redshift
c4d.CallCommand(1057471)   # Convert All Cameras to Redshift
c4d.CallCommand(1057472)   # Convert Selected Cameras to Redshift

# Tools / Windows
c4d.CallCommand(1036221)   # RS IPR (live viewer)
c4d.CallCommand(1038666)   # RS RenderView
c4d.CallCommand(1036229)   # RS Shader Graph Editor
c4d.CallCommand(1038693)   # RS AOV Manager
c4d.CallCommand(1040254)   # Redshift Material Presets
```

## Ensuring Redshift Renderer is Active
```python
import redshift

rd = doc.GetActiveRenderData()
# FindAddVideoPost adds RS VideoPost if not present, returns it either way
rs_vp = redshift.FindAddVideoPost(rd, redshift.VPrsrenderer)
```

## Common Material Recipes

### Glossy Plastic
```python
with graph.BeginTransaction() as t:
    set_port(node, "base_color", maxon.ColorA64(0.8, 0.2, 0.2, 1.0))
    set_port(node, "refl_roughness", 0.15)
    set_port(node, "metalness", 0.0)
    t.Commit()
```

### Brushed Metal
```python
with graph.BeginTransaction() as t:
    set_port(node, "base_color", maxon.ColorA64(0.85, 0.85, 0.85, 1.0))
    set_port(node, "metalness", 1.0)
    set_port(node, "refl_roughness", 0.2)
    set_port(node, "refl_aniso", 0.5)
    t.Commit()
```

### Clear Glass
```python
with graph.BeginTransaction() as t:
    set_port(node, "base_color_weight", 0.0)
    set_port(node, "refl_weight", 1.0)
    set_port(node, "refl_roughness", 0.0)
    set_port(node, "refr_weight", 1.0)
    set_port(node, "refr_color", maxon.ColorA64(0.98, 0.98, 1.0, 1.0))
    set_port(node, "refl_ior", 1.5)
    t.Commit()
```

### Frosted Glass
```python
with graph.BeginTransaction() as t:
    set_port(node, "base_color_weight", 0.0)
    set_port(node, "refl_weight", 1.0)
    set_port(node, "refl_roughness", 0.05)
    set_port(node, "refr_weight", 1.0)
    set_port(node, "refr_roughness", 0.15)
    set_port(node, "refl_ior", 1.5)
    t.Commit()
```

### White Matte (Cyclorama)
```python
with graph.BeginTransaction() as t:
    set_port(node, "base_color", maxon.ColorA64(0.95, 0.95, 0.95, 1.0))
    set_port(node, "refl_roughness", 0.4)
    set_port(node, "refl_weight", 0.3)
    t.Commit()
```

### Emissive (Light Panel)
```python
with graph.BeginTransaction() as t:
    set_port(node, "emission_weight", 1.0)
    set_port(node, "emission_color", maxon.ColorA64(1.0, 0.95, 0.85, 1.0))
    t.Commit()
```

## Helper: Create + Configure RS Material
```python
import c4d, maxon

RS_SPACE = maxon.Id("com.redshift3d.redshift4c4d.class.nodespace")
PREFIX = "com.redshift3d.redshift4c4d.nodes.core.standardmaterial"

def create_rs_material(doc, name):
    """Create an RS Standard Material. Returns (material, graph, rs_node)."""
    mat = c4d.BaseMaterial(5703)
    mat.SetName(name)
    doc.InsertMaterial(mat)
    nm = mat.GetNodeMaterialReference()
    graph = nm.CreateDefaultGraph(RS_SPACE)

    root = graph.GetViewRoot()
    children = []
    root.GetChildren(children)
    rs_node = None
    for c in children:
        try:
            aid = c.GetValue("net.maxon.node.attribute.assetid")
            if "standardmaterial" in str(aid):
                rs_node = c
                break
        except:
            pass
    return mat, graph, rs_node

def set_port(rs_node, port_name, value):
    """Set a port value on the RS Standard Material node. Call inside a transaction."""
    port = rs_node.GetInputs().FindChild(f"{PREFIX}.{port_name}")
    if port:
        port.SetPortValue(value)
        return True
    return False
```

## Known Limitations
- **`c4d.BaseMaterial(1067447)` does NOT work** — creates a material with no node space. Always use `c4d.BaseMaterial(5703)` + `CreateDefaultGraph(RS_SPACE)`.
- **`GetRoot()` is deprecated** since 2025.0 — use `GetViewRoot()` instead.
- **`GetDefaultValue()` is deprecated** since 2024.4 — use `GetPortValue()` instead.
- **Color values** use `maxon.ColorA64(r, g, b, a)` with alpha, not `c4d.Vector`.
- **All graph edits must be inside a transaction** (`graph.BeginTransaction()` → `transaction.Commit()`).
- **Call `c4d.EventAdd()`** after material changes for the viewport to update.
