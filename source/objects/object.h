#pragma once
#include "core/math.h"
#include "core/ray.h"
#include "core/randomization.h"
#include "core/material.h"
#include "core/aabb.h"

class Object
{
public:
	vec3 position;
	Material material;
	float area = 1.0f;
	AABB aabb;

	Object() = default;
	~Object() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
	virtual bool IsLight() { return false; };
	virtual vec3 GetRandomPointOnSurface(UniformRandomGenerator& gen)
	{
		return position;
	}

	virtual double PDF() { return 1.0 / area; }
	virtual void UpdateAABB() {}
};

class ImplicitObject : public Object
{
public:
	ImplicitObject() = default;
	~ImplicitObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
};
