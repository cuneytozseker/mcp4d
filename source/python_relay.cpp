#include "python_relay.h"
#include "maxon/vm.h"
#include "maxon/cpython.h"
#include "maxon/logger.h"
#include "c4d.h"

using json = nlohmann::json;

static std::string MaxonToStd(const maxon::String& s)
{
	cinema::String cs(s);
	cinema::Char* cstr = cs.GetCStringCopy();
	if (!cstr)
		return "";
	std::string result(cstr);
	DeleteMem(cstr);
	return result;
}

// Escape user code for embedding in a Python string.
// Replace backslashes, then quotes, then newlines.
static std::string EscapePythonString(const std::string& s)
{
	std::string out;
	out.reserve(s.size() + 32);
	for (char c : s)
	{
		switch (c)
		{
			case '\\': out += "\\\\"; break;
			case '\'': out += "\\'"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:   out += c; break;
		}
	}
	return out;
}

nlohmann::json ExecutePython(const std::string& code)
{
	auto vm = MAXON_CPYTHON3VM();
	if (!vm)
		return json{{"error", "CPYTHON3VM not available"}};

	auto scopeResult = vm.CreateScope();
	if (scopeResult == maxon::FAILED)
		return json{{"error", "Failed to create scope"}};
	auto scope = scopeResult.GetValue();

	// Build a single script that:
	// 1. Sets up stdout/stderr capture
	// 2. Runs the user code via compile+exec
	// 3. Defines getter functions for output
	std::string escaped = EscapePythonString(code);

	maxon::String fullScript = maxon::String(
		(std::string(
			"import sys, io, traceback\n"
			"_stdout_buf = io.StringIO()\n"
			"_stderr_buf = io.StringIO()\n"
			"_old_out, _old_err = sys.stdout, sys.stderr\n"
			"sys.stdout = _stdout_buf\n"
			"sys.stderr = _stderr_buf\n"
			"try:\n"
			"    exec(compile('") + escaped + std::string("', '<mcp>', 'exec'))\n"
			"except Exception:\n"
			"    traceback.print_exc(file=_stderr_buf)\n"
			"finally:\n"
			"    sys.stdout = _old_out\n"
			"    sys.stderr = _old_err\n"
			"def _get_stdout():\n"
			"    return _stdout_buf.getvalue()\n"
			"def _get_stderr():\n"
			"    return _stderr_buf.getvalue()\n"
		)).c_str()
	);

	auto initResult = scope.Init("mcp_exec"_s, fullScript, maxon::ERRORHANDLING::PRINT, nullptr);
	if (initResult == maxon::FAILED)
		return json{{"error", "Init failed: " + MaxonToStd(initResult.GetError().GetMessage())}};

	auto execResult = scope.Execute();
	if (execResult == maxon::FAILED)
		return json{{"error", "Execute failed: " + MaxonToStd(execResult.GetError().GetMessage())}};

	// Retrieve output
	maxon::BlockArray<maxon::Data> helperStack;
	maxon::BaseArray<maxon::Data*> emptyArgs;
	auto emptyBlock = emptyArgs.ToBlock();

	std::string stdoutStr, stderrStr;

	auto stdoutInvoke = scope.PrivateInvoke("_get_stdout"_s, helperStack,
		maxon::GetDataType<maxon::String>(), &emptyBlock);
	if (stdoutInvoke != maxon::FAILED)
	{
		auto* res = stdoutInvoke.GetValue();
		if (res)
		{
			ifnoerr (maxon::String& val = res->Get<maxon::String>())
				stdoutStr = MaxonToStd(val);
		}
	}

	auto stderrInvoke = scope.PrivateInvoke("_get_stderr"_s, helperStack,
		maxon::GetDataType<maxon::String>(), &emptyBlock);
	if (stderrInvoke != maxon::FAILED)
	{
		auto* res = stderrInvoke.GetValue();
		if (res)
		{
			ifnoerr (maxon::String& val = res->Get<maxon::String>())
				stderrStr = MaxonToStd(val);
		}
	}

	json result;
	result["stdout"] = stdoutStr;
	result["stderr"] = stderrStr;
	return result;
}
