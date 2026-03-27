#pragma once

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4702 4267)
#endif
#include "json.hpp"
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

namespace cinema { class BaseDocument; }

// Cast a ray from screen coordinates into the scene.
// Returns hit_point, surface_normal, object_name, polygon_index, distance.
nlohmann::json Raycast(cinema::BaseDocument* doc, int screenX, int screenY);
