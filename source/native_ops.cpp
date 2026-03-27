#include "native_ops.h"
#include "surface_rect_tool.h"
#include "c4d.h"

using json = nlohmann::json;
using namespace cinema;

// From obooleangenerator.h
#define BOOLEANGENERATOR_OPERATION               1001
#define BOOLEANGENERATOR_OPERATION_UNION          1
#define BOOLEANGENERATOR_OPERATION_SUBTRACT       2
#define BOOLEANGENERATOR_OPERATION_INTERSECT      3
#define BOOLEANGENERATOR_OPERATION_WITHOUT        4

// From ge_prepass.h
// MDATA_CURRENTSTATETOOBJECT_INHERITANCE = 2140
// MDATA_CURRENTSTATETOOBJECT_KEEPANIMATION = 2141

static std::string ToStd(const cinema::String& s)
{
	Char* cstr = s.GetCStringCopy();
	if (!cstr) return "";
	std::string result(cstr);
	DeleteMem(cstr);
	return result;
}

// ---------------------------------------------------------------------------
// Boolean Operation
// ---------------------------------------------------------------------------
nlohmann::json BooleanOp(BaseDocument* doc, const std::string& objectA,
                         const std::string& objectB, const std::string& operation)
{
	if (!doc) return json{{"error", "No active document"}};

	BaseObject* objA = doc->SearchObject(maxon::String(objectA.c_str()));
	BaseObject* objB = doc->SearchObject(maxon::String(objectB.c_str()));
	if (!objA) return json{{"error", "Object A not found: " + objectA}};
	if (!objB) return json{{"error", "Object B not found: " + objectB}};

	Int32 boolOp;
	if (operation == "union")          boolOp = BOOLEANGENERATOR_OPERATION_UNION;
	else if (operation == "subtract")  boolOp = BOOLEANGENERATOR_OPERATION_SUBTRACT;
	else if (operation == "intersect") boolOp = BOOLEANGENERATOR_OPERATION_INTERSECT;
	else if (operation == "without")   boolOp = BOOLEANGENERATOR_OPERATION_WITHOUT;
	else return json{{"error", "Invalid operation. Use: union, subtract, intersect, without"}};

	doc->StartUndo();

	// Create boolean generator
	BaseObject* boolGen = BaseObject::Alloc(Oboole);
	if (!boolGen)
	{
		doc->EndUndo();
		return json{{"error", "Failed to create boolean generator"}};
	}

	boolGen->GetDataInstanceRef().SetInt32(BOOLEANGENERATOR_OPERATION, boolOp);
	boolGen->SetName(maxon::String((objectA + "_bool_" + objectB).c_str()));

	// Insert boolean generator into document
	doc->InsertObject(boolGen, nullptr, nullptr);
	doc->AddUndo(UNDOTYPE::NEWOBJ, boolGen);

	// Clone objects as children of the boolean (preserves originals)
	BaseObject* cloneA = static_cast<BaseObject*>(objA->GetClone(COPYFLAGS::NONE, nullptr));
	BaseObject* cloneB = static_cast<BaseObject*>(objB->GetClone(COPYFLAGS::NONE, nullptr));
	if (!cloneA || !cloneB)
	{
		doc->EndUndo();
		return json{{"error", "Failed to clone objects"}};
	}

	cloneA->InsertUnder(boolGen);
	cloneB->InsertUnder(boolGen);

	// Bake the boolean generator via CSTO
	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = boolGen;

	if (!SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT, mcd) || !mcd.result)
	{
		doc->EndUndo();
		return json{{"error", "CSTO on boolean generator failed"}};
	}

	BaseObject* resultObj = static_cast<BaseObject*>(mcd.result->GetIndex(0));
	if (!resultObj)
	{
		AtomArray::Free(mcd.result);
		doc->EndUndo();
		return json{{"error", "Boolean produced no result"}};
	}

	// Insert result, name it, clean up
	resultObj->SetName(boolGen->GetName());
	doc->InsertObject(resultObj, nullptr, nullptr);
	doc->AddUndo(UNDOTYPE::NEWOBJ, resultObj);
	doc->SetActiveObject(resultObj);

	// Remove the boolean generator (it has the clones)
	doc->AddUndo(UNDOTYPE::DELETEOBJ, boolGen);
	boolGen->Remove();
	BaseObject::Free(boolGen);

	// Hide originals
	doc->AddUndo(UNDOTYPE::CHANGE, objA);
	doc->AddUndo(UNDOTYPE::CHANGE, objB);
	objA->SetEditorMode(MODE_OFF);
	objA->SetRenderMode(MODE_OFF);
	objB->SetEditorMode(MODE_OFF);
	objB->SetRenderMode(MODE_OFF);

	AtomArray::Free(mcd.result);
	doc->EndUndo();
	EventAdd();

	Int32 polyCnt = 0;
	if (resultObj->IsInstanceOf(Opolygon))
		polyCnt = static_cast<PolygonObject*>(resultObj)->GetPolygonCount();

	return json{
		{"result_object", ToStd(resultObj->GetName())},
		{"operation", operation},
		{"polygon_count", (int)polyCnt}
	};
}

// ---------------------------------------------------------------------------
// Current State to Object
// ---------------------------------------------------------------------------
nlohmann::json CurrentStateToObject(BaseDocument* doc, const std::string& objectName)
{
	if (!doc) return json{{"error", "No active document"}};

	BaseObject* obj = doc->SearchObject(maxon::String(objectName.c_str()));
	if (!obj) return json{{"error", "Object not found: " + objectName}};

	// Already a polygon object?
	if (obj->IsInstanceOf(Opolygon))
	{
		PolygonObject* poly = static_cast<PolygonObject*>(obj);
		return json{
			{"object", objectName},
			{"already_polygon", true},
			{"polygon_count", (int)poly->GetPolygonCount()},
			{"point_count", (int)poly->GetPointCount()}
		};
	}

	doc->StartUndo();

	ModelingCommandData mcd;
	mcd.doc = doc;
	mcd.op = obj;

	if (!SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT, mcd) || !mcd.result)
	{
		doc->EndUndo();
		return json{{"error", "CSTO failed for: " + objectName}};
	}

	BaseObject* resultObj = static_cast<BaseObject*>(mcd.result->GetIndex(0));
	if (!resultObj)
	{
		AtomArray::Free(mcd.result);
		doc->EndUndo();
		return json{{"error", "CSTO produced no result"}};
	}

	// Replace original with result
	resultObj->SetName(obj->GetName());
	resultObj->SetMg(obj->GetMg());
	doc->InsertObject(resultObj, obj->GetUp(), obj);
	doc->AddUndo(UNDOTYPE::NEWOBJ, resultObj);
	doc->SetActiveObject(resultObj);

	doc->AddUndo(UNDOTYPE::DELETEOBJ, obj);
	obj->Remove();
	BaseObject::Free(obj);

	AtomArray::Free(mcd.result);
	doc->EndUndo();
	EventAdd();

	Int32 polyCnt = 0, ptCnt = 0;
	if (resultObj->IsInstanceOf(Opolygon))
	{
		PolygonObject* poly = static_cast<PolygonObject*>(resultObj);
		polyCnt = poly->GetPolygonCount();
		ptCnt = poly->GetPointCount();
	}

	return json{
		{"object", ToStd(resultObj->GetName())},
		{"polygon_count", (int)polyCnt},
		{"point_count", (int)ptCnt}
	};
}

// ---------------------------------------------------------------------------
// Select Polygons at Surface Rect
// ---------------------------------------------------------------------------
nlohmann::json SelectPolysAtRect(BaseDocument* doc)
{
	if (!doc) return json{{"error", "No active document"}};

	SurfaceRect& rect = GetSurfaceRect();
	if (!rect.valid)
		return json{{"error", "No surface rect defined"}};

	if (!rect.hitObj)
		return json{{"error", "Surface rect has no associated object"}};

	BaseObject* obj = doc->SearchObject(rect.hitObj->GetName());
	if (!obj)
		return json{{"error", "Object not found: " + ToStd(rect.hitObj->GetName())}};

	doc->StartUndo();

	// Make editable if needed
	PolygonObject* poly = nullptr;
	if (obj->IsInstanceOf(Opolygon))
	{
		poly = static_cast<PolygonObject*>(obj);
	}
	else
	{
		doc->AddUndo(UNDOTYPE::DELETEOBJ, obj);

		ModelingCommandData mcd;
		mcd.doc = doc;
		mcd.op = obj;
		if (!SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT, mcd) || !mcd.result)
		{
			doc->EndUndo();
			return json{{"error", "Failed to make editable"}};
		}

		BaseObject* result = static_cast<BaseObject*>(mcd.result->GetIndex(0));
		if (!result || !result->IsInstanceOf(Opolygon))
		{
			AtomArray::Free(mcd.result);
			doc->EndUndo();
			return json{{"error", "CSTO did not produce polygon object"}};
		}

		poly = static_cast<PolygonObject*>(result);
		poly->SetName(obj->GetName());
		poly->SetMg(obj->GetMg());
		doc->InsertObject(poly, obj->GetUp(), obj);
		doc->AddUndo(UNDOTYPE::NEWOBJ, poly);
		obj->Remove();
		BaseObject::Free(obj);
		AtomArray::Free(mcd.result);
	}

	doc->AddUndo(UNDOTYPE::CHANGE, poly);

	// Select polygons within rect
	const CPolygon* polys = poly->GetPolygonR();
	const Vector* points = poly->GetPointR();
	Int32 polyCnt = poly->GetPolygonCount();

	BaseSelect* polySel = poly->GetWritablePolygonS();
	polySel->DeselectAll();

	Matrix mgInv = ~poly->GetMg();
	Vector localCenter = mgInv * rect.center;
	Vector localNormal = (mgInv * (rect.center + rect.normal) - localCenter).GetNormalized();
	Vector localRight  = (mgInv * (rect.center + rect.right) - localCenter).GetNormalized();
	Vector localUp     = (mgInv * (rect.center + rect.up) - localCenter).GetNormalized();
	Float halfW = rect.width * 0.5;
	Float halfH = rect.height * 0.5;

	Int32 selectedCount = 0;
	for (Int32 i = 0; i < polyCnt; i++)
	{
		const CPolygon& p = polys[i];
		Vector pc = (points[p.a] + points[p.b] + points[p.c] + points[p.d]) * 0.25;

		// Must face same direction
		Vector faceN = Cross(points[p.b] - points[p.a], points[p.c] - points[p.a]).GetNormalized();
		if (Dot(faceN, localNormal) < 0.5)
			continue;

		Vector delta = pc - localCenter;
		if (Abs(Dot(delta, localRight)) <= halfW && Abs(Dot(delta, localUp)) <= halfH)
		{
			polySel->Select(i);
			selectedCount++;
		}
	}

	doc->SetMode(Mpolygons);
	doc->SetActiveObject(poly);

	doc->EndUndo();
	poly->Message(MSG_UPDATE);
	EventAdd();

	return json{
		{"object", ToStd(poly->GetName())},
		{"selected_polygons", selectedCount},
		{"total_polygons", (int)polyCnt}
	};
}
