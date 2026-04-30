"""
MCP4D Installer

Finds your Cinema 4D installation, copies the plugin, installs
the Python dependency, and registers the MCP server with Claude Code.

Usage:
    python install.py
    python install.py --c4d "D:/Maxon/Cinema 4D 2025"
"""

import glob
import os
import shutil
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PLUGIN_FILE = os.path.join(SCRIPT_DIR, "dist", "c4d-mcp-bridge.xdl64")
MCP_SERVER = os.path.join(SCRIPT_DIR, "mcp", "server.py")
PLUGIN_FOLDER_NAME = "c4d-mcp-bridge"


def find_c4d_installations():
    """Search common locations for Cinema 4D user preferences (plugins folder).

    Checks AppData/Roaming/Maxon first (the standard user plugins location),
    then falls back to Program Files.
    """
    candidates = []

    # User preferences (AppData/Roaming/Maxon) -- this is where plugins normally go
    appdata = os.environ.get("APPDATA", "")
    if appdata:
        for path in glob.glob(os.path.join(appdata, "Maxon", "Maxon Cinema 4D*")):
            if os.path.isdir(os.path.join(path, "plugins")):
                candidates.append(path)

    # Program Files (application install directory) -- less common for plugins
    search_roots = [
        os.path.join(os.environ.get("PROGRAMFILES", "C:\\Program Files"), "Maxon Cinema 4D*"),
        os.path.join(os.environ.get("PROGRAMFILES", "C:\\Program Files"), "Maxon", "Cinema*"),
    ]
    for pattern in search_roots:
        for path in glob.glob(pattern):
            if os.path.isdir(os.path.join(path, "plugins")):
                candidates.append(path)

    # Deduplicate and sort (AppData entries will appear first since they were added first)
    seen = set()
    unique = []
    for c in candidates:
        norm = os.path.normpath(c)
        if norm not in seen:
            seen.add(norm)
            unique.append(norm)
    return unique


def prompt_choice(options, prompt_text):
    """Let the user pick from a numbered list."""
    for i, opt in enumerate(options, 1):
        print(f"  [{i}] {opt}")
    while True:
        try:
            choice = int(input(f"\n{prompt_text} ")) - 1
            if 0 <= choice < len(options):
                return options[choice]
        except (ValueError, EOFError):
            pass
        print("Invalid choice, try again.")


def step_copy_plugin(c4d_path):
    """Copy the compiled plugin to the C4D plugins folder."""
    dest_dir = os.path.join(c4d_path, "plugins", PLUGIN_FOLDER_NAME)
    dest_file = os.path.join(dest_dir, "c4d-mcp-bridge.xdl64")

    if not os.path.isfile(PLUGIN_FILE):
        print(f"\n  ERROR: Compiled plugin not found at:\n  {PLUGIN_FILE}")
        print("  Build the plugin first, or place c4d-mcp-bridge.xdl64 in the dist/ folder.")
        return False

    os.makedirs(dest_dir, exist_ok=True)
    shutil.copy2(PLUGIN_FILE, dest_file)
    print(f"  Copied plugin to:\n  {dest_file}")
    return True


def step_install_python_deps():
    """Install the fastmcp Python package."""
    try:
        result = subprocess.run(
            [sys.executable, "-m", "pip", "install", "fastmcp"],
            capture_output=True, text=True
        )
        if result.returncode == 0:
            print("  fastmcp installed successfully.")
            return True
        else:
            print(f"  pip install failed:\n  {result.stderr.strip()}")
            return False
    except FileNotFoundError:
        print("  ERROR: Could not run pip. Make sure Python and pip are installed.")
        return False


def step_register_mcp():
    """Register the MCP server with Claude Code."""
    server_path = MCP_SERVER.replace("\\", "/")
    try:
        result = subprocess.run(
            ["claude", "mcp", "add", "cinema4d", "--", "python", server_path],
            capture_output=True, text=True
        )
        if result.returncode == 0:
            print(f"  MCP server registered: cinema4d -> {server_path}")
            return True
        else:
            print(f"  Registration failed: {result.stderr.strip()}")
            print(f"\n  You can register manually by running:")
            print(f'  claude mcp add cinema4d -- python "{server_path}"')
            return False
    except FileNotFoundError:
        print("  Claude Code CLI not found.")
        print(f"  After installing Claude Code, register manually:")
        print(f'  claude mcp add cinema4d -- python "{server_path}"')
        return False


def main():
    print()
    print("=" * 56)
    print("  MCP4D Installer -- Cinema 4D Bridge for Claude Code")
    print("=" * 56)
    print()

    # Parse --c4d argument
    c4d_path = None
    if "--c4d" in sys.argv:
        idx = sys.argv.index("--c4d")
        if idx + 1 < len(sys.argv):
            c4d_path = sys.argv[idx + 1]
            if not os.path.isdir(os.path.join(c4d_path, "plugins")):
                print(f"  No plugins/ folder found in: {c4d_path}")
                c4d_path = None

    # Find C4D
    if not c4d_path:
        installations = find_c4d_installations()
        if not installations:
            print("  Could not find Cinema 4D automatically.")
            print("  Please enter the path to your Cinema 4D preferences folder.")
            print("  This is usually something like:")
            print("  C:\\Users\\YourName\\AppData\\Roaming\\Maxon\\Maxon Cinema 4D 2025_XXXXXXXX\n")
            print("  You can find it in C4D under Edit > Preferences > Open Preferences Folder.\n")
            raw = input("  Path: ").strip().strip('"')
            if raw and os.path.isdir(os.path.join(raw, "plugins")):
                c4d_path = raw
            else:
                print(f"\n  No plugins/ folder found at that path. Exiting.")
                print()
                return
        elif len(installations) == 1:
            c4d_path = installations[0]
            print(f"  Found Cinema 4D at: {c4d_path}")
        else:
            print("  Found multiple Cinema 4D installations:\n")
            c4d_path = prompt_choice(installations, "Which one?")

    print()

    # Step 1: Copy plugin
    print("[1/3] Copying plugin to Cinema 4D...")
    if not step_copy_plugin(c4d_path):
        return
    print()

    # Step 2: Install Python dependencies
    print("[2/3] Installing Python dependencies...")
    step_install_python_deps()
    print()

    # Step 3: Register MCP server
    print("[3/3] Registering MCP server with Claude Code...")
    step_register_mcp()
    print()

    # Done
    print("-" * 56)
    print("  Installation complete!")
    print()
    print("  Next steps:")
    print("  1. Restart Cinema 4D (if it's running)")
    print("  2. Open Claude Code in your terminal")
    print("  3. Ask Claude to 'ping cinema 4d' to verify")
    print("-" * 56)
    print()


if __name__ == "__main__":
    main()
