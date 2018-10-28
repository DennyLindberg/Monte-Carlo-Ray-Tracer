#pragma once
#include "core/math.h"
#include "core/ray.h"
#include "core/randomization.h"

enum class SurfaceType { Diffuse, Specular, Diffuse_Specular, Refractive, COUNT };
enum class DiffuseType { Lambertian, OrenNayar, COUNT };
enum class LightSourceType { Point, Sphere, Rectangle, COUNT };

class Object
{
public:
	vec3 position;
	ColorDbl color = ColorDbl{ 1.0f, 1.0f, 1.0f };
	ColorDbl emission = ColorDbl{ 0.0f, 0.0f, 0.0f };
	SurfaceType surfaceType = SurfaceType::Diffuse;
	float area = 1.0f;

	Object() = default;
	~Object() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
	virtual bool IsLight() { return false; };
	virtual vec3 GetRandomPointOnSurface(UniformRandomGenerator& gen)
	{
		return position;
	}
};

class ImplicitObject : public Object
{
public:
	ImplicitObject() = default;
	~ImplicitObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
};
