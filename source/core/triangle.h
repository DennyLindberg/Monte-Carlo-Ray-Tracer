/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "../core/math.h"

struct Triangle
{
	vec3 vertex0;
	vec3 vertex1;
	vec3 vertex2;

	vec3 normal;

	// The points must be defined in clockwise order in respect to their normal
	Triangle(vec3& v0, vec3& v1, vec3& v2)
		: vertex0{ v0 }, vertex1{ v1 }, vertex2{ v2 }
	{
		vec3 u = v1 - v0;
		vec3 v = v2 - v0;
		normal = glm::normalize(glm::cross(u, v));
	}

	bool Intersects(vec3 rayOrigin, vec3 rayDirection, float& t)
	{
		// Code referenced from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
		/*
			Detect if ray is parallel to the triangle
		*/
		vec3 edge1 = vertex1 - vertex0;
		vec3 edge2 = vertex2 - vertex0;
		vec3 h = glm::cross(rayDirection, edge2);
		float a = glm::dot(edge1, h);

		if (abs(a) < FLT_EPSILON) // approximately zero
		{
			return false;
		}

		/*
			Detect if ray is inside the triangle
		*/
		float f = 1.0f / a;
		vec3 s = rayOrigin - vec3(vertex0);
		float u = f * (glm::dot(s, h));
		if (u < 0.0f || u > 1.0f)
		{
			return false;
		}

		vec3 q = glm::cross(s, edge1);
		float v = f * glm::dot(rayDirection, q);
		if (v < 0.0f || u + v > 1.0f)
		{
			return false;
		}

		// Determine where and if we intersected
		t = f * glm::dot(edge2, q);
		return (t > FLT_EPSILON);
	}
};
