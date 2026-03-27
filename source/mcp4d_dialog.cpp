#include "mcp4d_dialog.h"

using namespace cinema;

static constexpr Int32 MCP4D_DIALOG_PLUGIN_ID = 1064010;

// Gadget IDs
enum
{
	IDC_GROUP_STATUS     = 10002,
	IDC_GROUP_TOOLS      = 10003,

	IDC_LABEL_STATUS     = 10200,
	IDC_STATUS_TEXT       = 10201,

	IDC_BTN_SURFACE_RECT = 10300,
	IDC_BTN_CAPTURE_VP   = 10301,

};

static constexpr Int32 SURFACE_RECT_TOOL_ID = 1064002;

class MCP4DDialog : public GeDialog
{
public:
	virtual Bool CreateLayout() override
	{
		SetTitle("MCP4D"_s);

		// --- API Credentials (disabled — Tencent 3D API not publicly available yet) ---
		// Uncomment when a backend is ready (Replicate, local TripoSR, etc.)
		/*
		GroupBegin(IDC_GROUP_API, BFH_SCALEFIT | BFV_TOP, 2, 0, "3D Generation API"_s, 0);
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
		*/

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

		return true;
	}

	virtual Bool InitValues() override
	{
		return true;
	}

	virtual Bool Command(Int32 id, const BaseContainer& msg) override
	{
		switch (id)
		{
			case IDC_BTN_SURFACE_RECT:
			{
				CallCommand(SURFACE_RECT_TOOL_ID);
				break;
			}

			case IDC_BTN_CAPTURE_VP:
			{
				ApplicationOutput("MCP4D: Use capture_viewport via MCP tool"_s);
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
