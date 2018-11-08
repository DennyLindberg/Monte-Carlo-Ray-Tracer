/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "object.h"
#include "../core/triangle.h"
#include <vector>

class TriangleMesh : public Object
{
public:
	std::vector<Triangle> triangles;

	TriangleMesh() = default;
	~TriangleMesh() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo);

	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index);

	// The points must be defined in ccw order in respect to their normal
	void AddQuad(vec3 p1, vec3 p2, vec3 p3, vec3 p4);

	virtual void UpdateAABB();

	void LoadMesh(std::string path);
};