#include "mcp4d_dialog.h"
#include <fstream>
#include <string>

using namespace cinema;

static constexpr Int32 MCP4D_DIALOG_PLUGIN_ID = 1064010;

// Gadget IDs
enum
{
	IDC_GROUP_MAIN       = 10000,
	IDC_GROUP_API        = 10001,
	IDC_GROUP_STATUS     = 10002,
	IDC_GROUP_TOOLS      = 10003,

	IDC_LABEL_SECRET_ID  = 10100,
	IDC_EDIT_SECRET_ID   = 10101,
	IDC_LABEL_SECRET_KEY = 10102,
	IDC_EDIT_SECRET_KEY  = 10103,
	IDC_LABEL_REGION     = 10104,
	IDC_EDIT_REGION      = 10105,
	IDC_BTN_CONNECT      = 10106,

	IDC_LABEL_STATUS     = 10200,
	IDC_STATUS_TEXT       = 10201,

	IDC_BTN_SURFACE_RECT = 10300,
	IDC_BTN_CAPTURE_VP   = 10301,

	IDC_GROUP_LOG        = 10500,
	IDC_LOG_TEXT         = 10501,
};

static constexpr Int32 SURFACE_RECT_TOOL_ID = 1064002;

// Helper: get plugin directory path
static std::string GetPluginDir()
{
	// The config file lives next to the mcp/ folder
	Filename dir = GeGetPluginPath();
	Char* cstr = cinema::String(dir.GetString()).GetCStringCopy();
	if (!cstr) return "";
	std::string result(cstr);
	DeleteMem(cstr);
	return result;
}

static std::string ToStd(const cinema::String& s)
{
	Char* cstr = s.GetCStringCopy();
	if (!cstr) return "";
	std::string result(cstr);
	DeleteMem(cstr);
	return result;
}

class MCP4DDialog : public GeDialog
{
	String _logBuffer;

	void Log(const String& msg)
	{
		if (_logBuffer.GetLength() > 0)
			_logBuffer += "\n"_s;
		_logBuffer += msg;
		SetString(IDC_LOG_TEXT, _logBuffer);
	}

	void LogClear()
	{
		_logBuffer = ""_s;
		SetString(IDC_LOG_TEXT, ""_s);
	}

public:
	virtual Bool CreateLayout() override
	{
		SetTitle("MCP4D"_s);

		// --- API Credentials ---
		GroupBegin(IDC_GROUP_API, BFH_SCALEFIT | BFV_TOP, 2, 0, "Tencent Cloud"_s, 0);
		GroupBorder(BORDER_WITH_TITLE_BOLD);
		GroupBorderSpace(8, 8, 8, 8);

			AddStaticText(IDC_LABEL_SECRET_ID, BFH_LEFT, 80, 0, "Secret ID"_s, 0);
			AddEditText(IDC_EDIT_SECRET_ID, BFH_SCALEFIT, 0, 0, EDITTEXT_PASSWORD);

			AddStaticText(IDC_LABEL_SECRET_KEY, BFH_LEFT, 80, 0, "Secret Key"_s, 0);
			AddEditText(IDC_EDIT_SECRET_KEY, BFH_SCALEFIT, 0, 0, EDITTEXT_PASSWORD);

			AddStaticText(IDC_LABEL_REGION, BFH_LEFT, 80, 0, "Region"_s, 0);
			AddEditText(IDC_EDIT_REGION, BFH_SCALEFIT, 0, 0);

			AddStaticText(0, BFH_LEFT, 80, 0, ""_s, 0);
			AddButton(IDC_BTN_CONNECT, BFH_LEFT, 120, 0, "Save & Connect"_s);

		GroupEnd();

		// --- Status ---
		GroupBegin(IDC_GROUP_STATUS, BFH_SCALEFIT | BFV_TOP, 2, 0, "Status"_s, 0);
		GroupBorder(BORDER_WITH_TITLE_BOLD);
		GroupBorderSpace(8, 8, 8, 8);

			AddStaticText(IDC_LABEL_STATUS, BFH_LEFT, 80, 0, "Server"_s, 0);
			AddStaticText(IDC_STATUS_TEXT, BFH_SCALEFIT, 0, 0, "localhost:5555"_s, 0);

		GroupEnd();

		// --- Tools ---
		GroupBegin(IDC_GROUP_TOOLS, BFH_SCALEFIT | BFV_TOP, 1, 0, "Tools"_s, 0);
		GroupBorder(BORDER_WITH_TITLE_BOLD);
		GroupBorderSpace(8, 8, 8, 8);

			AddButton(IDC_BTN_SURFACE_RECT, BFH_SCALEFIT, 0, 25, "Surface Rectangle Tool"_s);
			AddButton(IDC_BTN_CAPTURE_VP, BFH_SCALEFIT, 0, 25, "Capture Viewport"_s);

		GroupEnd();

		// --- Log ---
		GroupBegin(IDC_GROUP_LOG, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "Log"_s, 0);
		GroupBorder(BORDER_WITH_TITLE_BOLD);
		GroupBorderSpace(8, 4, 8, 4);

			AddMultiLineEditText(IDC_LOG_TEXT, BFH_SCALEFIT | BFV_SCALEFIT, 0, 80,
				DR_MULTILINE_READONLY | DR_MULTILINE_WORDWRAP | DR_MULTILINE_MONOSPACED);

		GroupEnd();

		return true;
	}

	virtual Bool InitValues() override
	{
		SetString(IDC_EDIT_REGION, "ap-singapore"_s);

		// Try to load existing config
		std::string configPath = GetPluginDir() + "/.mcp4d_config.json";
		std::ifstream f(configPath);
		if (f.is_open())
		{
			std::string content((std::istreambuf_iterator<char>(f)),
			                     std::istreambuf_iterator<char>());
			f.close();

			// Simple JSON parsing for our known fields
			auto extractField = [&](const std::string& json, const std::string& key) -> std::string
			{
				std::string needle = "\"" + key + "\": \"";
				auto pos = json.find(needle);
				if (pos == std::string::npos) return "";
				pos += needle.size();
				auto end = json.find("\"", pos);
				if (end == std::string::npos) return "";
				return json.substr(pos, end - pos);
			};

			std::string sid = extractField(content, "secret_id");
			std::string skey = extractField(content, "secret_key");
			std::string region = extractField(content, "region");

			if (!sid.empty())
				SetString(IDC_EDIT_SECRET_ID, maxon::String(sid.c_str()));
			if (!skey.empty())
				SetString(IDC_EDIT_SECRET_KEY, maxon::String(skey.c_str()));
			if (!region.empty())
				SetString(IDC_EDIT_REGION, maxon::String(region.c_str()));

			SetString(IDC_STATUS_TEXT, "Config loaded"_s);
		}

		return true;
	}

	virtual Bool Command(Int32 id, const BaseContainer& msg) override
	{
		switch (id)
		{
			case IDC_BTN_CONNECT:
			{
				cinema::String secretId, secretKey, region;
				GetString(IDC_EDIT_SECRET_ID, secretId);
				GetString(IDC_EDIT_SECRET_KEY, secretKey);
				GetString(IDC_EDIT_REGION, region);

				if (secretId.GetLength() == 0 || secretKey.GetLength() == 0)
				{
					SetString(IDC_STATUS_TEXT, "Enter Secret ID and Key"_s);
					Log("Error: Secret ID and Key required"_s);
					break;
				}

				// Save to config file
				std::string configPath = GetPluginDir() + "/.mcp4d_config.json";
				std::ofstream f(configPath);
				if (f.is_open())
				{
					f << "{\n"
					  << "  \"secret_id\": \"" << ToStd(secretId) << "\",\n"
					  << "  \"secret_key\": \"" << ToStd(secretKey) << "\",\n"
					  << "  \"region\": \"" << ToStd(region) << "\",\n"
					  << "  \"endpoint\": \"hunyuan.intl.tencentcloudapi.com\"\n"
					  << "}" << std::endl;
					f.close();

					SetString(IDC_STATUS_TEXT, "Credentials saved"_s);
					Log("Credentials saved to .mcp4d_config.json"_s);
					ApplicationOutput("MCP4D: Credentials saved to config"_s);
				}
				else
				{
					SetString(IDC_STATUS_TEXT, "Failed to save config"_s);
				}
				break;
			}

			case IDC_BTN_SURFACE_RECT:
			{
				CallCommand(SURFACE_RECT_TOOL_ID);
				Log("Surface Rectangle tool activated"_s);
				break;
			}

			case IDC_BTN_CAPTURE_VP:
			{
				Log("Use capture_viewport via MCP tool"_s);
				break;
			}

			}
		return true;
	}
};

class MCP4DDialogCommand : public CommandData
{
	MCP4DDialog _dialog;

public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager) override
	{
		if (!_dialog.IsOpen())
			_dialog.Open(DLG_TYPE::ASYNC, MCP4D_DIALOG_PLUGIN_ID, -1, -1, 350, 450);
		else
			_dialog.Close();
		return true;
	}

	virtual Bool RestoreLayout(void* secret) override
	{
		return _dialog.RestoreLayout(MCP4D_DIALOG_PLUGIN_ID, 0, secret);
	}
};

Bool RegisterMCP4DDialog()
{
	return RegisterCommandPlugin(MCP4D_DIALOG_PLUGIN_ID, "MCP4D"_s, 0,
		nullptr, "MCP4D Control Panel"_s, NewObjClear(MCP4DDialogCommand));
}
