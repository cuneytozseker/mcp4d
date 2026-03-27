#include "scene_reader.h"
#include "c4d.h"

using json = nlohmann::json;
using namespace cinema;

// Convert cinema::String to std::string (UTF-8)
static std::string ToStd(const cinema::String& s)
{
	// Use the deprecated but simple GetCStringCopy
	Char* cstr = s.GetCStringCopy();
	if (!cstr)
		return "";
	std::string result(cstr);
	DeleteMem(cstr);
	return result;
}

static json VectorToJson(const cinema::Vector& v)
{
	return json::array({v.x, v.y, v.z});
}

static json ObjectTransform(BaseObject* obj)
{
	return json{
		{"position", VectorToJson(obj->GetAbsPos())},
		{"rotation", VectorToJson(obj->GetAbsRot())},
		{"scale",    VectorToJson(obj->GetAbsScale())}
	};
}

static json TraverseObject(BaseObject* obj)
{
	if (!obj)
		return nullptr;

	json node;
	node["name"]      = ToStd(obj->GetName());
	node["type"]      = (int)obj->GetType();
	node["type_name"] = ToStd(obj->GetTypeName());
	node["transform"] = ObjectTransform(obj);
	node["visible"]   = (obj->GetEditorMode() != MODE_OFF);

	// Material tags
	json materials = json::array();
	for (BaseTag* tag = obj->GetFirstTag(); tag; tag = tag->GetNext())
	{
		if (tag->GetType() == Ttexture)
		{
			BaseMaterial* mat = static_cast<TextureTag*>(tag)->GetMaterial();
			if (mat)
				materials.push_back(ToStd(mat->GetName()));
		}
	}
	if (!materials.empty())
		node["materials"] = materials;

	// Polygon/point counts for mesh objects
	if (obj->IsInstanceOf(Opolygon))
	{
		PolygonObject* poly = static_cast<PolygonObject*>(obj);
		node["point_count"]   = (int)poly->GetPointCount();
		node["polygon_count"] = (int)poly->GetPolygonCount();
	}

	// Children
	json children = json::array();
	for (BaseObject* child = obj->GetDown(); child; child = child->GetNext())
	{
		children.push_back(TraverseObject(child));
	}
	if (!children.empty())
		node["children"] = children;

	return node;
}

nlohmann::json GetSceneInfo(BaseDocument* doc)
{
	if (!doc)
		return json{{"error", "No active document"}};

	json result;
	result["document_name"] = ToStd(doc->GetDocumentName().GetString());
	result["fps"] = (int)doc->GetFps();
	result["frame_min"] = (int)doc->GetMinTime().GetFrame(doc->GetFps());
	result["frame_max"] = (int)doc->GetMaxTime().GetFrame(doc->GetFps());
	result["current_frame"] = (int)doc->GetTime().GetFrame(doc->GetFps());

	// Scene hierarchy
	json objects = json::array();
	for (BaseObject* obj = doc->GetFirstObject(); obj; obj = obj->GetNext())
	{
		objects.push_back(TraverseObject(obj));
	}
	result["objects"] = objects;

	// Materials
	json mats = json::array();
	for (BaseMaterial* mat = doc->GetFirstMaterial(); mat; mat = mat->GetNext())
	{
		mats.push_back(json{
			{"name",      ToStd(mat->GetName())},
			{"type",      (int)mat->GetType()},
			{"type_name", ToStd(mat->GetTypeName())}
		});
	}
	result["materials"] = mats;

	// Render settings
	RenderData* rd = doc->GetActiveRenderData();
	if (rd)
	{
		BaseContainer bc = rd->GetDataInstanceRef();
		result["render_settings"] = json{
			{"name",   ToStd(rd->GetName())},
			{"width",  (int)bc.GetInt32(RDATA_XRES)},
			{"height", (int)bc.GetInt32(RDATA_YRES)},
			{"fps",    (int)bc.GetInt32(RDATA_FRAMERATE)}
		};
	}

	return result;
}

nlohmann::json GetObjectInfo(BaseDocument* doc, const std::string& name)
{
	if (!doc)
		return json{{"error", "No active document"}};

	// Search recursively
	maxon::String searchName(name.c_str());

	std::function<BaseObject*(BaseObject*)> findByName = [&](BaseObject* obj) -> BaseObject*
	{
		if (!obj) return nullptr;
		if (obj->GetName() == searchName)
			return obj;
		BaseObject* found = findByName(obj->GetDown());
		if (found) return found;
		return findByName(obj->GetNext());
	};

	BaseObject* obj = findByName(doc->GetFirstObject());
	if (!obj)
		return json{{"error", "Object not found: " + name}};

	json result = TraverseObject(obj);

	// Add bounding box
	cinema::Vector bbMin, bbMax;
	obj->GetRad();  // ensure cache
	cinema::Vector rad = obj->GetRad();
	cinema::Vector mp  = obj->GetMp();
	result["bounding_box"] = json{
		{"center", VectorToJson(mp)},
		{"size",   VectorToJson(rad * 2.0)}
	};

	// Tags
	json tags = json::array();
	for (BaseTag* tag = obj->GetFirstTag(); tag; tag = tag->GetNext())
	{
		tags.push_back(json{
			{"name",      ToStd(tag->GetName())},
			{"type",      (int)tag->GetType()},
			{"type_name", ToStd(tag->GetTypeName())}
		});
	}
	result["tags"] = tags;

	return result;
}

nlohmann::json ListMaterials(BaseDocument* doc)
{
	if (!doc)
		return json{{"error", "No active document"}};

	json mats = json::array();
	for (BaseMaterial* mat = doc->GetFirstMaterial(); mat; mat = mat->GetNext())
	{
		json m;
		m["name"]      = ToStd(mat->GetName());
		m["type"]      = (int)mat->GetType();
		m["type_name"] = ToStd(mat->GetTypeName());

		// For standard materials, extract key channel info
		if (mat->IsInstanceOf(Mmaterial))
		{
			Material* stdMat = static_cast<Material*>(mat);
			BaseContainer bc = stdMat->GetDataInstanceRef();

			if (stdMat->GetChannelState(CHANNEL_COLOR))
			{
				cinema::Vector col = bc.GetVector(MATERIAL_COLOR_COLOR);
				m["color"] = VectorToJson(col);
			}
			if (stdMat->GetChannelState(CHANNEL_LUMINANCE))
				m["luminance"] = true;
			if (stdMat->GetChannelState(CHANNEL_TRANSPARENCY))
				m["transparency"] = true;
			if (stdMat->GetChannelState(CHANNEL_REFLECTION))
				m["reflection"] = true;
		}

		mats.push_back(m);
	}
	return mats;
}
