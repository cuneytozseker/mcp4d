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

// Import a mesh file (OBJ/FBX/GLB/STL/USD) into the current document.
// Optionally align to the current surface rect.
nlohmann::json ImportMesh(cinema::BaseDocument* doc, const std::string& filePath,
                          const std::string& name, bool alignToRect);
