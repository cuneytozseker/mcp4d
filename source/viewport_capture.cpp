#include "viewport_capture.h"
#include "c4d.h"
#include "maxon/gfx_image.h"
#include "maxon/mediasession_image_export_png.h"

using json = nlohmann::json;
using namespace cinema;

static json VectorToJson(const cinema::Vector& v)
{
	return json::array({v.x, v.y, v.z});
}

nlohmann::json CaptureViewport(BaseDocument* doc, const std::string& outputPath,
                               int width, int height)
{
	if (!doc)
		return json{{"error", "No active document"}};

	if (outputPath.empty())
		return json{{"error", "output_path is required"}};

	BaseDraw* bd = doc->GetActiveBaseDraw();
	if (!bd)
		return json{{"error", "No active viewport"}};

	// Get viewport dimensions
	Int32 cl, ct, cr, cb;
	bd->GetFrame(&cl, &ct, &cr, &cb);
	int vpW = (int)(cr - cl + 1);
	int vpH = (int)(cb - ct + 1);
	if (width <= 0) width = vpW;
	if (height <= 0) height = vpH;

	// Grab the GPU framebuffer — instant, no re-render
	maxon::ImageRef viewportImage;
	bd->GetViewportImage(viewportImage);

	if (!viewportImage)
		return json{{"error", "GetViewportImage returned empty image"}};

	// Wrap ImageRef in ImageTextureRef for saving
	iferr_scope_handler
	{
		return json{{"error", "Failed to save viewport image"}};
	};

	const maxon::ImageTextureRef outTexture = maxon::ImageTextureClasses::TEXTURE().Create() iferr_return;
	outTexture.AddChildren(maxon::IMAGEHIERARCHY::IMAGE, viewportImage, maxon::ImageBaseRef()) iferr_return;

	// Save as PNG via maxon media session
	const maxon::MediaOutputUrlRef pngFormat = maxon::ImageSaverClasses::Png().Create() iferr_return;
	const maxon::Url saveUrl(maxon::String(outputPath.c_str()));
	outTexture.Save(saveUrl, pngFormat, maxon::MEDIASESSIONFLAGS::NONE) iferr_return;

	// Gather camera data
	json camData;
	BaseObject* camObj = bd->GetSceneCamera(doc);
	if (!camObj)
		camObj = bd->GetEditorCamera();
	if (camObj)
	{
		camData["position"] = VectorToJson(camObj->GetAbsPos());
		camData["rotation"] = VectorToJson(camObj->GetAbsRot());
		BaseContainer cbc = camObj->GetDataInstanceRef();
		camData["focal_length"] = cbc.GetFloat(CAMERA_FOCUS);
	}

	return json{
		{"path",   outputPath},
		{"width",  vpW},
		{"height", vpH},
		{"camera", camData}
	};
}
