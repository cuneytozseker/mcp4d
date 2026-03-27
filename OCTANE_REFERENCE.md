# Octane Render for C4D — Quick Reference

## Key Limitation
Octane materials use the old layer/shader system, NOT C4D's new Node Material graph.
You can modify materials at the material level via Python, but cannot interact with Octane's node editor selections.
Trick to discover IDs: drag any Octane node into C4D's Script Manager console, append `.GetType()` and hit enter.

## Plugin IDs
```
# Materials
# NOTE: In Octane 2024+, Universal material uses ID 1029501 (was 1029750 in older versions)
# Always verify with: create material manually, then read mat.GetType()
ID_OCTANE_UNIVERSAL_MATERIAL = 1029501  # Universal (use this for everything)
ID_OCTANE_GLOSSY_MATERIAL   = 1029502   # Glossy (reflective)
ID_OCTANE_SPECULAR_MATERIAL = 1029503   # Specular (glass/transparent)
ID_OCTANE_MIX_MATERIAL      = 1029504   # Mix two materials
ID_OCTANE_PORTAL_MATERIAL   = 1029505   # Portal

# Textures/Shaders
ID_OCTANE_IMAGE_TEXTURE     = 1029508   # Image/Bitmap texture
ID_OCTANE_FLOAT_TEXTURE     = 1029509   # Float value
ID_OCTANE_RGB_SPECTRUM      = 1029510   # RGB color
ID_OCTANE_GAUSSIAN_SPECTRUM = 1029511   # Gaussian spectrum
ID_OCTANE_COLORCORRECTION   = 1029512   # Color correction
ID_OCTANE_INVERT_TEXTURE    = 1029514   # Invert
ID_OCTANE_MULTIPLY_TEXTURE  = 1029516   # Multiply
ID_OCTANE_MIXTEXTURE        = 1029505   # Mix texture
ID_OCTANE_NOISE             = 1029530   # Noise texture
ID_OCTANE_CHECKS            = 1029506   # Checker texture
ID_OCTANE_DIRT              = 1029531   # Dirt/AO texture

# Tags
ID_OCTANE_OBJECTTAG         = 1029524   # Octane Object tag
ID_OCTANE_CAMERATAG         = 1029528   # Octane Camera tag
ID_OCTANE_LIGHTTAG          = 1029525   # Octane Light tag (emission)

# Environment
ID_OCTANE_ENVIRONMENT       = 1029526   # Octane Environment tag
ID_OCTANE_DAYLIGHT          = 1029540   # Octane Daylight environment

# Render
ID_OCTANE_VIDEOPOST         = 1029527   # Octane render engine VideoPost
ID_OCTANE_LIVEVIWER         = 1029529   # Live Viewer
```

## Creating Octane Materials
```python
# Universal material (preferred for all use cases)
mat = c4d.BaseMaterial(1029501)
mat.SetName("Mat_Steel")
doc.InsertMaterial(mat)

# Assign to object
tag = obj.MakeTag(c4d.Ttexture)
tag[c4d.TEXTURETAG_MATERIAL] = mat
```

## Universal Material Channels (OCT_MAT_*)
```
# Albedo/Diffuse
OCT_MATERIAL_DIFFUSE_LINK          # Albedo texture slot
OCT_MATERIAL_DIFFUSE_COLOR         # Albedo color (Vector RGB 0-1)
OCT_MATERIAL_DIFFUSE_FLOAT         # Albedo float

# Specular/Reflection
OCT_MAT_SPECULAR_MAP_LINK          # Specular/metallic texture slot
OCT_MAT_SPECULAR_MAP_FLOAT         # Specular float (0-1)

# Roughness
OCT_MATERIAL_ROUGHNESS_LINK        # Roughness texture slot
OCT_MATERIAL_ROUGHNESS_FLOAT       # Roughness float (0=mirror, 1=matte)

# Bump/Normal
OCT_MATERIAL_BUMP_LINK             # Bump texture slot
OCT_MATERIAL_BUMP_FLOAT            # Bump strength
OCT_MATERIAL_NORMAL_LINK           # Normal map slot

# Opacity
OCT_MATERIAL_OPACITY_LINK          # Opacity texture slot
OCT_MATERIAL_OPACITY_FLOAT         # Opacity float

# Emission
OCT_MATERIAL_EMISSION              # Emission texture/node
OCT_MATERIAL_EMISSION_LINK         # Emission link

# Displacement
OCT_MATERIAL_DISPLACEMENT_LINK     # Displacement texture slot
OCT_MATERIAL_DISPLACEMENT_FLOAT    # Displacement amount

# Transmission
OCT_MATERIAL_TRANSMISSION_LINK     # Transmission texture
OCT_MATERIAL_TRANSMISSION_FLOAT    # Transmission amount

# Index of Refraction
OCT_MATERIAL_INDEX                 # IOR value (glass=1.5, water=1.33, diamond=2.42)

# Coating
OCT_MATERIAL_COATING_FLOAT         # Coating amount
OCT_MATERIAL_COATING_ROUGHNESS     # Coating roughness
OCT_MATERIAL_COATING_IOR           # Coating IOR
```

## Image Texture Properties
```python
tex = c4d.BaseShader(1029508)       # ID_OCTANE_IMAGE_TEXTURE
tex[c4d.IMAGETEXTURE_FILE] = "C:\\textures\\diffuse.png"
tex[c4d.IMAGETEXTURE_GAMMA] = 2.2   # sRGB=2.2, linear/data=1.0
tex[c4d.IMAGETEXTURE_MODE] = 0      # 0=normal
tex[c4d.IMAGETEX_BORDER_MODE] = 0
mat.InsertShader(tex)
mat[c4d.OCT_MATERIAL_DIFFUSE_LINK] = tex
```

## Octane Object Tag
```python
tag = obj.MakeTag(1029524)  # Octane Object tag
# Common properties:
# OBJECTTAG_VISIBILITY (camera visibility)
# Used for: custom object ID, shadow visibility, Octane-specific object properties
```

## Octane Camera Tag
```python
tag = cam.MakeTag(1029528)  # Octane Camera tag
# Properties: DOF, aperture, bokeh, stereo, post-processing
# The C4D camera handles position/rotation/focal length
# The Octane tag adds render-specific properties
```

## Octane Light Tag (Emission)
```python
tag = light.MakeTag(1029525)  # Octane Light tag
# Or use emission channel on any material to make mesh lights
```

## Octane Environment
```python
# HDRI environment
env = c4d.BaseObject(1029526)       # Octane Environment
# Set HDRI texture on environment's texture slot
# Adjust rotation, intensity

# Daylight/Sun+Sky
sky = c4d.BaseObject(1029540)       # Octane Daylight
# Turbidity, sun direction, ground color
```

## Octane Render Settings
```python
# Access via VideoPost
rd = doc.GetActiveRenderData()
vp = rd.GetFirstVideoPost()
while vp:
    if vp.GetType() == 1029527:     # Octane VideoPost
        # Kernel settings
        # vp[OCTANE_KERNEL_TYPE]    # Path Tracing, Direct Light, PMC
        # vp[OCTANE_MAX_SAMPLES]    # Max samples per pixel
        # vp[OCTANE_DENOISE_ENABLED]
        break
    vp = vp.GetNext()
```

## Common Material Recipes
```python
# Brushed metal
mat = c4d.BaseMaterial(1029501)
mat[c4d.OCT_MAT_SPECULAR_MAP_FLOAT] = 1.0  # Full metallic
mat[c4d.OCT_MATERIAL_ROUGHNESS_FLOAT] = 0.15
# Set IOR to conductor mode + metal IOR values

# Glossy plastic
mat = c4d.BaseMaterial(1029501)
mat[c4d.OCT_MATERIAL_DIFFUSE_COLOR] = c4d.Vector(0.8, 0.2, 0.2)  # Red
mat[c4d.OCT_MAT_SPECULAR_MAP_FLOAT] = 0.0  # Non-metallic
mat[c4d.OCT_MATERIAL_ROUGHNESS_FLOAT] = 0.05

# Glass
mat = c4d.BaseMaterial(1029503)  # Specular material better for glass
mat[c4d.OCT_MATERIAL_INDEX] = 1.5
# Or use Universal with transmission

# Matte/Diffuse
mat = c4d.BaseMaterial(1029501)
mat[c4d.OCT_MATERIAL_DIFFUSE_COLOR] = c4d.Vector(0.5, 0.5, 0.5)
mat[c4d.OCT_MAT_SPECULAR_MAP_FLOAT] = 0.0
mat[c4d.OCT_MATERIAL_ROUGHNESS_FLOAT] = 0.5

# Shadow catcher (for compositing)
# Use Octane Diffuse material with shadow catcher enabled in Octane Object tag
```

## OSL (Open Shading Language)
Octane supports OSL for custom textures, displacement, cameras, and pattern generation.
OSL scripts are C-like code that runs on the GPU at render time.
CC can write OSL code and inject it into materials programmatically.

```python
# OSL Texture node types
ID_OCTANE_OSL_TEXTURE     = 1033573   # OSL Texture (general purpose)
ID_OCTANE_OSL_CAMERA      = 1033574   # OSL Camera projection
ID_OCTANE_OSL_PROJECTION  = 1033575   # OSL Projection
ID_OCTANE_OSL_DISPLACEMENT = 1033576  # OSL Displacement

# Create an OSL texture and connect to material
osl = c4d.BaseShader(1033573)
# Set OSL code via the shader's container
# osl[c4d.OSL_CODE] = osl_source_string  # exact ID needs discovery
mat.InsertShader(osl)
mat[c4d.OCT_MATERIAL_DIFFUSE_LINK] = osl
```

**OSL script structure (minimal):**
```osl
shader MyTexture(
    color BaseColor = color(0.8, 0.2, 0.1),
    float Scale = 5.0,
    output color c = 0
)
{
    point p = P * Scale;
    float n = noise("perlin", p);
    c = mix(BaseColor, color(1), n);
}
```

**OSL use cases for industrial animation:**
- Procedural wear/scratch patterns on metal surfaces
- Custom UV projection for labels on curved surfaces
- Parametric patterns (grids, hex, carbon fiber) driven by float inputs
- Edge detection / curvature-based effects
- Custom camera lens distortion

**Key facts:**
- OSL scripts saved at: `C4D plugins\c4doctane\res\osl_scripts\`
- Custom Pattern nodes in Octane are pre-built OSL scripts exposed as native nodes
- OSL compiles at render time, no pre-compilation needed
- Only ONE output per OSL script in Octane (limitation vs spec)
- Octane targets OSL spec v1.9.13 compatibility
- OSL runs on GPU, badly written code can crash Octane

**OSL docs:**
https://docs.otoy.com/cinema4d/OpenShadingLanguageOSL.html
https://docs.otoy.com/cinema4d/OSLOverview.html
OSL spec: https://github.com/AcademySoftwareFoundation/OpenShadingLanguage

## DunHouGo/renderEngine Wrapper
If the boghma Renderer library is available, it provides cleaner access:
```python
from Renderer import Octane
# MaterialHelper, AOVHelper, SceneHelper classes
# See: https://github.com/DunHouGo/renderEngine
```

## Viewport Preview (for CC feedback loop)
Octane's Live Viewer buffer is NOT accessible from Python/C++.
For visual verification, use a low-sample Octane render instead of hardware preview:
```python
rd = doc.GetActiveRenderData()
# Octane should already be the active engine — don't switch it
# Temporarily override samples for a fast preview
vp = rd.GetFirstVideoPost()
while vp:
    if vp.GetType() == 1029527:  # Octane VideoPost
        # Store originals, set low samples
        # vp[OCTANE_MAX_SAMPLES] = 64
        # Enable AI denoiser
        break
    vp = vp.GetNext()
bmp = c4d.bitmaps.MultipassBitmap(800, 600, c4d.COLORMODE_RGB)
bmp.AddChannel(True, True)
c4d.documents.RenderDocument(doc, rd.GetData(), bmp, c4d.RENDERFLAGS_EXTERNAL)
bmp.Save("C:\\temp\\octane_preview.png", c4d.FILTER_PNG)
# Restore original sample count after
```
This gives a usable Octane preview in a few seconds showing actual materials, lighting, and reflections.
The hardware preview renderer (RDATA_RENDERENGINE_PREVIEWHARDWARE) does NOT show Octane materials.

## Octane Viewport Render (preferred for CC feedback loop)
```python
# Render to Viewport (Ctrl+R) — uses Octane, renders INTO the viewport buffer
c4d.CallCommand(12163)  # Render Viewport
import time; time.sleep(15)  # Wait for Octane to finish (adjust based on complexity)
# Then capture_viewport grabs the Octane-rendered result
```
This is the best way to get Octane renders for CC visual feedback — fast, uses current camera, and capture_viewport picks it up.

## Offline Rendering via Python
```python
# RenderDocument uses Octane if it's the active engine, but it's an OFFLINE render,
# NOT the Live Viewer. The Live Viewer buffer is not accessible from Python/C++.
rd = doc.GetActiveRenderData().GetClone(c4d.COPYFLAGS_NONE)
bc = rd.GetDataInstance()
bc[c4d.RDATA_XRES] = 800.0   # MUST be float in C4D 2026, not int
bc[c4d.RDATA_YRES] = 600.0
bmp = c4d.bitmaps.BaseBitmap()
bmp.Init(800, 600)
c4d.documents.RenderDocument(doc, bc, bmp, c4d.RENDERFLAGS_EXTERNAL)
bmp.Save("C:/temp/render.png", c4d.FILTER_PNG)
```

## Gotchas
- Octane materials are NOT C4D Node Materials. No GraphNode/port/wire API.
- To discover unknown parameter IDs: drag node to Script Manager console, use GetType() and bracket access
- Image textures need `mat.InsertShader(tex)` before linking: `mat[channel] = tex`
- Color float slider does nothing if color picker is set to anything other than black
- Octane camera/light properties are on TAGS, not the C4D object itself
- LiveDB materials are Octane materials — same system, just pre-built
- Always `mat.Message(c4d.MSG_UPDATE)` and `c4d.EventAdd()` after material changes
- `RDATA_XRES` and `RDATA_YRES` expect **float** in C4D 2026 (not int) — `bc[c4d.RDATA_XRES] = 800.0`
- `capture_viewport` grabs the hardware preview, NOT the Octane Live Viewer
- For Octane-quality previews, use `RenderDocument` with low samples

## Octane docs
https://docs.otoy.com/cinema4d/OctaneRenderforCinema4D.html
https://help.otoy.com/hc/en-us/articles/115002979803
