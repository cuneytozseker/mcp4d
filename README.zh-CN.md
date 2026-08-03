# MCP4D - 面向 Claude Code 的 Cinema 4D Bridge

[English](README.md)

一个原生 C++ Cinema 4D 插件，通过[模型上下文协议（MCP）](https://modelcontextprotocol.io/)将 [Claude Code](https://claude.ai/claude-code) 连接到 Cinema 4D。你可以从终端读取场景、创建几何体、执行 Python、进行射线检测并捕捉视图。

```
Claude Code CLI
  └── Python MCP Server（stdio，轻量适配器）
        └── TCP localhost:5555
              └── C4D C++ Plugin（.xdl64）
```

## 它能做什么

Claude Code 可通过 15 个原生命令完整访问 Cinema 4D：

| 类别 | 命令 |
| --- | --- |
| **场景** | `ping`、`get_scene_info`、`get_object_info`、`list_materials` |
| **Python** | `execute_python`：在 C4D 的 Python VM 中运行任意代码 |
| **射线检测** | `raycast`：从屏幕到世界坐标的命中测试 |
| **表面矩形** | `define_surface_rect`、`get_surface_rect`、`clear_surface_rect` |
| **原生操作** | `boolean`、`current_state_to_object`、`select_polys_at_rect` |
| **导入** | `import_mesh`：OBJ、FBX、GLB、STL、USD |
| **视图** | `capture_viewport`：带相机数据的 PNG 快照 |

`execute_python` 是扩展能力的出口。没有专门原生命令的操作，仍可通过 C4D Python API 完成；原生命令主要用于 Python 不够可靠的操作（如 Boolean、CSTO），或 C++ 明显更适合的操作（如场景遍历、射线检测、视图捕捉）。

## Surface Rectangle 工具

这是一个交互式视图工具：`Extensions > Surface Rectangle`。它允许你在任意多边形表面上单击并拖动，定义一个矩形区域。矩形数据会与 MCP 共享：你可以在视图中手动绘制，再由 Claude Code 以程序方式读取。

它适用于放置导入网格、选择多边形区域，以及定义表面上的关注区域。

## Recipes 与 Skills

插件在 `SKILLS.md` 中提供配方系统，为常见场景类型准备了可直接使用的工作流。当你描述想要的效果，例如“dark moody product shot”或“exploded technical view”，Claude 会把描述匹配到相应配方，并逐步执行。

当前配方包括：

- **clean-product-hero**：白色摄影棚、戏剧化灯光、产品主视觉设置
- **dark-moody-tech**：深色背景、彩色强调灯光，适合电子与科技产品
- **exploded-technical**：干净白色背景的工程风爆炸图
- **tech-collage-cluster**：密集的竖向科技板块组合、编辑风格与程序纹理

也可以直接用名称引用配方，例如“build a tech-collage-cluster”，Claude 会严格按配方执行。

### 教会 Claude 你的工作方式

你可以逐步演示自己的流程，为 Claude Code 教授新的配方。它会从操作中学习并保存为可复用配方，因此不必重复解释。还可以要求 Claude Code 生成带动态字段的 Python Extensions，把多次来回操作缩成一次点击。

除配方外，`SKILLS.md` 还包含多边形计数、纹理尺寸审计、批量删除等实用 Skills；Claude 会在合适时自动使用它们。

## 参考文档

仓库包含编译好的参考文档。Claude 在处理特定渲染器或 API 前会读取它们；文档内含已验证的 ID、参数值和代码模式，因此 Claude 不必在运行时猜测或查询：

| 文件 | 内容 |
| --- | --- |
| `C4D_PYTHON_REFERENCE.md` | 常用 C4D Python 方法、常量、建模命令 |
| `OCTANE_REFERENCE.md` | Octane 材质 ID、纹理节点连接、灯光参数、渲染设置 |
| `REDSHIFT_REFERENCE.md` | Redshift 节点材质设置、着色器图构建 |
| `INSYDIUM_REFERENCE.md` | X-Particles 与相关 Insydium 插件参考 |

这些文件属于 agent 的工作知识；你不必亲自阅读，但若发现新的 ID 或模式，可以更新它们。

## 环境要求

- Cinema 4D 2024 或更高版本
- Python 3.10 或更高版本（通常已随 C4D 安装）
- Windows（项目定义了 macOS 和 Linux 构建，但尚未测试）
- 已安装 [Claude Code](https://claude.ai/claude-code) CLI

## 安装

### 快速安装

克隆仓库后，在其文件夹中运行安装脚本：

```bash
git clone https://github.com/cuneytozseker/mcp4d.git
cd mcp4d
python install.py
```

脚本会自动查找 Cinema 4D、复制插件、安装 Python 依赖，并向 Claude Code 注册 MCP server。若电脑中有多个 C4D 版本，它会询问使用哪一个。

如果自动检测没有找到 C4D，可手动指定路径：

```bash
python install.py --c4d "C:/Program Files/Maxon Cinema 4D 2025"
```

### 手动安装

若希望手动安装，请按以下步骤进行。

#### 第 1 步：安装 C4D 插件

编译好的插件位于仓库的 `dist/` 文件夹：

```
dist/c4d-mcp-bridge.xdl64
```

将 `c4d-mcp-bridge.xdl64` 复制到 Cinema 4D 用户插件目录。默认位置为：

```
C:\\Users\\<YourName>\\AppData\\Roaming\\Maxon\\Maxon Cinema 4D 2025_XXXXXXXX\\plugins\\c4d-mcp-bridge\\
```

要找到准确路径，请在 Cinema 4D 中打开 `Edit > Preferences > Open Preferences Folder`；其中包含 `plugins` 文件夹。若 `c4d-mcp-bridge` 子文件夹不存在，请创建它并把 `.xdl64` 文件放入其中。

重启 Cinema 4D。插件正确加载后，应能在 `Extensions` 菜单看到 “MCP4D”。

#### 第 2 步：安装 Python MCP Server

MCP server 是位于 Claude Code 与 C4D 插件之间的小型 Python 脚本，需要安装一个依赖。打开终端（Command Prompt、PowerShell 或其他终端），运行：

```bash
pip install fastmcp
```

若电脑中安装了多个 Python，请确认使用的是 Python 3.10+。可通过 `python --version` 查看。

#### 第 3 步：向 Claude Code 注册 MCP Server

告诉 Claude Code MCP server 脚本的位置。在终端运行：

```bash
claude mcp add cinema4d -- python "C:/path/to/c4d-mcp-bridge/mcp/server.py"
```

将 `C:/path/to/c4d-mcp-bridge/` 替换为本机仓库的实际路径。即使在 Windows 上也使用正斜杠。

#### 第 4 步：验证连接

1. 确保 Cinema 4D 已运行且插件已加载。
2. 在终端打开 Claude Code。
3. 请求 Claude ping Cinema 4D：

```
> ping cinema 4d
```

若一切连接正常，会得到 `{pong: true}` 响应。

#### 可选：自动允许 MCP 工具

默认情况下，Claude Code 每次调用 Cinema 4D 工具都会请求你的许可。若希望减少打断，可在项目文件夹创建 `.claude/settings.json`：

```json
{
  "permissions": {
    "allow": [
      "mcp__cinema4d__*"
    ]
  }
}
```

这会让 Claude Code 不再逐次询问，直接允许所有 Cinema 4D MCP 工具。

## 协议

TCP 协议很简单：以换行分隔 JSON，每次连接传递一条命令。

```
->  {"cmd": "ping"}
<-  {"status": "ok", "data": {"pong": true}}

->  {"cmd": "execute_python", "args": {"code": "print(c4d.GetC4DVersion())"}}
<-  {"status": "ok", "data": {"stdout": "2025200\\n", "stderr": ""}}

->  {"cmd": "boolean", "args": {"object_a": "Cube", "object_b": "Sphere", "operation": "subtract"}}
<-  {"status": "ok", "data": {"name": "Cube", "points": 192, "polygons": 194}}
```

## 项目结构

```
c4d-mcp-bridge/
├── source/                       # C++ 插件源代码
│   ├── main.cpp                  # 插件入口、MessageData 注册
│   ├── socket_server.cpp/.h      # Winsock2 TCP server、select() 循环
│   ├── command_handler.cpp/.h    # JSON 命令分发
│   ├── scene_reader.cpp/.h       # get_scene_info、get_object_info、list_materials
│   ├── python_relay.cpp/.h       # 通过 CPYTHON3VM 执行 execute_python
│   ├── raycaster.cpp/.h          # GeRayCollider 屏幕到世界的射线检测
│   ├── viewport_capture.cpp/.h   # 通过 GetViewportImage 生成 PNG
│   ├── surface_rect_tool.cpp/.h  # ToolData、视图叠加层、UI
│   ├── native_ops.cpp/.h         # Boolean、CSTO、SelectPolysAtRect
│   ├── mesh_import.cpp/.h        # MergeDocument、对齐到 surface rect
│   ├── mcp4d_dialog.cpp/.h       # 插件对话框 UI
│   └── json.hpp                  # nlohmann/json（内置）
├── project/
│   └── projectdefinition.txt     # C4D SDK 构建配置
├── dist/
│   └── c4d-mcp-bridge.xdl64      # 预构建插件二进制文件（Windows）
├── mcp/
│   ├── server.py                 # FastMCP stdio server（轻量 TCP 适配器）
│   └── requirements.txt          # Python 依赖
├── install.py                    # 自动安装脚本
├── CLAUDE.md                     # Agent 指令与内部参考
├── SKILLS.md                     # 场景构建配方与实用 Skills
├── C4D_PYTHON_REFERENCE.md       # C4D Python API 快速参考
├── OCTANE_REFERENCE.md           # Octane Render ID 与模式
├── REDSHIFT_REFERENCE.md         # Redshift 节点材质参考
├── INSYDIUM_REFERENCE.md         # Insydium / X-Particles 参考
└── README.md                     # 本文件
```

## 工作原理

1. **C++ plugin** 注册一个 `MessageData` hook，在 Cinema 4D 主线程中每 200ms 轮询一次 TCP socket。
2. **Python MCP server**（`server.py`）是由 Claude Code 管理的 stdio 进程。它将 MCP 工具调用转换为 JSON 命令，再经 TCP 转发给插件。
3. 插件中的 **command handler** 将传入 JSON 分发到对应模块，例如 scene reader、Python relay、raycaster。
4. 所有 C++ 操作都在 C4D 主线程执行（通过 `CoreMessage` hook），从而保证场景图的线程安全。

## 从源代码构建

若要修改 C++ plugin 并自行编译，需要 Cinema 4D C++ SDK（R2024+）与 CMake：

```bash
# 在 Cinema 4D SDK 根文件夹中执行（包含 "plugins/" 的目录）
cmake --preset windows_vs2022_v143
cmake --build _build_v143 --target c4d-mcp-bridge --config Release
```

构建结果为：

```
_build_v143/bin/Release/plugins/c4d-mcp-bridge/
```

## 许可证

MIT

---

中文文档贡献：[@truman-t3](https://github.com/truman-t3)
