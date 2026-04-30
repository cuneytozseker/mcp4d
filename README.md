# MCP4D -- Cinema 4D Bridge for Claude Code

A native C++ Cinema 4D plugin that connects [Claude Code](https://claude.ai/claude-code) to Cinema 4D via the [Model Context Protocol](https://modelcontextprotocol.io/). Read the scene, build geometry, execute Python, cast rays, capture the viewport, all from your terminal.

```
Claude Code CLI
  └── Python MCP Server (stdio, thin adapter)
        └── TCP localhost:5555
              └── C4D C++ Plugin (.xdl64)
```

## What It Does
<img width="600" height="337" alt="ezgif com-video-to-gif-converter" src="https://github.com/user-attachments/assets/d5be754f-7a36-4209-8063-122892031e1c" />

Claude Code gains full access to Cinema 4D through 15 native commands:

| Category | Commands |
|----------|----------|
| **Scene** | `ping`, `get_scene_info`, `get_object_info`, `list_materials` |
| **Python** | `execute_python` -- run arbitrary code in C4D's Python VM |
| **Raycasting** | `raycast` -- screen-to-world hit testing |
| **Surface Rect** | `define_surface_rect`, `get_surface_rect`, `clear_surface_rect` |
| **Native Ops** | `boolean`, `current_state_to_object`, `select_polys_at_rect` |
| **Import** | `import_mesh` -- OBJ, FBX, GLB, STL, USD |
| **Viewport** | `capture_viewport` -- PNG snapshot with camera data |

The `execute_python` relay is the escape hatch. Anything that doesn't have a dedicated native command can be done through C4D's Python API. Native commands exist where Python is unreliable (booleans, CSTO) or where C++ is significantly better (scene traversal, raycasting, viewport capture).

## Surface Rectangle Tool
<img width="600" height="337" alt="ezgif com-video-to-gif-converter (1)" src="https://github.com/user-attachments/assets/f3669116-fde2-4619-8943-36bdd70233ff" />
<img width="600" height="337" alt="ezgif com-video-to-gif-converter (2)" src="https://github.com/user-attachments/assets/ce4e9ea5-cf27-41c7-9659-227989da57d9" />

An interactive viewport tool (**Extensions > Surface Rectangle**) that lets you click and drag on any polygon surface to define a rectangle. The rect data is shared with MCP -- draw interactively in the viewport, then read it programmatically from Claude Code.

Used for placing imported meshes, selecting polygon regions, and defining areas of interest on surfaces.

## Recipes and Skills
<img width="600" height="337" alt="MACP4D_1-ezgif com-video-to-gif-converter" src="https://github.com/user-attachments/assets/f443da9e-3ee8-44a1-a0a8-76f1e5426629" />

The plugin ships with a recipe system in `SKILLS.md` that gives Claude Code ready-made workflows for common scene types. When you describe what you want (e.g. "dark moody product shot" or "exploded technical view"), Claude matches your description to a recipe and follows it step by step.

Current recipes include:

- **clean-product-hero** -- white studio, dramatic lighting, beauty shot setup
- **dark-moody-tech** -- dark background with colored accent lighting for electronics and tech products
- **exploded-technical** -- engineering-style exploded view with clean white background
- **tech-collage-cluster** -- dense vertical cluster of tech slabs, editorial style, procedural textures

You can also reference a recipe by name ("build a tech-collage-cluster") and Claude will follow it exactly.

You can also teach Claude Code new recipes by walking it through your workflow step by step. It learns from your actions and saves them as reusable recipes so you don't have to repeat yourself. Even better -- ask Claude Code to generate Python Extensions with dynamic fields so the back-and-forth becomes a single click.

Beyond recipes, `SKILLS.md` includes utility skills like polygon counting, texture size auditing, and batch deletion tools that Claude uses automatically when relevant.

<img width="600" height="337" alt="ezgif com-video-to-gif-converter (4)" src="https://github.com/user-attachments/assets/2d10ec37-fe79-4bba-94f0-9231fc46d9ef" />
<img width="600" height="337" alt="ezgif com-video-to-gif-converter (5)" src="https://github.com/user-attachments/assets/c457343e-cbd0-40b9-a9d7-7ff34462ff2e" />
<img width="600" height="337" alt="ezgif com-video-to-gif-converter (3)" src="https://github.com/user-attachments/assets/248febcf-057e-422f-9f9e-36af817ee7a7" />

## Reference Documents

The repo includes compiled reference docs that Claude reads before working with specific renderers or APIs. These contain verified IDs, parameter values, and code patterns so Claude doesn't have to guess or look things up at runtime:

| File | Contents |
|------|----------|
| `C4D_PYTHON_REFERENCE.md` | Common C4D Python methods, constants, modeling commands |
| `OCTANE_REFERENCE.md` | Octane material IDs, texture node wiring, light parameters, render settings |
| `REDSHIFT_REFERENCE.md` | Redshift node material setup, shader graph construction |
| `INSYDIUM_REFERENCE.md` | X-Particles and related Insydium plugin references |

These files are part of the agent's working knowledge. You don't need to read them yourself, but you can update them if you discover new IDs or patterns.

## Requirements

- Cinema 4D 2024 or newer
- Python 3.10 or newer (usually already installed with C4D)
- Windows (macOS and Linux builds are defined in the project but untested)
- [Claude Code](https://claude.ai/claude-code) CLI installed

## Installation

### Quick Install

Run the installer script from this repo's folder:

```bash
python install.py
```

It will automatically find your Cinema 4D installation, copy the plugin, install the Python dependency, and register the MCP server with Claude Code. If you have multiple C4D versions installed, it will ask which one to use.

If automatic detection doesn't find your C4D, point it manually:

```bash
python install.py --c4d "C:/Program Files/Maxon Cinema 4D 2025"
```

### Manual Installation

If you prefer to install manually, follow the steps below.

#### Step 1: Install the C4D Plugin

The compiled plugin is included in the `dist/` folder of this repo:

```
dist/c4d-mcp-bridge.xdl64
```

Copy `c4d-mcp-bridge.xdl64` into your Cinema 4D user plugins directory. The default location is:

```
C:\Users\<YourName>\AppData\Roaming\Maxon\Maxon Cinema 4D 2025_XXXXXXXX\plugins\c4d-mcp-bridge\
```

To find your exact path, open Cinema 4D and go to **Edit > Preferences > Open Preferences Folder**. The `plugins` folder is inside there.

Create the `c4d-mcp-bridge` subfolder if it doesn't exist, and place the `.xdl64` file inside it.

Restart Cinema 4D. You should see "MCP4D" in the **Extensions** menu if the plugin loaded correctly.

#### Step 2: Install the Python MCP Server

The MCP server is a small Python script that sits between Claude Code and the C4D plugin. It needs one dependency installed.

Open a terminal (Command Prompt, PowerShell, or your preferred terminal) and run:

```bash
pip install fastmcp
```

If you have multiple Python installations, make sure you're using Python 3.10+. You can check with `python --version`.

#### Step 3: Register the MCP Server with Claude Code

Tell Claude Code where to find the MCP server script. In your terminal, run:

```bash
claude mcp add cinema4d -- python "C:/path/to/c4d-mcp-bridge/mcp/server.py"
```

Replace `C:/path/to/c4d-mcp-bridge/` with the actual path to this repo on your machine. Use forward slashes even on Windows.

#### Step 4: Verify the Connection

1. Make sure Cinema 4D is running with the plugin loaded
2. Open Claude Code in your terminal
3. Ask Claude to ping Cinema 4D:

```
> ping cinema 4d
```

If everything is connected, you'll get a `{pong: true}` response.

#### Optional: Auto-allow MCP Tools

By default, Claude Code will ask for your permission each time it wants to use a Cinema 4D tool. If you'd rather let it work without interruptions, create a file called `.claude/settings.json` in your project folder:

```json
{
  "permissions": {
    "allow": [
      "mcp__cinema4d__*"
    ]
  }
}
```

This tells Claude Code to allow all Cinema 4D MCP tools without prompting.

## Protocol

The TCP protocol is simple: newline-delimited JSON, one command per connection.

```
->  {"cmd": "ping"}
<-  {"status": "ok", "data": {"pong": true}}

->  {"cmd": "execute_python", "args": {"code": "print(c4d.GetC4DVersion())"}}
<-  {"status": "ok", "data": {"stdout": "2025200\n", "stderr": ""}}

->  {"cmd": "boolean", "args": {"object_a": "Cube", "object_b": "Sphere", "operation": "subtract"}}
<-  {"status": "ok", "data": {"name": "Cube", "points": 192, "polygons": 194}}
```

## Project Structure

```
c4d-mcp-bridge/
├── source/                       # C++ plugin source
│   ├── main.cpp                  # Plugin entry, MessageData registration
│   ├── socket_server.cpp/.h      # Winsock2 TCP server, select() loop
│   ├── command_handler.cpp/.h    # JSON command dispatch
│   ├── scene_reader.cpp/.h       # get_scene_info, get_object_info, list_materials
│   ├── python_relay.cpp/.h       # execute_python via CPYTHON3VM
│   ├── raycaster.cpp/.h          # GeRayCollider screen-to-world raycasting
│   ├── viewport_capture.cpp/.h   # GetViewportImage to PNG
│   ├── surface_rect_tool.cpp/.h  # ToolData + viewport overlay + UI
│   ├── native_ops.cpp/.h         # Boolean, CSTO, SelectPolysAtRect
│   ├── mesh_import.cpp/.h        # MergeDocument + align to surface rect
│   ├── mcp4d_dialog.cpp/.h       # Plugin dialog UI
│   └── json.hpp                  # nlohmann/json (vendored)
├── project/
│   └── projectdefinition.txt     # C4D SDK build config
├── dist/
│   └── c4d-mcp-bridge.xdl64     # Pre-built plugin binary (Windows)
├── mcp/
│   ├── server.py                 # FastMCP stdio server (thin TCP adapter)
│   └── requirements.txt          # Python dependencies
├── install.py                    # Automated installer script
├── CLAUDE.md                     # Agent instructions and internal reference
├── SKILLS.md                     # Scene-building recipes and utility skills
├── C4D_PYTHON_REFERENCE.md       # C4D Python API quick reference
├── OCTANE_REFERENCE.md           # Octane Render IDs and patterns
├── REDSHIFT_REFERENCE.md         # Redshift node material reference
├── INSYDIUM_REFERENCE.md         # Insydium / X-Particles reference
└── README.md                     # This file
```

## How It Works

1. The **C++ plugin** registers a `MessageData` hook that polls a TCP socket every 200ms on Cinema 4D's main thread.
2. The **Python MCP server** (`server.py`) runs as a stdio process managed by Claude Code. It translates MCP tool calls into JSON commands and forwards them over TCP to the plugin.
3. The **command handler** inside the plugin dispatches incoming JSON to the appropriate module: scene reader, Python relay, raycaster, and so on.
4. All C++ operations run on C4D's main thread (via the `CoreMessage` hook), ensuring thread safety with the scene graph.

## Building from Source

If you want to modify the C++ plugin and compile it yourself, you'll need the Cinema 4D C++ SDK (R2024+) and CMake.

```bash
# From the Cinema 4D SDK root folder (the one containing "plugins/")
cmake --preset windows_vs2022_v143
cmake --build _build_v143 --target c4d-mcp-bridge --config Release
```

The build produces `c4d-mcp-bridge.xdl64` at:
```
_build_v143/bin/Release/plugins/c4d-mcp-bridge/
```

## License

MIT
