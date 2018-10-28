/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "../core/math.h"

struct Ray
{
	vec3 origin;
	vec3 direction;

	Ray() = default;
	Ray(vec3 start, vec3 dir) : origin{ start }, direction{ dir } {}
};

struct RayIntersectionInfo
{
	class Object* object = nullptr;
	unsigned int elementIndex = 0;
	float hitDistance = 0.0f;

	void Reset()
	{
		object = nullptr;
		elementIndex = 0;
		hitDistance = 0.0f;
	}
};
