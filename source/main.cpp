/*
C4D MCP Bridge Plugin
Connects Claude Code to Cinema 4D via TCP socket on localhost:5555.
Processes JSON commands on the main thread via MessageData.
*/

#include "c4d.h"
#include "c4d_messagedata.h"

#include "socket_server.h"
#include "command_handler.h"
#include "surface_rect_tool.h"

using namespace cinema;

// Plugin IDs — must be unique. Obtain from https://developers.maxon.net/forum/pid
static constexpr Int32 MCP_BRIDGE_MSG_PLUGIN_ID = 1064000;
static SocketServer* g_server = nullptr;

// MessageData plugin that processes socket commands on the main thread.
class MCPBridgeMessageData : public MessageData
{
public:
	virtual Bool CoreMessage(Int32 id, const BaseContainer& bc) override
	{
		// Only process on timer events to avoid re-entrancy
		if (id == EVMSG_ASYNCEDITORMOVE || id == MSG_TIMER)
		{
			ProcessPendingCommands();
		}
		return true;
	}

	virtual Int32 GetTimer() override
	{
		// Return timer interval in ms — poll every 200ms
		return 200;
	}

private:
	void ProcessPendingCommands()
	{
		if (!g_server)
			return;

		// Process at most one command per timer tick to avoid blocking the UI
		PendingCommand cmd;
		if (!g_server->PopCommand(cmd))
			return;

		try
		{
			BaseDocument* doc = GetActiveDocument();
			std::string response = HandleCommand(cmd.requestJson, doc);
			g_server->SendResponse(cmd.clientSocket, response);
		}
		catch (...)
		{
			// Catch any C++ exception to prevent crash propagation
			try
			{
				g_server->SendResponse(cmd.clientSocket,
					"{\"status\":\"error\",\"message\":\"Internal plugin crash caught\"}");
			}
			catch (...) {}
		}
	}
};

cinema::Bool cinema::PluginStart()
{
	ApplicationOutput("MCP Bridge: Starting socket server on localhost:5555"_s);

	g_server = new SocketServer();
	if (!g_server->Start(5555))
	{
		ApplicationOutput("MCP Bridge: Failed to start socket server!"_s);
		delete g_server;
		g_server = nullptr;
		return true;  // Don't prevent C4D from loading
	}

	ApplicationOutput("MCP Bridge: Socket server running"_s);

	if (!RegisterSurfaceRectTool())
		ApplicationOutput("MCP Bridge: Failed to register Surface Rect tool"_s);

	// Register MessageData with timer to poll for commands
	if (!RegisterMessagePlugin(MCP_BRIDGE_MSG_PLUGIN_ID, "MCP Bridge"_s, 0,
	                           NewObjClear(MCPBridgeMessageData)))
	{
		ApplicationOutput("MCP Bridge: Failed to register MessageData plugin"_s);
	}

	return true;
}

void cinema::PluginEnd()
{
	ApplicationOutput("MCP Bridge: Shutting down"_s);
	if (g_server)
	{
		g_server->Stop();
		delete g_server;
		g_server = nullptr;
	}
}

cinema::Bool cinema::PluginMessage(cinema::Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
		{
			if (!cinema::g_resource.Init())
				return false;
			return true;
		}
	}
	return false;
}
