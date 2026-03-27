# C4D MCP Plugin — MVP Architecture

## Overview

A native C++ Cinema 4D plugin that serves two purposes:

1. **MCP Bridge** — connects Claude Code to Cinema 4D via socket, enabling prompt-to-revision and scene manipulation
2. **Surface Rectangle Tool** — interactive tool for drawing rectangles on object surfaces via raycasting, outputting selection data for AI generation or programmatic operations

Both features share the same plugin binary and socket server. The rectangle tool can be triggered interactively by the artist OR commanded remotely via Claude Code.

---

## Architecture

```
                        ┌─────────────────────────┐
                        │      Claude Code CLI     │
                        │                          │
                        │  ┌────────────────────┐  │
                        │  │  Python MCP Server  │  │
                        │  │  (thin stdio adapter)│  │
                        │  └─────────┬──────────┘  │
                        └────────────┼─────────────┘
                                     │ TCP localhost:5555
                                     ▼
┌────────────────────────────────────────────────────────────┐
│                    C4D C++ Plugin (.cdl64)                  │
│                                                            │
│  ┌──────────────┐  ┌───────────────┐  ┌─────────────────┐ │
│  │ Socket Server │  │ Surface Rect  │  │ Viewport Capture│ │
│  │ (MessageData) │  │ Tool (ToolData│  │ GetViewportImage│ │
│  │               │  │  + GeRay      │  │ → save PNG      │ │
│  │ Receives JSON │  │  Collider)    │  │                 │ │
│  │ commands,     │  │               │  │                 │ │
│  │ dispatches to │  │ Interactive   │  │ On-demand only  │ │
│  │ handlers      │  │ OR remote     │  │ (no polling)    │ │
│  └──────┬───────┘  └───────┬───────┘  └────────┬────────┘ │
│         │                  │                    │          │
│         ▼                  ▼                    ▼          │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Native C4D C++ API                     │   │
│  │                                                     │   │
│  │  Scene Graph    Modeling Kernel    Materials         │   │
│  │  BaseObject     BooleanOp         BaseMaterial      │   │
│  │  PolygonObject  Bevel/Extrude     Octane API*       │   │
│  │  BaseDocument   SendModelingCmd   BaseShader        │   │
│  │                                                     │   │
│  │  Raycasting     Viewport          Python Relay      │   │
│  │  GeRayCollider  BaseDraw          PythonLibrary     │   │
│  │  ColliderCache  GetViewportImage  (escape hatch)    │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                            │
│  * Octane API access depends on Octane SDK availability    │
└────────────────────────────────────────────────────────────┘
```

---

## Plugin Components

### 1. Socket Server (MessageData)

Listens on `localhost:5555` for JSON commands from the Python MCP server. Auto-starts when C4D launches (registered as MessageData plugin, hooks into core message loop).

```
Incoming JSON:
{
  "cmd": "get_scene_info",
  "args": {}
}

Response JSON:
{
  "status": "ok",
  "data": { ... scene hierarchy, object transforms, etc. }
}
```

**No polling. No heartbeat.** The socket only processes commands when they arrive. Between commands, zero CPU/memory overhead.

### 2. Surface Rectangle Tool (ToolData)

Interactive viewport tool for drawing rectangles projected onto mesh surfaces.

**Interactive mode (artist uses it manually):**
1. Artist activates tool from C4D toolbar
2. Clicks on an object surface → raycast finds hit point + normal
3. Drags to define rectangle width/height on the surface tangent plane
4. Rectangle preview drawn in viewport via BaseDraw
5. On release → rectangle data stored, ready for next action

**Remote mode (Claude Code commands it):**
```json
{
  "cmd": "raycast",
  "args": {
    "screen_x": 500,
    "screen_y": 300
  }
}
→ Returns: hit_point, surface_normal, hit_object_name, polygon_index

{
  "cmd": "define_surface_rect",
  "args": {
    "center": [100, 50, 0],
    "normal": [0, 0, 1],
    "width": 30,
    "height": 20
  }
}
→ Stores rect selection, draws preview in viewport
```

**Output data structure:**
```cpp
struct SurfaceRect {
    Vector center;        // World position of rectangle center
    Vector normal;        // Surface normal at center
    Vector up;            // Rectangle "up" direction on surface
    Vector right;         // Rectangle "right" direction on surface
    Float width;          // Rectangle width in scene units
    Float height;         // Rectangle height in scene units
    BaseObject* hitObj;   // The object that was clicked
    Int32 polyIndex;      // Polygon index at hit point
};
```

**What you can do with a SurfaceRect:**
- Pass to AI 3D generation API (Hunyuan/Rodin/Tripo) → get mesh → place it
- Tell Claude Code "create a cube here" → native object creation at that position
- Use as selection area for modeling operations (extrude, inset, boolean)
- Capture viewport from that angle for context-aware AI prompting

### 3. Viewport Capture

Native `BaseDraw::GetViewportImage()` — grabs the actual GPU framebuffer.

```json
{
  "cmd": "capture_viewport",
  "args": {
    "output_path": "C:\\temp\\viewport.png",
    "width": 1280,
    "height": 720
  }
}
→ Saves PNG, returns path + camera matrix
```

- Instant (reads existing VRAM, no re-render)
- Captures exactly what the artist sees
- On-demand only — zero overhead when not called
- Returns camera matrix alongside image (useful for AI context)

### 4. Python Relay (escape hatch)

For anything not yet mapped to a native command:

```json
{
  "cmd": "execute_python",
  "args": {
    "code": "import c4d\nobj = doc.GetActiveObject()\nprint(obj.GetName())"
  }
}
→ Returns: stdout/stderr from script execution
```

This is the same `execute_python` from the original MCP server, but routed through the C++ plugin's socket instead of a separate Python process.

---

## MCP Tool Definitions

The Python MCP server exposes these tools to Claude Code. Intentionally lean — Tool Search handles the rest.

```
CORE (always loaded, ~2K tokens):
  execute_python    — Run arbitrary Python in C4D
  get_scene_info    — Object tree, transforms, materials, render settings

ON-DEMAND (loaded via Tool Search when needed):
  capture_viewport  — Screenshot viewport to PNG
  raycast           — Cast ray from screen coords, get hit data
  define_surface_rect — Define a rectangle on a surface
  get_surface_rect  — Read current rectangle selection data
  start_render      — Kick off command-line Octane render
  save_scene        — Save current project
  reload_textures   — Force texture refresh
```

Total: 9 tools. Only 2 loaded by default. The rest appear when Claude needs them.

---

## Command Reference

### Scene Reading
```json
{"cmd": "get_scene_info"}
→ Full hierarchy with transforms, material assignments, render settings

{"cmd": "get_object_info", "args": {"name": "MainBody"}}
→ Detailed info for one object (points count, poly count, bbox, tags)

{"cmd": "list_materials"}
→ All materials with their types and key parameters
```

### Raycasting & Surface Selection
```json
{"cmd": "raycast", "args": {"screen_x": 500, "screen_y": 300}}
→ {hit_point, normal, object_name, poly_index, uv_coords}

{"cmd": "define_surface_rect", "args": {
    "center": [100, 50, 0],
    "normal": [0, 0, 1],
    "up": [0, 1, 0],
    "width": 30,
    "height": 20,
    "parent_object": "Building_Facade"
}}
→ Stores selection, draws viewport preview

{"cmd": "get_surface_rect"}
→ Returns current SurfaceRect data

{"cmd": "clear_surface_rect"}
→ Clears selection and viewport overlay
```

### Viewport
```json
{"cmd": "capture_viewport", "args": {
    "output_path": "C:\\temp\\viewport.png",
    "width": 1280
}}
→ {path, camera_position, camera_target, focal_length}

{"cmd": "capture_surface_rect_view", "args": {
    "output_path": "C:\\temp\\rect_context.png"
}}
→ Renders viewport from angle facing the surface rect (for AI context)
```

### Object Manipulation
```json
{"cmd": "create_object", "args": {
    "type": "cube",
    "name": "AC_Unit",
    "position": [100, 50, 0],
    "size": [30, 20, 10],
    "parent": "Building_Facade"
}}

{"cmd": "import_mesh", "args": {
    "file_path": "C:\\temp\\generated_model.obj",
    "name": "AI_Generated_AC",
    "align_to_surface_rect": true
}}
→ Imports mesh, aligns to current SurfaceRect (normal, scale, position)

{"cmd": "set_transform", "args": {
    "name": "MainBody",
    "position": [0, 10, 0],
    "rotation": [0, 45, 0]
}}

{"cmd": "set_visibility", "args": {
    "name": "Screw_03",
    "visible": false
}}
```

### Materials
```json
{"cmd": "set_material_color", "args": {
    "material": "Mat_Housing",
    "color": [0.2, 0.4, 0.8]
}}

{"cmd": "set_material_param", "args": {
    "material": "Mat_Plastic",
    "param": "roughness",
    "value": 0.3
}}
```

### Rendering
```json
{"cmd": "start_render", "args": {
    "frame_start": 0,
    "frame_end": 120,
    "output_folder": "D:\\Projects\\ITW\\render\\"
}}
→ Starts C4D command-line render, returns process ID

{"cmd": "render_status"}
→ {running: true, current_frame: 45, total_frames: 120, eta: "12min"}
```

### Python Relay
```json
{"cmd": "execute_python", "args": {
    "code": "import c4d\n..."
}}
→ {stdout, stderr, return_value}
```

---

## Build Requirements

### SDK & Tools
- Visual Studio 2022 (Community edition, free)
- Cinema 4D 2026 SDK (in C4D install directory: `sdk.zip`)
- Maxon Project Tool (generates VS solution from SDK)
- Output: single `.cdl64` binary → drop in C4D plugins folder

### Build Steps (one-time setup)
```
1. Extract sdk.zip from C4D 2026 install
2. Run Project Tool → generate VS 2022 solution
3. Add plugin source files to the solution
4. Build → produces plugin.cdl64
5. Copy to: C:\Users\Aerovisual\AppData\Roaming\Maxon\
   Maxon Cinema 4D 2026_XXXXX\plugins\c4d_mcp_bridge\
6. Restart C4D
```

### Recompilation
Only needed when updating C4D to a new major version (e.g., 2026 → 2027). Point releases within 2026.x are binary compatible.

---

## File Structure

```
c4d-mcp-plugin/
├── cpp/                          # C++ plugin source
│   ├── main.cpp                  # Plugin registration
│   ├── socket_server.cpp/.h      # TCP socket listener
│   ├── command_handler.cpp/.h    # JSON command dispatch
│   ├── surface_rect_tool.cpp/.h  # ToolData for rectangle drawing
│   ├── viewport_capture.cpp/.h   # GetViewportImage wrapper
│   ├── scene_reader.cpp/.h       # Scene graph traversal
│   ├── python_relay.cpp/.h       # execute_python bridge
│   └── utils.cpp/.h              # JSON parsing, helpers
│
├── mcp/                          # Python MCP server
│   ├── server.py                 # FastMCP server (thin adapter)
│   └── c4d_client.py             # TCP client connecting to C++ plugin
│
├── CLAUDE.md                     # Project context for Claude Code
├── SKILLS.md                     # Workflow macros
└── README.md
```

---

## MVP Scope (what to build first)

### Milestone 1: Socket + Scene Reading
- [ ] C++ plugin skeleton (MessageData registration, socket listener)
- [ ] `get_scene_info` command (traverse scene graph, return JSON)
- [ ] `execute_python` relay
- [ ] Python MCP server connecting to socket
- [ ] Test with Claude Code CLI: "what's in my C4D scene?"

### Milestone 2: Viewport Capture
- [ ] `capture_viewport` via `BaseDraw::GetViewportImage()`
- [ ] Save to PNG, return path + camera data
- [ ] Test: Claude Code captures viewport after making a change

### Milestone 3: Surface Raycasting
- [ ] `raycast` command using `GeRayCollider`
- [ ] Returns hit point, normal, object, polygon index
- [ ] Test: Claude Code can query "what's at screen position X,Y?"

### Milestone 4: Surface Rectangle Tool
- [ ] `ToolData` subclass with `MouseInput()` for interactive drawing
- [ ] Rectangle projection onto surface tangent plane
- [ ] Viewport overlay via `BaseDraw` (draw rectangle wireframe)
- [ ] `define_surface_rect` for remote definition via Claude Code
- [ ] `get_surface_rect` to read current selection
- [ ] Test: draw rectangle on a cube face, read data back via MCP

### Milestone 5: Mesh Import + Placement
- [ ] `import_mesh` command (OBJ/FBX/GLB import)
- [ ] `align_to_surface_rect` — orient imported mesh to match rectangle
- [ ] Scale mesh bounding box to fit rectangle dimensions
- [ ] Offset along normal so mesh sits ON surface
- [ ] Test: import a mesh, place it on a surface rectangle

### Milestone 6: AI Generation Pipeline
- [ ] `capture_surface_rect_view` — viewport render from rect-facing angle
- [ ] Connect to Hunyuan3D local server (localhost:8081)
- [ ] Send prompt + context image → receive mesh
- [ ] Auto-place via Milestone 5
- [ ] Test: draw rect on wall, prompt "AC unit", get placed model

---

## Usage Scenarios

### Scenario A: Claude Code revision workflow
```
You: "Change the housing color to blue and re-render"
CC → get_scene_info → finds housing material
CC → execute_python → changes material color
CC → capture_viewport → verifies change looks right
CC → start_render → kicks off overnight render
```

### Scenario B: AI model placement (interactive)
```
You: activate Surface Rect tool in C4D toolbar
You: draw rectangle on building facade
You: type prompt "industrial ventilation unit"
Plugin → captures viewport context
Plugin → sends to Hunyuan3D local server
Plugin → receives OBJ → imports → aligns to surface rect
You: adjust position if needed
```

### Scenario C: AI model placement (via Claude Code)
```
You: "Add an AC unit on the south wall of the building, 
      about 2 meters up, 80cm wide"
CC → raycast to find the wall surface
CC → define_surface_rect at specified location/size
CC → capture_surface_rect_view for context
CC → call Hunyuan3D API with prompt + context
CC → import_mesh with align_to_surface_rect
CC → capture_viewport → shows you the result
You: "Move it 20cm to the left" 
CC → adjusts position
```

### Scenario D: Programmatic surface operations
```
You: "Extrude this area inward by 2cm to create a recess"
CC → get_surface_rect → knows the area
CC → execute_python → selects polygons in that area
CC → execute_python → applies inset + extrude
CC → capture_viewport → shows result
```
