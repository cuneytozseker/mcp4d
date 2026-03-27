# MCP4D — Cinema 4D Plugin for Claude Code

Native C++ Cinema 4D plugin that connects Claude Code to C4D via TCP socket + MCP protocol.

## Architecture

```
Claude Code CLI
  └── Python MCP Server (mcp/server.py, thin stdio adapter)
        └── TCP localhost:5555
              └── C4D C++ Plugin (.xdl64)
                    ├── Socket Server (MessageData, 200ms poll)
                    ├── Command Handler (JSON dispatch)
                    ├── Scene Reader (hierarchy, materials, render settings)
                    ├── Python Relay (execute_python via CPYTHON3VM)
                    ├── Raycaster (GeRayCollider from screen coords)
                    ├── Viewport Capture (GetViewportImage → PNG)
                    ├── Surface Rectangle Tool (ToolData + viewport overlay)
                    ├── Native Ops (Boolean, CSTO, SelectPolysAtRect)
                    └── Mesh Import (MergeDocument + align to rect)
```

## Protocol

Send newline-delimited JSON to `localhost:5555`. One command per TCP connection.

```json
{"cmd": "command_name", "args": {...}}
→ {"status": "ok", "data": {...}}
→ {"status": "error", "message": "..."}
```

## Commands

### Scene Reading
| Command | Args | Returns |
|---------|------|---------|
| `ping` | — | `{pong: true}` |
| `get_scene_info` | — | Full hierarchy, transforms, materials, render settings |
| `get_object_info` | `name` | Points, polygons, bbox, tags for one object |
| `list_materials` | — | All materials with types and channel info |

### Python Relay
| Command | Args | Returns |
|---------|------|---------|
| `execute_python` | `code` | `{stdout, stderr}` — runs in C4D's Python VM |

**Note:** Use `c4d.documents.GetActiveDocument()` (not `c4d.GetActiveDocument()`). The `c4d` module is available. Code is passed via `compile()` + `exec()` with escaped string embedding.

### Raycasting & Surface Rectangle
| Command | Args | Returns |
|---------|------|---------|
| `raycast` | `screen_x, screen_y` | hit_point, surface_normal, object_name, polygon_index, distance |
| `define_surface_rect` | `center, normal, width, height` + optional `up, parent_object` | Stores rect, draws viewport preview |
| `get_surface_rect` | — | center, normal, up, right, width, height, object_name, polygon_index |
| `clear_surface_rect` | — | Clears rect and overlay |

### Native C++ Operations
These are in C++ because they're unreliable or tedious in Python.

| Command | Args | Returns |
|---------|------|---------|
| `boolean` | `object_a, object_b, operation` | Baked polygon result. Operations: union, subtract, intersect, without |
| `current_state_to_object` | `name` | Bakes generator → polygon object, replaces original |
| `select_polys_at_rect` | — | Makes editable + selects polygons within surface rect bounds |

### Mesh Import
| Command | Args | Returns |
|---------|------|---------|
| `import_mesh` | `file_path` + optional `name, align_to_surface_rect` | Imports OBJ/FBX/GLB/STL/USD, optionally aligns to rect |

### Viewport
| Command | Args | Returns |
|---------|------|---------|
| `capture_viewport` | `output_path` + optional `width, height` | Saves PNG via GetViewportImage, returns path + camera data |

## Surface Rectangle Tool

Interactive viewport tool: **Extensions > Surface Rectangle**

- **Click + drag** on any polygon surface to draw a rectangle
- **Alignment modes** (Attribute Manager): Screen (viewport-relative) or World (axis-aligned)
- **Display options**: Show/hide normal arrow and diagonal lines
- **Data shared** with MCP commands — draw interactively, read via `get_surface_rect`

The tool uses ray-plane intersection for perspective-correct corner placement. Tangent basis is computed by projecting screen/world axes onto the surface plane, with cross-product orthogonalization.

## Scene Building

When building scenes, check [SKILLS.md](SKILLS.md) for matching styles before improvising. If the user references a recipe by name, follow it exactly. If no recipe matches, build from first principles using the available MCP tools.

When faced with complex compositions, break the scene into smaller parts — build and verify each part individually, then bring them together. Don't try to create everything in one shot.

## Design Principles

### Native C++ vs Python Relay
Default to `execute_python` for modeling operations. Only add native C++ commands when:
- **Booleans** — generator-based approach more reliable in C++
- **Bevel/Extrude with selection** — polygon selection + operation is tedious in Python
- **CurrentStateToObject** — baking generators, commonly needed

### Python Relay Tips
See [C4D_PYTHON_REFERENCE.md](C4D_PYTHON_REFERENCE.md) for commonly used Python methods and constants.
See [OCTANE_REFERENCE.md](OCTANE_REFERENCE.md) for Octane material creation, textures, lighting, and render settings.

- Use `c4d.documents.GetActiveDocument()` to get the document
- `c4d.utils.SendModelingCommand()` for modeling operations
- Always call `c4d.EventAdd()` after scene changes
- Create objects with `c4d.BaseObject(c4d.Ocube)` etc.
- Set parameters with `obj[c4d.PRIM_CUBE_LEN] = c4d.Vector(x, y, z)`
- Build matrices with `c4d.Matrix()` — set `m.off`, `m.v1`, `m.v2`, `m.v3`
- Cube chamfer: `obj[c4d.PRIM_CUBE_DOFILLET] = True`, `PRIM_CUBE_FRAD` for radius, `PRIM_CUBE_SUBF] = 1` for sharp chamfer (catches edge highlights in render)

### Working with the Surface Rect from Python
```python
# Get rect data, then place objects at that location
rect = get_surface_rect()  # via MCP
center = rect.center
normal = rect.normal
right = rect.right
up = rect.up

# Build a matrix aligned to the surface
m = c4d.Matrix()
m.off = c4d.Vector(*center) + c4d.Vector(*normal) * offset
m.v1 = c4d.Vector(*right)
m.v2 = c4d.Vector(*up)
m.v3 = c4d.Vector(*normal)
obj.SetMg(m)
```

## File Structure

```
c4d-mcp-bridge/
├── source/                       # C++ plugin
│   ├── main.cpp                  # Plugin entry, MessageData registration
│   ├── socket_server.cpp/.h      # Winsock2 TCP server with select() loop
│   ├── command_handler.cpp/.h    # JSON command dispatch
│   ├── scene_reader.cpp/.h       # get_scene_info, get_object_info, list_materials
│   ├── python_relay.cpp/.h       # execute_python via CPYTHON3VM
│   ├── raycaster.cpp/.h          # GeRayCollider screen-to-world raycasting
│   ├── viewport_capture.cpp/.h   # GetViewportImage → PNG via ImageSaverClasses
│   ├── surface_rect_tool.cpp/.h  # ToolData + viewport overlay + UI
│   ├── native_ops.cpp/.h         # Boolean, CSTO, SelectPolysAtRect
│   ├── mesh_import.cpp/.h        # MergeDocument + align to surface rect
│   └── json.hpp                  # nlohmann/json (vendored)
├── project/
│   └── projectdefinition.txt     # CMake build config
├── mcp/
│   ├── server.py                 # FastMCP stdio server (thin adapter)
│   ├── generator.py              # Hunyuan3D client (submit → poll → download)
│   └── requirements.txt          # fastmcp + tencentcloud-sdk-python
└── CLAUDE.md                     # This file
```

## Build

```bash
# Configure (from c4d_sdk root)
cmake --preset windows_vs2022_v143

# Build
cmake --build _build_v143 --target c4d-mcp-bridge --config Release

# Output: _build_v143/bin/Release/plugins/c4d-mcp-bridge/c4d-mcp-bridge.xdl64
```

Copy the `.xdl64` to your C4D plugins folder and restart C4D.

## MCP Setup

```bash
claude mcp add cinema4d -- python C:/path/to/mcp/server.py
```

### Auto-allow MCP tools (optional)

To skip permission prompts for MCP tools and Python commands, create `.claude/settings.json` in your project root:

```json
{
  "permissions": {
    "allow": [
      "mcp__cinema4d__*",
      "Bash(python:*)"
    ]
  }
}
```

## Asset Browser (via Python Relay)

Search and load assets from C4D's Asset Browser programmatically:

```python
import c4d, maxon

# Search for assets by name
repo = maxon.AssetInterface.GetUserPrefsRepository()
found = repo.FindAssets(maxon.AssetTypes.File(), maxon.Id(), maxon.Id(), maxon.ASSET_FIND_MODE.LATEST)
for asset in found:
    name = str(asset.GetMetaString(maxon.OBJECT.BASE.NAME, maxon.Resource.GetCurrentLanguage()))
    if "search_term" in name.lower():
        print(name, "|", asset.GetId())

# Load an asset into the scene
assetId = maxon.Id("file_XXXXX")
maxon.AssetManagerInterface.LoadAssets(repo, [(assetId, "")])
c4d.EventAdd()
```

## Object Placement at Surface Rect

To place an object aligned to the current surface rect:

```python
import c4d

# Get rect data via MCP: get_surface_rect
# Then build a matrix from the rect basis vectors:
m = c4d.Matrix()
m.off = c4d.Vector(cx, cy, cz) + c4d.Vector(nx, ny, nz) * offset  # offset along normal
m.v1 = c4d.Vector(rx, ry, rz)   # rect right
m.v2 = c4d.Vector(ux, uy, uz)   # rect up
m.v3 = c4d.Vector(nx, ny, nz)   # rect normal
obj.SetMg(m)

# For scaling to fit rect: compute hierarchy bounding box,
# then uniform scale = min(rect_width/bbox_w, rect_height/bbox_h)
# Apply scale by multiplying m.v1, m.v2, m.v3 by the scale factor
```

**Note on hierarchy bounding boxes:** Top-level null objects return size 0. Walk children recursively to compute the true bounding box in world space.

## Python Effector Code

To set code on a Python Effector, use `OEPYTHON_STRING` (ID 1100), **not** `GV_PYTHON_CODE` (ID 400). The effector won't re-evaluate code changed in-place on an existing object. Instead, **delete and recreate** the effector with the code pre-set before inserting:

```python
eff = c4d.BaseObject(c4d.Omgpython)
eff[c4d.OEPYTHON_MODE] = c4d.OEPYTHON_MODE_FULLCONTROL  # or OEPYTHON_MODE_PARAMETERS
eff[c4d.OEPYTHON_STRING] = "import c4d\n..."  # set code BEFORE inserting
doc.InsertObject(eff)

# Add to cloner's effector list
ied = c4d.InExcludeData()
ied.InsertObject(eff, 1)
cloner[c4d.ID_MG_MOTIONGENERATOR_EFFECTORLIST] = ied
```

**Full Control mode** uses `GetArray`/`SetArray` on MoData to modify all clones at once. **Parameter Control mode** is called per-clone and returns a weight vector.

## Camera Positioning

Don't compute camera rotation manually — C4D's HPB convention is error-prone. Instead, select the target object and frame it:

```python
doc.SetActiveObject(obj)
c4d.CallCommand(12151)  # Frame Selected (same as pressing S key)
c4d.DrawViews(c4d.DRAWFLAGS_ONLY_ACTIVE_VIEW | c4d.DRAWFLAGS_NO_THREAD | c4d.DRAWFLAGS_STATICBREAK)
```

Always call `DrawViews` before `capture_viewport` — the viewport framebuffer won't update until a redraw happens.

**Viewport render (`CallCommand(12163)`) requires C4D to be in the foreground.** If C4D is minimized or behind other windows, the render won't execute and `capture_viewport` will grab the stale OpenGL preview instead. Use offline `RenderDocument` as a fallback when viewport render fails.

**Viewport render timing:** Issue the render in one `execute_python` call, wait in a separate call (`time.sleep`), then `capture_viewport` as a third call. Combining render + sleep in the same Python call blocks the main thread and the render may not actually execute during the sleep.

## Path Format

**Always use forward slashes** in file paths sent to the plugin (e.g. `C:/temp/file.png`). Backslashes get mangled by JSON string escaping (`\t` → tab, `\n` → newline).

**Viewport captures** go to `<project_root>/plugins/c4d-mcp-bridge/temp/`.

## AI Decal Generator Plugin

A separate C4D plugin (`ai_decal_generator`) is installed that generates 2D decals/labels/textures using LLM-generated PyCairo or Pillow code. CC can use it via `execute_python`:

```python
import sys
sys.path.insert(0, "C:/Users/Aerovisual/AppData/Roaming/Maxon/Maxon Cinema 4D 2026_1ABCDC12/plugins/ai_decal_generator/lib")
from generator import DecalGenerator

gen = DecalGenerator("C:/Users/Aerovisual/AppData/Roaming/Maxon/Maxon Cinema 4D 2026_1ABCDC12/plugins/ai_decal_generator")
image_path = gen.generate(
    prompt="warning label with skull icon",
    width=1024, height=1024,
    colors={"primary": (255, 200, 0), "secondary": (0, 0, 0), "background": (255, 255, 255, 255)}
)
# image_path -> workspace/current_decal.png
# Then create an Octane Image Texture material pointing at it
```

Use this for text, logos, labels, warning signs, procedural patterns — anything that's easier as a 2D render than an OSL shader. The generated PNG can be applied as an Octane Image Texture on any object.

## Known Issues
- **Octane Light tag cannot be created via Python.** `MakeTag(1029526)` and `BaseTag(1029526)` both crash. Workaround: user must add one Octane area light manually, then CC clones it for additional lights.
- **`MakeEditable` fails via Python relay.** Use native CSTO command instead.

## TODO
- [ ] Render management (start_render, render_status)
- [ ] Save scene command
- [ ] capture_surface_rect_view — viewport render from rect-facing angle for AI context

## Future: Direct 3D Generation from C4D UI

Currently 3D generation (Hunyuan3D) runs via the `generate_3d` MCP tool from Claude Code. If we ever want a Generate button inside C4D that works independently:

**Architecture:** The C4D Generate button should NOT call the API on the main thread (blocks C4D for ~4 minutes). Instead:
1. Generate button sends `{"cmd": "generate_3d_async", ...}` to `localhost:5555`
2. Command handler spawns a **background Python process**: `python generator.py --prompt "..." --output temp/`
3. The dialog polls via timer with `{"cmd": "generation_status"}` to update the log panel
4. When done, the handler calls `import_mesh` with `align_to_surface_rect`

This keeps C4D responsive while generation runs. Use a preprocessor define `MCP4D_ENABLE_GENERATION` to conditionally compile the UI elements for two build variants.
