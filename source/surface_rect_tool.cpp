#include "surface_rect_tool.h"
#include "lib_collider.h"

using json = nlohmann::json;

static constexpr Int32 SURFACE_RECT_TOOL_ID = 1064002;

// ---------------------------------------------------------------------------
// Global SurfaceRect state
// ---------------------------------------------------------------------------
static SurfaceRect g_surfaceRect;

SurfaceRect& GetSurfaceRect() { return g_surfaceRect; }

void ClearSurfaceRect()
{
	g_surfaceRect = SurfaceRect();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
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

// Compute tangent-plane basis from a normal
static void ComputeTangentBasis(const Vector& normal, Vector& outRight, Vector& outUp)
{
	// Pick an axis not parallel to normal
	Vector initial = (Abs(normal.z) < 0.9) ? Vector(0, 0, 1) : Vector(1, 0, 0);
	outRight = Cross(normal, initial).GetNormalized();
	outUp    = Cross(outRight, normal).GetNormalized();
}

// Raycast from screen coords, return hit info. Returns false if no hit.
static Bool RaycastAtScreen(BaseDocument* doc, BaseDraw* bd, Float sx, Float sy,
                            Vector& hitPos, Vector& hitNormal, BaseObject*& hitObj, Int32& polyIdx)
{
	Vector nearW = bd->SW(Vector(sx, sy, 0.0));
	Vector farW  = bd->SW(Vector(sx, sy, 1000.0));
	Vector rayDir = farW - nearW;
	Float rayLen = rayDir.GetLength();
	if (rayLen < 0.0001) return false;
	rayDir = rayDir / rayLen;

	// Collect polygon objects from scene (including generator caches)
	struct ObjCollector
	{
		static void Collect(BaseObject* obj, maxon::BaseArray<BaseObject*>& out)
		{
			while (obj)
			{
				// Check deform cache first (deformed polygon objects)
				BaseObject* deformed = obj->GetDeformCache();
				if (deformed)
				{
					Collect(deformed, out);
					obj = obj->GetNext();
					continue;
				}

				// Check generator cache (primitives like Cube, Sphere)
				BaseObject* cache = obj->GetCache();
				if (cache)
				{
					Collect(cache, out);
					obj = obj->GetNext();
					continue;
				}

				// It's a real polygon object
				if (obj->IsInstanceOf(Opolygon))
				{
					PolygonObject* poly = static_cast<PolygonObject*>(obj);
					if (poly->GetPolygonCount() > 0)
						out.Append(obj) iferr_ignore("collect"_s);
				}

				// Recurse children
				Collect(obj->GetDown(), out);
				obj = obj->GetNext();
			}
		}
	};

	maxon::BaseArray<BaseObject*> polyObjs;
	ObjCollector::Collect(doc->GetFirstObject(), polyObjs);

	ApplicationOutput("SurfaceRect: Found @ polygon objects"_s, (Int32)polyObjs.GetCount());

	AutoAlloc<GeRayCollider> rc;
	if (!rc) return false;

	Float bestDist = MAXVALUE_FLOAT;
	Bool found = false;

	for (auto* obj : polyObjs)
	{
		if (!rc->Init(obj, false)) continue;

		Matrix mg = obj->GetMg();
		Matrix mgInv = ~mg;
		Vector localP   = mgInv * nearW;
		Vector localDir = mgInv * (nearW + rayDir) - localP;
		Float localLen = localDir.GetLength();
		if (localLen < 0.0001) continue;
		localDir = localDir / localLen;

		if (!rc->Intersect(localP, localDir, rayLen * 2.0)) continue;

		GeRayColResult res;
		if (!rc->GetNearestIntersection(&res)) continue;

		Vector worldHit = mg * res.hitpos;
		Float dist = (worldHit - nearW).GetLength();

		if (dist < bestDist)
		{
			bestDist  = dist;
			hitPos    = worldHit;
			// Transform normal to world space
			Vector wn = mg * res.f_normal - mg * Vector(0);
			Float nLen = wn.GetLength();
			hitNormal = (nLen > 0.0001) ? wn / nLen : Vector(0, 1, 0);
			hitObj    = obj;
			polyIdx   = res.face_id;
			found     = true;
		}
	}

	return found;
}

// ---------------------------------------------------------------------------
// Tangent basis computation
// ---------------------------------------------------------------------------
static void ComputeScreenAlignedBasis(BaseDraw* bd, Float mx, Float my,
                                      const Vector& normal, Vector& outRight, Vector& outUp)
{
	Vector screenRight3D = (bd->SW(Vector(mx + 100.0, my, 500.0)) - bd->SW(Vector(mx, my, 500.0))).GetNormalized();
	outRight = screenRight3D - normal * Dot(screenRight3D, normal);
	Float rLen = outRight.GetLength();
	if (rLen > 0.0001)
		outRight = outRight / rLen;
	else
		outRight = Vector(1, 0, 0);
	outUp = Cross(normal, outRight).GetNormalized();
}

static void ComputeWorldAlignedBasis(const Vector& normal, Vector& outRight, Vector& outUp)
{
	Float ax = Abs(normal.x), ay = Abs(normal.y), az = Abs(normal.z);

	// Pick world axes that make visual sense for each face direction
	Vector wantRight, wantUp;
	if (ay >= ax && ay >= az)
	{
		// Top/bottom face: right=+X, up=+Z
		wantRight = Vector(1, 0, 0);
		wantUp    = Vector(0, 0, 1);
	}
	else if (ax >= ay && ax >= az)
	{
		// Left/right face: right=-Z, up=+Y
		wantRight = Vector(0, 0, -((normal.x >= 0) ? 1.0 : -1.0));
		wantUp    = Vector(0, 1, 0);
	}
	else
	{
		// Front/back face: right=+X * sign, up=+Y
		wantRight = Vector(((normal.z >= 0) ? 1.0 : -1.0), 0, 0);
		wantUp    = Vector(0, 1, 0);
	}

	// Project wantRight onto tangent plane, normalize
	outRight = (wantRight - normal * Dot(wantRight, normal));
	Float rLen = outRight.GetLength();
	outRight = (rLen > 0.0001) ? outRight / rLen : Vector(1, 0, 0);

	// Derive outUp via cross product to guarantee orthogonality,
	// then flip if it doesn't match the intended direction
	outUp = Cross(outRight, normal).GetNormalized();
	if (Dot(outUp, wantUp) < 0)
		outUp = -outUp;
}

// ---------------------------------------------------------------------------
// ToolData implementation
// ---------------------------------------------------------------------------
class SurfaceRectTool : public ToolData
{
public:
	virtual Int32 GetState(BaseDocument* doc) override
	{
		return CMD_ENABLED;
	}

	virtual void InitDefaultSettings(BaseDocument* pDoc, BaseContainer& data) override
	{
		data.SetInt32(SURFRECTTOOL_ALIGN_MODE, SURFRECTTOOL_ALIGN_SCREEN);
		data.SetBool(SURFRECTTOOL_SHOW_NORMAL, true);
		data.SetBool(SURFRECTTOOL_SHOW_DIAGONALS, true);
		data.SetFloat(SURFRECTTOOL_WIDTH, 0.0);
		data.SetFloat(SURFRECTTOOL_HEIGHT, 0.0);
	}

	virtual Bool GetDDescription(const BaseDocument* doc, const BaseContainer& data,
	                             Description* description, DESCFLAGS_DESC& flags) const override
	{
		if (!description->LoadDescription(Tbase))
			return false;

		const DescID* singleid = description->GetSingleDescID();

		// Group
		{
			const DescID cid = ConstDescID(DescLevel(SURFRECTTOOL_GROUP, DTYPE_GROUP, 0));
			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_GROUP);
				bc.SetString(DESC_NAME, "Surface Rectangle"_s);
				description->SetParameter(cid, bc, ConstDescIDLevel(0));
			}
		}

		// Alignment mode cycle
		{
			const DescID cid = ConstDescID(DescLevel(SURFRECTTOOL_ALIGN_MODE, DTYPE_LONG, 0));
			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_LONG);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_CYCLE);
				bc.SetString(DESC_NAME, "Alignment"_s);

				BaseContainer items;
				items.SetString(SURFRECTTOOL_ALIGN_SCREEN, "Screen"_s);
				items.SetString(SURFRECTTOOL_ALIGN_WORLD, "World"_s);
				bc.SetContainer(DESC_CYCLE, items);

				description->SetParameter(cid, bc, ConstDescIDLevel(SURFRECTTOOL_GROUP));
			}
		}

		// Show normal checkbox
		{
			const DescID cid = ConstDescID(DescLevel(SURFRECTTOOL_SHOW_NORMAL, DTYPE_BOOL, 0));
			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_BOOL);
				bc.SetString(DESC_NAME, "Show Normal"_s);
				description->SetParameter(cid, bc, ConstDescIDLevel(SURFRECTTOOL_GROUP));
			}
		}

		// Show diagonals checkbox
		{
			const DescID cid = ConstDescID(DescLevel(SURFRECTTOOL_SHOW_DIAGONALS, DTYPE_BOOL, 0));
			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_BOOL);
				bc.SetString(DESC_NAME, "Show Diagonals"_s);
				description->SetParameter(cid, bc, ConstDescIDLevel(SURFRECTTOOL_GROUP));
			}
		}

		// Width (read-only display)
		{
			const DescID cid = ConstDescID(DescLevel(SURFRECTTOOL_WIDTH, DTYPE_REAL, 0));
			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_REAL);
				bc.SetString(DESC_NAME, "Width"_s);
				bc.SetInt32(DESC_UNIT, DESC_UNIT_METER);
				description->SetParameter(cid, bc, ConstDescIDLevel(SURFRECTTOOL_GROUP));
			}
		}

		// Height (read-only display)
		{
			const DescID cid = ConstDescID(DescLevel(SURFRECTTOOL_HEIGHT, DTYPE_REAL, 0));
			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_REAL);
				bc.SetString(DESC_NAME, "Height"_s);
				bc.SetInt32(DESC_UNIT, DESC_UNIT_METER);
				description->SetParameter(cid, bc, ConstDescIDLevel(SURFRECTTOOL_GROUP));
			}
		}

		flags |= DESCFLAGS_DESC::LOADED;
		return true;
	}

	virtual Bool MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd,
	                        EditorWindow* win, const BaseContainer& msg) override
	{
		if (!doc || !bd) return false;

		Float mx = msg.GetFloat(BFM_INPUT_X);
		Float my = msg.GetFloat(BFM_INPUT_Y);
		Int32 button = msg.GetInt32(BFM_INPUT_CHANNEL);

		if (button != BFM_INPUT_MOUSELEFT) return true;

		// Raycast
		Vector hitPos, hitNormal;
		BaseObject* hitObj = nullptr;
		Int32 polyIdx = -1;

		if (!RaycastAtScreen(doc, bd, mx, my, hitPos, hitNormal, hitObj, polyIdx))
			return true;

		g_surfaceRect.normal    = hitNormal;
		g_surfaceRect.hitObj    = hitObj;
		g_surfaceRect.polyIndex = polyIdx;

		// Compute tangent basis based on alignment mode
		Int32 alignMode = data.GetInt32(SURFRECTTOOL_ALIGN_MODE, SURFRECTTOOL_ALIGN_SCREEN);
		Vector surfRight, surfUp;

		if (alignMode == SURFRECTTOOL_ALIGN_WORLD)
			ComputeWorldAlignedBasis(hitNormal, surfRight, surfUp);
		else
			ComputeScreenAlignedBasis(bd, mx, my, hitNormal, surfRight, surfUp);

		g_surfaceRect.right = surfRight;
		g_surfaceRect.up    = surfUp;

		// Corner-to-corner drag via ray-plane intersection
		Vector corner1 = hitPos;
		Float curMx = mx, curMy = my;
		Float dx, dy;
		BaseContainer channels;

		g_surfaceRect.width  = 0.0;
		g_surfaceRect.height = 0.0;
		g_surfaceRect.center = corner1;
		g_surfaceRect.valid  = true;

		win->MouseDragStart(KEY_MLEFT, mx, my, MOUSEDRAGFLAGS::DONTHIDEMOUSE | MOUSEDRAGFLAGS::NOMOVE);

		while (win->MouseDrag(&dx, &dy, &channels) == MOUSEDRAGRESULT::CONTINUE)
		{
			if (dx == 0.0 && dy == 0.0)
				continue;

			curMx += dx;
			curMy += dy;

			Vector rayO = bd->SW(Vector(curMx, curMy, 0.0));
			Vector rayD = (bd->SW(Vector(curMx, curMy, 1000.0)) - rayO).GetNormalized();

			Float denom = Dot(rayD, hitNormal);
			if (Abs(denom) < 0.00001)
				continue;

			Float t = Dot(corner1 - rayO, hitNormal) / denom;
			Vector planeHit = rayO + rayD * t;

			Vector delta = planeHit - corner1;
			Float w = Dot(delta, surfRight);
			Float h = Dot(delta, surfUp);

			g_surfaceRect.width  = Abs(w);
			g_surfaceRect.height = Abs(h);
			g_surfaceRect.center = corner1 + surfRight * (w * 0.5) + surfUp * (h * 0.5);

			// Update width/height in tool data for the UI
			data.SetFloat(SURFRECTTOOL_WIDTH, g_surfaceRect.width);
			data.SetFloat(SURFRECTTOOL_HEIGHT, g_surfaceRect.height);

			DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
		}

		win->MouseDragEnd();
		EventAdd();
		return true;
	}

	virtual Bool GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd,
	                           Float x, Float y, BaseContainer& bc) override
	{
		if (!doc || !bd) return false;

		Int32 mode = data.GetInt32(SURFRECTTOOL_ALIGN_MODE, SURFRECTTOOL_ALIGN_SCREEN);
		const char* modeStr = (mode == SURFRECTTOOL_ALIGN_WORLD) ? "World" : "Screen";
		bc.SetString(RESULT_BUBBLEHELP, maxon::String(
			(std::string("Surface Rectangle [") + modeStr + "] — Click + drag to draw").c_str()));
		bc.SetInt32(RESULT_CURSOR, MOUSE_CROSS);
		return true;
	}

	virtual TOOLDRAW Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd,
	                      BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags) override
	{
		if (!g_surfaceRect.valid || g_surfaceRect.width < 0.001 || g_surfaceRect.height < 0.001)
			return TOOLDRAW::NONE;

		if (!(flags & TOOLDRAWFLAGS::HIGHLIGHT) && flags != TOOLDRAWFLAGS::NONE)
			return TOOLDRAW::NONE;

		Bool showNormal    = data.GetBool(SURFRECTTOOL_SHOW_NORMAL, true);
		Bool showDiagonals = data.GetBool(SURFRECTTOOL_SHOW_DIAGONALS, true);

		bd->SetMatrix_Matrix(nullptr, Matrix());
		bd->SetPen(Vector(0, 0.8, 1.0)); // Cyan

		Vector c = g_surfaceRect.center;
		Vector r = g_surfaceRect.right * (g_surfaceRect.width * 0.5);
		Vector u = g_surfaceRect.up * (g_surfaceRect.height * 0.5);

		Vector p0 = c - r - u;
		Vector p1 = c + r - u;
		Vector p2 = c + r + u;
		Vector p3 = c - r + u;

		// Wireframe rectangle
		bd->DrawLine(p0, p1, 0);
		bd->DrawLine(p1, p2, 0);
		bd->DrawLine(p2, p3, 0);
		bd->DrawLine(p3, p0, 0);

		// Diagonals
		if (showDiagonals)
		{
			bd->SetPen(Vector(0, 0.4, 0.5));
			bd->DrawLine(p0, p2, 0);
			bd->DrawLine(p1, p3, 0);
		}

		// Normal arrow
		if (showNormal)
		{
			bd->SetPen(Vector(1.0, 0.3, 0.0));
			Float arrowLen = maxon::Max(g_surfaceRect.width, g_surfaceRect.height) * 0.3;
			bd->DrawLine(c, c + g_surfaceRect.normal * arrowLen, 0);
		}

		// Center handle
		bd->SetPen(Vector(1.0, 1.0, 0.0));
		bd->DrawHandle(c, DRAWHANDLE::BIG, 0);

		return TOOLDRAW::HIGHLIGHTS;
	}
};

Bool RegisterSurfaceRectTool()
{
	return RegisterToolPlugin(SURFACE_RECT_TOOL_ID, "Surface Rectangle"_s, 0,
		nullptr, "Draw rectangles on object surfaces"_s, NewObjClear(SurfaceRectTool));
}

// ---------------------------------------------------------------------------
// MCP command handlers
// ---------------------------------------------------------------------------
nlohmann::json DefineSurfaceRect(BaseDocument* doc, const nlohmann::json& args)
{
	if (!doc)
		return json{{"error", "No active document"}};

	auto getVec = [&](const std::string& key) -> Vector
	{
		auto& a = args[key];
		return Vector(a[0].get<Float>(), a[1].get<Float>(), a[2].get<Float>());
	};

	if (!args.contains("center") || !args.contains("normal"))
		return json{{"error", "center and normal are required"}};

	g_surfaceRect.center = getVec("center");
	g_surfaceRect.normal = getVec("normal").GetNormalized();

	if (args.contains("up"))
		g_surfaceRect.up = getVec("up").GetNormalized();

	ComputeTangentBasis(g_surfaceRect.normal, g_surfaceRect.right, g_surfaceRect.up);

	// If explicit up was given, recompute right to respect it
	if (args.contains("up"))
	{
		g_surfaceRect.up = getVec("up").GetNormalized();
		g_surfaceRect.right = Cross(g_surfaceRect.up, g_surfaceRect.normal).GetNormalized();
	}

	g_surfaceRect.width  = args.value("width", 10.0);
	g_surfaceRect.height = args.value("height", 10.0);

	// Try to find the parent object by name
	if (args.contains("parent_object"))
	{
		std::string parentName = args["parent_object"].get<std::string>();
		maxon::String searchName(parentName.c_str());
		// Simple search — find first object with this name
		std::function<BaseObject*(BaseObject*)> findObj = [&](BaseObject* obj) -> BaseObject*
		{
			if (!obj) return nullptr;
			if (obj->GetName() == searchName) return obj;
			BaseObject* found = findObj(obj->GetDown());
			return found ? found : findObj(obj->GetNext());
		};
		g_surfaceRect.hitObj = findObj(doc->GetFirstObject());
	}

	g_surfaceRect.valid = true;

	DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD);
	EventAdd();

	return json{{"defined", true}};
}

nlohmann::json GetSurfaceRectInfo()
{
	if (!g_surfaceRect.valid)
		return json{{"valid", false}};

	json result;
	result["valid"]         = true;
	result["center"]        = VectorToJson(g_surfaceRect.center);
	result["normal"]        = VectorToJson(g_surfaceRect.normal);
	result["up"]            = VectorToJson(g_surfaceRect.up);
	result["right"]         = VectorToJson(g_surfaceRect.right);
	result["width"]         = g_surfaceRect.width;
	result["height"]        = g_surfaceRect.height;
	result["polygon_index"] = (int)g_surfaceRect.polyIndex;

	if (g_surfaceRect.hitObj)
		result["object_name"] = ToStd(g_surfaceRect.hitObj->GetName());

	return result;
}
