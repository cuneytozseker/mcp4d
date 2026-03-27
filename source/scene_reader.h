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

// Full scene hierarchy with transforms, materials, render settings
nlohmann::json GetSceneInfo(cinema::BaseDocument* doc);

// Detailed info for a single object by name
nlohmann::json GetObjectInfo(cinema::BaseDocument* doc, const std::string& name);

// All materials in the document
nlohmann::json ListMaterials(cinema::BaseDocument* doc);
