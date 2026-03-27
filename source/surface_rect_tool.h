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

// Global surface rect state — shared between tool and MCP commands
SurfaceRect& GetSurfaceRect();
void          ClearSurfaceRect();

// Register the ToolData plugin
Bool RegisterSurfaceRectTool();

// MCP command handlers
nlohmann::json DefineSurfaceRect(cinema::BaseDocument* doc, const nlohmann::json& args);
nlohmann::json GetSurfaceRectInfo();
