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

// Execute Python code in C4D's Python VM, return stdout/stderr/result
nlohmann::json ExecutePython(const std::string& code);
