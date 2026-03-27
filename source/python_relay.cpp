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

// Escape a string for safe embedding in a Python string literal using compile()
static maxon::String EscapeForPython(const std::string& code)
{
	// We pass the code via compile() + exec() instead of string injection.
	// This avoids all escaping issues.
	return maxon::String(code.c_str());
}

nlohmann::json ExecutePython(const std::string& code)
{
	iferr_scope_handler
	{
		return json{{"error", "Python execution failed"}};
	};

	auto vm = MAXON_CPYTHON3VM();
	auto scope = vm.CreateScope() iferr_return;

	// Use a bootstrap that receives code via a function parameter,
	// avoiding string-in-string escaping issues entirely.
	// The bootstrap defines helpers, then we call _run_user_code with the actual code.
	maxon::String bootstrap = maxon::String(
		"import sys, io, traceback\n"
		"_stdout_capture = io.StringIO()\n"
		"_stderr_capture = io.StringIO()\n"
		"def _run_user_code(code_str):\n"
		"    old_out, old_err = sys.stdout, sys.stderr\n"
		"    sys.stdout = _stdout_capture\n"
		"    sys.stderr = _stderr_capture\n"
		"    try:\n"
		"        compiled = compile(code_str, '<mcp>', 'exec')\n"
		"        exec(compiled)\n"
		"    except Exception:\n"
		"        traceback.print_exc(file=_stderr_capture)\n"
		"    finally:\n"
		"        sys.stdout = old_out\n"
		"        sys.stderr = old_err\n"
		"def _get_stdout():\n"
		"    return _stdout_capture.getvalue()\n"
		"def _get_stderr():\n"
		"    return _stderr_capture.getvalue()\n"
	);

	scope.Init("mcp_python_exec"_s, bootstrap, maxon::ERRORHANDLING::PRINT, nullptr) iferr_return;
	scope.Execute() iferr_return;

	// Call _run_user_code with the actual code string
	maxon::BlockArray<maxon::Data> helperStack;
	auto codeArg = maxon::Data(EscapeForPython(code));
	maxon::BaseArray<maxon::Data*> runArgs;
	runArgs.Append(&codeArg) iferr_return;
	auto runBlock = runArgs.ToBlock();

	scope.PrivateInvoke("_run_user_code"_s, helperStack,
		maxon::GetDataType<maxon::String>(), &runBlock) iferr_return;

	// Retrieve captured output
	maxon::BaseArray<maxon::Data*> emptyArgs;
	auto emptyBlock = emptyArgs.ToBlock();

	auto* stdoutRes = scope.PrivateInvoke("_get_stdout"_s, helperStack,
		maxon::GetDataType<maxon::String>(), &emptyBlock) iferr_return;
	auto* stderrRes = scope.PrivateInvoke("_get_stderr"_s, helperStack,
		maxon::GetDataType<maxon::String>(), &emptyBlock) iferr_return;

	std::string stdoutStr, stderrStr;

	if (stdoutRes)
	{
		ifnoerr (maxon::String& val = stdoutRes->Get<maxon::String>())
		{
			stdoutStr = MaxonToStd(val);
		}
	}
	if (stderrRes)
	{
		ifnoerr (maxon::String& val = stderrRes->Get<maxon::String>())
		{
			stderrStr = MaxonToStd(val);
		}
	}

	json result;
	result["stdout"] = stdoutStr;
	result["stderr"] = stderrStr;

	return result;
}
