"""
C4D MCP Bridge — Python MCP server.

Thin stdio adapter that forwards MCP tool calls to the C++ plugin
via TCP on localhost:5555. All logic lives in the C++ plugin;
this file is pure protocol translation.
"""

import json
import socket
from fastmcp import FastMCP

C4D_HOST = "127.0.0.1"
C4D_PORT = 5555

mcp = FastMCP("c4d-mcp-bridge")


def _send(cmd: str, args: dict | None = None) -> dict:
    """Send a JSON command to the C++ plugin and return the parsed response."""
    msg: dict = {"cmd": cmd}
    if args:
        msg["args"] = args

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(30)
    try:
        sock.connect((C4D_HOST, C4D_PORT))
        sock.sendall((json.dumps(msg) + "\n").encode())

        data = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    finally:
        sock.close()

    return json.loads(data.decode())


# ---------------------------------------------------------------------------
# MCP Tools
# ---------------------------------------------------------------------------

@mcp.tool()
def ping() -> dict:
    """Check if the C4D plugin is running and responsive."""
    return _send("ping")


@mcp.tool()
def get_scene_info() -> dict:
    """Get the full scene hierarchy with object transforms, materials, and render settings."""
    return _send("get_scene_info")


@mcp.tool()
def get_object_info(name: str) -> dict:
    """Get detailed info for a single object by name (points, polygons, bbox, tags)."""
    return _send("get_object_info", {"name": name})


@mcp.tool()
def list_materials() -> dict:
    """List all materials in the scene with their types and key parameters."""
    return _send("list_materials")


@mcp.tool()
def execute_python(code: str) -> dict:
    """Execute Python code inside Cinema 4D's Python environment. Returns stdout and stderr."""
    return _send("execute_python", {"code": code})


@mcp.tool()
def raycast(screen_x: int, screen_y: int) -> dict:
    """Cast a ray from screen coordinates into the scene. Returns hit point, surface normal, object name, polygon index."""
    return _send("raycast", {"screen_x": screen_x, "screen_y": screen_y})


@mcp.tool()
def define_surface_rect(center: list[float], normal: list[float], width: float = 10.0,
                        height: float = 10.0, up: list[float] | None = None,
                        parent_object: str | None = None) -> dict:
    """Define a rectangle on a surface. Draws a preview in the viewport."""
    args = {"center": center, "normal": normal, "width": width, "height": height}
    if up:
        args["up"] = up
    if parent_object:
        args["parent_object"] = parent_object
    return _send("define_surface_rect", args)


@mcp.tool()
def get_surface_rect() -> dict:
    """Read the current surface rectangle selection data."""
    return _send("get_surface_rect")


@mcp.tool()
def clear_surface_rect() -> dict:
    """Clear the surface rectangle selection and viewport overlay."""
    return _send("clear_surface_rect")


@mcp.tool()
def boolean(object_a: str, object_b: str, operation: str) -> dict:
    """Boolean operation between two objects. Operations: union, subtract, intersect, without. Bakes result to polygons."""
    return _send("boolean", {"object_a": object_a, "object_b": object_b, "operation": operation})


@mcp.tool()
def current_state_to_object(name: str) -> dict:
    """Bake a generator object into polygon geometry (Current State to Object)."""
    return _send("current_state_to_object", {"name": name})


@mcp.tool()
def select_polys_at_rect() -> dict:
    """Make object editable and select polygons within the current surface rectangle. Use after drawing a surface rect."""
    return _send("select_polys_at_rect")


@mcp.tool()
def import_mesh(file_path: str, name: str = "", align_to_surface_rect: bool = False) -> dict:
    """Import a mesh file (OBJ/FBX/GLB/STL/USD) into the scene. Optionally align to the current surface rectangle."""
    args = {"file_path": file_path}
    if name:
        args["name"] = name
    if align_to_surface_rect:
        args["align_to_surface_rect"] = True
    return _send("import_mesh", args)


@mcp.tool()
def capture_viewport(output_path: str, width: int = 0, height: int = 0) -> dict:
    """Render the active viewport to a PNG file. Returns the file path and camera data."""
    args = {"output_path": output_path}
    if width > 0:
        args["width"] = width
    if height > 0:
        args["height"] = height
    return _send("capture_viewport", args)


if __name__ == "__main__":
    mcp.run()
