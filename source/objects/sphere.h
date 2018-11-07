#pragma once
#include "object.h"

class SphereObject : public ImplicitObject
{
public:
	float radius = 1.0f;

	SphereObject() = default;
	~SphereObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo)
	{
		if (radius < FLT_EPSILON) return false;

		float radiusSq = radius * radius;

		/*
		Code based on ScratchAPixel guide
		*/
		float t0, t1;

		vec3 L = position - rayOrigin;

		float tca = glm::dot(L, rayDirection);
		if (tca < 0) return false;

		float distanceSq = glm::dot(L, L) - tca * tca;
		if (distanceSq > radiusSq) return false;

		float thc = sqrt(radiusSq - distanceSq);
		t0 = tca - thc;
		t1 = tca + thc;

		if (t0 > t1) std::swap(t0, t1);

		if (t0 < 0)
		{
			t0 = t1; // if t0 is negative, let's use t1 instead 
			if (t0 < 0) return false; // both t0 and t1 are negative 
		}

		hitInfo.object = this;
		hitInfo.elementIndex = 0;
		hitInfo.hitDistance = t0;

		return true;
	}

	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index)
	{
		return glm::normalize(location - position);
	}

	virtual vec3 GetRandomPointOnSurface(UniformRandomGenerator& gen) override
	{
		float u = gen.RandomFloat();
		float v = gen.RandomFloat();
		float theta = float(M_TWO_PI) * u;
		float phi = acos(2.0f * v - 1.0f);
		float x = position.x + (radius * sin(phi) * cos(theta));
		float y = position.y + (radius * sin(phi) * sin(theta));
		float z = position.z + (radius * cos(phi));
		return vec3(x, y, z);
	}

	virtual void UpdateAABB() 
	{
		aabb = AABB(position, vec3{ radius });
	}
};