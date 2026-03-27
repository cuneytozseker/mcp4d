#include "raycaster.h"
#include "c4d.h"
#include "lib_collider.h"

using json = nlohmann::json;
using namespace cinema;

static std::string ToStd(const cinema::String& s)
{
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

// Collect all polygon objects in the scene (recursively)
static void CollectPolyObjects(BaseObject* obj, maxon::BaseArray<BaseObject*>& out)
{
	while (obj)
	{
		// Get the deformed/cached version if available
		BaseObject* deformed = obj->GetDeformCache();
		if (deformed)
		{
			CollectPolyObjects(deformed, out);
		}
		else
		{
			BaseObject* cache = obj->GetCache();
			if (cache)
			{
				CollectPolyObjects(cache, out);
			}
			else if (obj->IsInstanceOf(Opolygon))
			{
				out.Append(obj) iferr_ignore("collect poly objects"_s);
			}
		}

		// Recurse into children (but not for caches — they're flat)
		if (!deformed && !obj->GetCache())
			CollectPolyObjects(obj->GetDown(), out);

		obj = obj->GetNext();
	}
}

nlohmann::json Raycast(BaseDocument* doc, int screenX, int screenY)
{
	if (!doc)
		return json{{"error", "No active document"}};

	BaseDraw* bd = doc->GetActiveBaseDraw();
	if (!bd)
		return json{{"error", "No active viewport"}};

	// Convert screen coords to world-space ray.
	// SW() with Z=0 gives a point on the near plane, Z=1 gives a point further along.
	Vector nearWorld = bd->SW(Vector((Float)screenX, (Float)screenY, 0.0));
	Vector farWorld  = bd->SW(Vector((Float)screenX, (Float)screenY, 1000.0));

	Vector rayDir = farWorld - nearWorld;
	Float rayLength = rayDir.GetLength();
	if (rayLength < 0.0001)
		return json{{"error", "Degenerate ray"}};
	rayDir = rayDir / rayLength;

	// Collect all polygon objects
	maxon::BaseArray<BaseObject*> polyObjects;
	CollectPolyObjects(doc->GetFirstObject(), polyObjects);

	if (polyObjects.GetCount() == 0)
		return json{{"error", "No polygon objects in scene"}};

	// Test ray against each object, find closest hit
	AutoAlloc<GeRayCollider> rc;
	if (!rc)
		return json{{"error", "Failed to allocate GeRayCollider"}};

	Float bestDist = MAXVALUE_FLOAT;
	GeRayColResult bestResult;
	BaseObject* bestObj = nullptr;
	Bool found = false;

	for (auto* obj : polyObjects)
	{
		if (!rc->Init(obj, false))
			continue;

		// Transform ray into object-local coordinates
		Matrix mg = obj->GetMg();
		Matrix mgInv = ~mg;
		Vector localRayP   = mgInv * nearWorld;
		Vector localRayDir = mgInv * (nearWorld + rayDir) - localRayP;
		Float localLen = localRayDir.GetLength();
		if (localLen < 0.0001)
			continue;
		localRayDir = localRayDir / localLen;

		if (!rc->Intersect(localRayP, localRayDir, rayLength * 2.0))
			continue;

		GeRayColResult res;
		if (!rc->GetNearestIntersection(&res))
			continue;

		// Transform hit back to world space for distance comparison
		Vector worldHit = mg * res.hitpos;
		Float dist = (worldHit - nearWorld).GetLength();

		if (dist < bestDist)
		{
			bestDist = dist;
			bestResult = res;
			bestObj = obj;
			found = true;
		}
	}

	if (!found)
		return json{{"status", "miss"}, {"message", "Ray did not hit any object"}};

	// Transform hit point and normal to world space
	Matrix mg = bestObj->GetMg();
	Vector worldHit = mg * bestResult.hitpos;
	// Normal needs to be transformed by transpose of inverse (= transpose of ~mg)
	// For orthonormal matrices: just rotate the normal
	Vector worldNormal = (mg * bestResult.f_normal - mg * Vector(0)) ;
	Float nLen = worldNormal.GetLength();
	if (nLen > 0.0001)
		worldNormal = worldNormal / nLen;

	// Find the original (non-cache) object name by walking up
	BaseObject* nameObj = bestObj;
	while (nameObj)
	{
		// If this object has no document, it's likely a cache — go to its origin
		if (nameObj->GetDocument() != nullptr)
			break;
		nameObj = (BaseObject*)nameObj->GetMain();
		if (!nameObj)
		{
			nameObj = bestObj;
			break;
		}
	}

	return json{
		{"hit_point",     VectorToJson(worldHit)},
		{"surface_normal", VectorToJson(worldNormal)},
		{"object_name",   ToStd(nameObj->GetName())},
		{"polygon_index", (int)bestResult.face_id},
		{"distance",      bestDist},
		{"backface",      (bool)bestResult.backface},
		{"barycentric",   VectorToJson(bestResult.barrycoords)}
	};
}
