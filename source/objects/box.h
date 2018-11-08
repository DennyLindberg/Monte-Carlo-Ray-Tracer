/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "mesh.h"

class Box : public TriangleMesh
{
public:
	Box() = default;
	~Box() = default;

	void SetGeometry(vec3 basePosition, vec3 upVector, vec3 sideVector, float width, float depth, float height)
	{
		position = basePosition;

		upVector = glm::normalize(upVector);
		sideVector = glm::normalize(sideVector);

		vec3 localY = upVector;
		vec3 localZ = glm::cross(sideVector, upVector);
		vec3 localX = glm::cross(upVector, localZ);

		float halfWidth = width / 2.0f;
		float halfDepth = depth / 2.0f;

		vec3 base1 = localX * halfWidth + localZ * halfDepth;
		vec3 base2 = localX * halfWidth - localZ * halfDepth;
		vec3 base3 = localX * halfWidth*-1.0f - localZ * halfDepth;
		vec3 base4 = localX * halfWidth*-1.0f + localZ * halfDepth;


		vec3 top1 = base1 + upVector * height;
		vec3 top2 = base2 + upVector * height;
		vec3 top3 = base3 + upVector * height;
		vec3 top4 = base4 + upVector * height;

		base1 += basePosition;
		base2 += basePosition;
		base3 += basePosition;
		base4 += basePosition;

		top1 += basePosition;
		top2 += basePosition;
		top3 += basePosition;
		top4 += basePosition;

		AddQuad(base4, base3, base2, base1);
		AddQuad(top1, top2, top3, top4);
		AddQuad(base1, base2, top2, top1);
		AddQuad(base2, base3, top3, top2);
		AddQuad(base3, base4, top4, top3);
		AddQuad(base4, base1, top1, top4);
	}
};