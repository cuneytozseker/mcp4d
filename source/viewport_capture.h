#pragma once

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4702 4267)
#endif
#include "json.hpp"
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#include <string>

namespace cinema { class BaseDocument; }

// Capture the active viewport to a PNG file. Returns path + camera data.
nlohmann::json CaptureViewport(cinema::BaseDocument* doc, const std::string& outputPath,
                               int width = 0, int height = 0);
