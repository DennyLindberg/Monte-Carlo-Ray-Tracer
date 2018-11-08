/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "../core/math.h"
#include "../core/ray.h"

#include <algorithm>

struct AABB
{
	vec3 center;
	vec3 min;
	vec3 max;

	AABB(vec3 centerPosition = vec3{ 0.0f }, vec3 dimensions = vec3{ 0.0f })
		: center(centerPosition),
		  min(centerPosition-dimensions/2.0f), 
		  max(centerPosition+dimensions/2.0f)
	{}

	void Reset()
	{
		center = vec3{ 0.0f };
		min = vec3{ 0.0f };
		max = vec3{ 0.0f };
	}

	inline bool Contains(vec3& point) const
	{
		return	point.x >= min.x && point.x <= max.x &&
				point.y >= min.y && point.y <= max.y &&
				point.z >= min.z && point.z <= max.z;
	}

	bool Overlaps(const AABB &other) const
	{
		vec3 relMax = max - center;
		vec3 oRelMax = other.max - other.center;

		if (abs(center.x - other.center.x) > (relMax.x + oRelMax.x)) return false;
		if (abs(center.y - other.center.y) > (relMax.y + oRelMax.y)) return false;
		if (abs(center.z - other.center.z) > (relMax.z + oRelMax.z)) return false;

		return true;
	};

	inline bool IntersectsRay(vec3& rayOrigin, vec3& rayDirection, double t) const
	{
		// https://tavianator.com/fast-branchless-raybounding-box-intersections/
		// Naive version
		double tmin = -FLOAT_INFINITY, tmax = FLOAT_INFINITY;
		double t1;
		double t2;

		if (rayDirection.x != 0.0) 
		{
			t1 = (min.x - rayOrigin.x) / rayDirection.x;
			t2 = (max.x - rayOrigin.x) / rayDirection.x;

			tmin = std::max(tmin, std::min(t1, t2));
			tmax = std::min(tmax, std::max(t1, t2));
		}

		if (rayDirection.y != 0.0) 
		{
			t1 = (min.y - rayOrigin.y) / rayDirection.y;
			t2 = (max.y - rayOrigin.y) / rayDirection.y;

			tmin = std::max(tmin, std::min(t1, t2));
			tmax = std::min(tmax, std::max(t1, t2));
		}

		if (rayDirection.z != 0.0) 
		{
			t1 = (min.z - rayOrigin.z) / rayDirection.z;
			t2 = (max.z - rayOrigin.z) / rayDirection.z;

			tmin = std::max(tmin, std::min(t1, t2));
			tmax = std::min(tmax, std::max(t1, t2));
		}

		if (tmax >= tmin)
		{
			t = tmin;
			return true;
		}
		else
		{
			// tmax < 0 => ray intersects, but AABB is behind it
			// tmin > tmax => ray does not intersect AABB
			t = tmax;
			return false;
		}
	}

	inline void Encapsulate(vec3& point)
	{
		min.x = std::min(min.x, point.x);
		min.y = std::min(min.y, point.y);
		min.z = std::min(min.z, point.z);

		max.x = std::max(max.x, point.x);
		max.y = std::max(max.y, point.y);
		max.z = std::max(max.z, point.z);
	}

	inline void Encapsulate(AABB& other)
	{
		Encapsulate(other.min);
		Encapsulate(other.max);
	}
};