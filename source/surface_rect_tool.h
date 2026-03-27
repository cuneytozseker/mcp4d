#pragma once

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4702 4267)
#endif
#include "json.hpp"
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#include "c4d.h"

using namespace cinema;

struct SurfaceRect
{
	Vector     center    = Vector(0);
	Vector     normal    = Vector(0, 1, 0);
	Vector     up        = Vector(0, 0, 1);
	Vector     right     = Vector(1, 0, 0);
	Float      width     = 0.0;
	Float      height    = 0.0;
	BaseObject* hitObj   = nullptr;
	Int32      polyIndex = -1;
	Bool       valid     = false;
};

// Tool parameter IDs for the Attribute Manager UI
enum
{
	SURFRECTTOOL_GROUP           = 10000,
	SURFRECTTOOL_ALIGN_MODE      = 10001,  // Cycle: Screen / World / Surface
	SURFRECTTOOL_SHOW_NORMAL     = 10002,  // Bool: draw normal arrow
	SURFRECTTOOL_SHOW_DIAGONALS  = 10003,  // Bool: draw diagonal lines
	SURFRECTTOOL_WIDTH           = 10004,  // Real: read-only current width
	SURFRECTTOOL_HEIGHT          = 10005,  // Real: read-only current height
};

// Alignment modes
enum
{
	SURFRECTTOOL_ALIGN_SCREEN = 0,
	SURFRECTTOOL_ALIGN_WORLD  = 1,
};

// Global surface rect state — shared between tool and MCP commands
SurfaceRect& GetSurfaceRect();
void          ClearSurfaceRect();

// Register the ToolData plugin
Bool RegisterSurfaceRectTool();

// MCP command handlers
nlohmann::json DefineSurfaceRect(cinema::BaseDocument* doc, const nlohmann::json& args);
nlohmann::json GetSurfaceRectInfo();
