#pragma once
#include "object.h"
#include "core/triangle.h"
#include <vector>

class TriangleMesh : public Object
{
public:
	std::vector<Triangle> triangles;

	TriangleMesh() = default;
	~TriangleMesh() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo)
	{
		float hitDistance = FLOAT_INFINITY;
		if (!aabb.IntersectsRay(rayOrigin, rayDirection, hitDistance))
		{
			hitInfo.Reset();
			return false;
		}

		int elementIndex = 0;
		float nearestDistance = FLOAT_INFINITY;
		for (unsigned int index = 0; index < triangles.size(); ++index)
		{
			if (triangles[index].Intersects(rayOrigin, rayDirection, hitDistance) && hitDistance < nearestDistance)
			{
				nearestDistance = hitDistance;
				elementIndex = index;
			}
		}

		if (nearestDistance < FLOAT_INFINITY)
		{
			hitInfo.object = this;
			hitInfo.elementIndex = elementIndex;
			hitInfo.hitDistance = nearestDistance;
		}
		else
		{
			hitInfo.Reset();
		}

		return (hitInfo.object != nullptr);
	}

	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index)
	{
		return triangles[index].normal;
	}

	// The points must be defined in ccw order in respect to their normal
	void AddQuad(vec3 p1, vec3 p2, vec3 p3, vec3 p4)
	{
		triangles.push_back(Triangle{ p1, p2, p3 });
		triangles.push_back(Triangle{ p3, p4, p1 });
	}

	virtual void UpdateAABB()
	{
		if (triangles.size() > 0)
		{
			aabb = AABB(position, vec3{ 0.0f });

			for (Triangle& t : triangles)
			{
				aabb.Encapsulate(t.vertex0);
				aabb.Encapsulate(t.vertex1);
				aabb.Encapsulate(t.vertex2);
			}
		}
	}
};