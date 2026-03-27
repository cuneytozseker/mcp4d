#pragma once

#include <string>

namespace cinema { class BaseDocument; }

// Processes a raw JSON command string and returns a JSON response string.
// Must be called from the main thread (accesses C4D scene graph).
std::string HandleCommand(const std::string& requestJson, cinema::BaseDocument* doc);
