#include "command_handler.h"
#include "scene_reader.h"
#include "python_relay.h"
#include "viewport_capture.h"
#include "raycaster.h"
#include "surface_rect_tool.h"
#include "native_ops.h"
#include "mesh_import.h"

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4702 4267)
#endif
#include "json.hpp"
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

using json = nlohmann::json;

static json MakeError(const std::string& message)
{
	return json{{"status", "error"}, {"message", message}};
}

static json MakeOk(const json& data = nullptr)
{
	json r = {{"status", "ok"}};
	if (!data.is_null())
		r["data"] = data;
	return r;
}

std::string HandleCommand(const std::string& requestJson, cinema::BaseDocument* doc)
{
	json request;
	try
	{
		request = json::parse(requestJson);
	}
	catch (const json::parse_error& e)
	{
		return MakeError(std::string("JSON parse error: ") + e.what()).dump();
	}

	if (!request.contains("cmd") || !request["cmd"].is_string())
		return MakeError("Missing or invalid 'cmd' field").dump();

	std::string cmd = request["cmd"].get<std::string>();
	json args = request.value("args", json::object());

	if (!doc && cmd != "ping")
		return MakeError("No active document").dump();

	try
	{
		if (cmd == "ping")
		{
			return MakeOk({{"pong", true}}).dump();
		}
		else if (cmd == "get_scene_info")
		{
			return MakeOk(GetSceneInfo(doc)).dump();
		}
		else if (cmd == "get_object_info")
		{
			std::string name = args.value("name", "");
			if (name.empty())
				return MakeError("get_object_info requires 'name' arg").dump();
			return MakeOk(GetObjectInfo(doc, name)).dump();
		}
		else if (cmd == "list_materials")
		{
			return MakeOk(ListMaterials(doc)).dump();
		}
		else if (cmd == "execute_python")
		{
			std::string code = args.value("code", "");
			if (code.empty())
				return MakeError("execute_python requires 'code' arg").dump();
			return MakeOk(ExecutePython(code)).dump();
		}
		else if (cmd == "capture_viewport")
		{
			std::string outputPath = args.value("output_path", "");
			int width = args.value("width", 0);
			int height = args.value("height", 0);
			return MakeOk(CaptureViewport(doc, outputPath, width, height)).dump();
		}
		else if (cmd == "raycast")
		{
			int sx = args.value("screen_x", -1);
			int sy = args.value("screen_y", -1);
			if (sx < 0 || sy < 0)
				return MakeError("raycast requires 'screen_x' and 'screen_y' args").dump();
			return MakeOk(Raycast(doc, sx, sy)).dump();
		}
		else if (cmd == "define_surface_rect")
		{
			return MakeOk(DefineSurfaceRect(doc, args)).dump();
		}
		else if (cmd == "get_surface_rect")
		{
			return MakeOk(GetSurfaceRectInfo()).dump();
		}
		else if (cmd == "clear_surface_rect")
		{
			ClearSurfaceRect();
			return MakeOk({{"cleared", true}}).dump();
		}
		else if (cmd == "boolean")
		{
			std::string objA = args.value("object_a", "");
			std::string objB = args.value("object_b", "");
			std::string op = args.value("operation", "");
			if (objA.empty() || objB.empty() || op.empty())
				return MakeError("boolean requires 'object_a', 'object_b', 'operation'").dump();
			return MakeOk(BooleanOp(doc, objA, objB, op)).dump();
		}
		else if (cmd == "current_state_to_object")
		{
			std::string name = args.value("name", "");
			if (name.empty())
				return MakeError("current_state_to_object requires 'name'").dump();
			return MakeOk(CurrentStateToObject(doc, name)).dump();
		}
		else if (cmd == "select_polys_at_rect")
		{
			return MakeOk(SelectPolysAtRect(doc)).dump();
		}
		else if (cmd == "import_mesh")
		{
			std::string path = args.value("file_path", "");
			if (path.empty())
				return MakeError("import_mesh requires 'file_path'").dump();
			std::string meshName = args.value("name", "");
			bool align = args.value("align_to_surface_rect", false);
			return MakeOk(ImportMesh(doc, path, meshName, align)).dump();
		}
		else
		{
			return MakeError("Unknown command: " + cmd).dump();
		}
	}
	catch (const std::exception& e)
	{
		return MakeError(std::string("Command error: ") + e.what()).dump();
	}
	catch (...)
	{
		return MakeError("Unknown internal error").dump();
	}
}
