#include "mesh_import.h"
#include "surface_rect_tool.h"
#include "c4d.h"

using json = nlohmann::json;
using namespace cinema;

static std::string ToStd(const cinema::String& s)
{
	Char* cstr = s.GetCStringCopy();
	if (!cstr) return "";
	std::string result(cstr);
	DeleteMem(cstr);
	return result;
}

static json VectorToJson(const Vector& v)
{
	return json::array({v.x, v.y, v.z});
}

// Collect all top-level objects that were added after a merge
static void CollectNewObjects(BaseDocument* doc, BaseObject* prevLast, maxon::BaseArray<BaseObject*>& out)
{
	// Walk from the first object until we find objects after prevLast
	Bool pastPrev = (prevLast == nullptr);
	for (BaseObject* obj = doc->GetFirstObject(); obj; obj = obj->GetNext())
	{
		if (pastPrev)
		{
			out.Append(obj) iferr_ignore("collect"_s);
		}
		else if (obj == prevLast)
		{
			pastPrev = true;
		}
	}
}

// Get the last top-level object in the document
static BaseObject* GetLastTopObject(BaseDocument* doc)
{
	BaseObject* last = nullptr;
	for (BaseObject* obj = doc->GetFirstObject(); obj; obj = obj->GetNext())
		last = obj;
	return last;
}

// Compute the bounding box of an object hierarchy in world space
static void ComputeWorldBBox(BaseObject* obj, Vector& bbMin, Vector& bbMax, Bool& first)
{
	if (!obj) return;

	Matrix mg = obj->GetMg();
	Vector rad = obj->GetRad();
	Vector mp = obj->GetMp();

	// 8 corners of the local bounding box
	for (int i = 0; i < 8; i++)
	{
		Vector corner = mp + Vector(
			(i & 1) ? rad.x : -rad.x,
			(i & 2) ? rad.y : -rad.y,
			(i & 4) ? rad.z : -rad.z
		);
		Vector world = mg * corner;

		if (first)
		{
			bbMin = bbMax = world;
			first = false;
		}
		else
		{
			bbMin.x = maxon::Min(bbMin.x, world.x);
			bbMin.y = maxon::Min(bbMin.y, world.y);
			bbMin.z = maxon::Min(bbMin.z, world.z);
			bbMax.x = maxon::Max(bbMax.x, world.x);
			bbMax.y = maxon::Max(bbMax.y, world.y);
			bbMax.z = maxon::Max(bbMax.z, world.z);
		}
	}

	// Recurse children
	for (BaseObject* child = obj->GetDown(); child; child = child->GetNext())
		ComputeWorldBBox(child, bbMin, bbMax, first);
}

nlohmann::json ImportMesh(BaseDocument* doc, const std::string& filePath,
                          const std::string& name, bool alignToRect)
{
	if (!doc)
		return json{{"error", "No active document"}};

	Filename fn(maxon::String(filePath.c_str()));

	// Check file exists
	if (!GeFExist(fn))
		return json{{"error", "File not found: " + filePath}};

	// Remember the last object so we can find new ones after merge
	BaseObject* prevLast = GetLastTopObject(doc);

	doc->StartUndo();

	// Merge the file into the document
	maxon::String errorString;
	SCENEFILTER flags = SCENEFILTER::OBJECTS | SCENEFILTER::MATERIALS;

	if (!MergeDocument(doc, fn, flags, nullptr, &errorString))
	{
		doc->EndUndo();
		std::string errMsg = "Import failed";
		if (!errorString.IsEmpty())
			errMsg += ": " + ToStd(cinema::String(errorString));
		return json{{"error", errMsg}};
	}

	// Find the newly imported objects
	maxon::BaseArray<BaseObject*> newObjects;
	CollectNewObjects(doc, prevLast, newObjects);

	if (newObjects.GetCount() == 0)
	{
		doc->EndUndo();
		return json{{"error", "Import produced no objects"}};
	}

	// If multiple objects, group them under a null
	BaseObject* root = nullptr;
	if (newObjects.GetCount() == 1)
	{
		root = newObjects[0];
	}
	else
	{
		root = BaseObject::Alloc(Onull);
		if (root)
		{
			doc->InsertObject(root, nullptr, nullptr);
			doc->AddUndo(UNDOTYPE::NEWOBJ, root);
			for (auto* obj : newObjects)
			{
				doc->AddUndo(UNDOTYPE::CHANGE, obj);
				obj->Remove();
				obj->InsertUnder(root);
			}
		}
		else
		{
			root = newObjects[0]; // fallback
		}
	}

	// Rename
	if (!name.empty())
		root->SetName(maxon::String(name.c_str()));

	doc->AddUndo(UNDOTYPE::CHANGE, root);

	// Align to surface rect if requested
	json alignInfo = nullptr;
	if (alignToRect)
	{
		SurfaceRect& rect = GetSurfaceRect();
		if (rect.valid)
		{
			// Compute imported mesh bounding box
			Vector bbMin, bbMax;
			Bool first = true;
			ComputeWorldBBox(root, bbMin, bbMax, first);
			Vector meshSize = bbMax - bbMin;
			Vector meshCenter = (bbMin + bbMax) * 0.5;

			// Scale to fit rect dimensions
			Float scaleX = (meshSize.x > 0.0001) ? rect.width / meshSize.x : 1.0;
			Float scaleY = (meshSize.y > 0.0001) ? rect.height / meshSize.y : 1.0;
			Float scaleFactor = maxon::Min(scaleX, scaleY); // Uniform scale, fit within rect

			// Build transform matrix: rect basis + scale
			// The mesh's local Z should align with the surface normal
			// right → X, up → Y, normal → Z
			Matrix targetMg;
			targetMg.off = rect.center + rect.normal * (meshSize.z * scaleFactor * 0.5);
			targetMg.sqmat.v1 = rect.right * scaleFactor;
			targetMg.sqmat.v2 = rect.up * scaleFactor;
			targetMg.sqmat.v3 = rect.normal * scaleFactor;

			root->SetMg(targetMg);

			alignInfo = json{
				{"aligned", true},
				{"scale_factor", scaleFactor},
				{"position", VectorToJson(targetMg.off)}
			};
		}
		else
		{
			alignInfo = json{{"aligned", false}, {"reason", "No surface rect defined"}};
		}
	}

	doc->SetActiveObject(root);
	doc->EndUndo();
	EventAdd();

	json result;
	result["file"] = filePath;
	result["object_name"] = ToStd(root->GetName());
	result["imported_count"] = (int)newObjects.GetCount();
	if (alignInfo != nullptr)
		result["alignment"] = alignInfo;

	return result;
}
