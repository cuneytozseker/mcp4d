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

// Boolean operation between two objects. Creates an Obooleangenerator, then bakes via CSTO.
// operation: "union", "subtract", "intersect", "without"
nlohmann::json BooleanOp(cinema::BaseDocument* doc, const std::string& objectA,
                         const std::string& objectB, const std::string& operation);

// Current State to Object — bakes a generator into polygon geometry.
nlohmann::json CurrentStateToObject(cinema::BaseDocument* doc, const std::string& objectName);

// Make editable + select polygons within the surface rect.
// Prepares the selection so Python can operate on it (extrude, bevel, etc.)
nlohmann::json SelectPolysAtRect(cinema::BaseDocument* doc);
