/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "mesh.h"
#include "../helpers/OBJ_Loader.h"

bool TriangleMesh::Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo)
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

vec3 TriangleMesh::GetSurfaceNormal(vec3 location, unsigned int index)
{
	return triangles[index].normal;
}

// The points must be defined in ccw order in respect to their normal
void TriangleMesh::AddQuad(vec3 p1, vec3 p2, vec3 p3, vec3 p4)
{
	triangles.push_back(Triangle{ p1, p2, p3 });
	triangles.push_back(Triangle{ p3, p4, p1 });
}

void TriangleMesh::UpdateAABB()
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

void TriangleMesh::LoadMesh(std::string path)
{
	std::cout << "\r\n";

	objl::Loader load;
	if (!load.LoadFile(path))
	{
		std::cout << "Failed to load mesh from path: " + path + "\r\n";
		return;
	}

	// Print loaded meshes
	std::vector<objl::Mesh>& meshes = load.LoadedMeshes;
	std::cout << "Loaded obj: " + path + "\r\n";
	for (objl::Mesh& mesh : meshes)
	{
		std::cout << "   " + mesh.MeshName + "\r\n";
	}

	// Load triangles
	for (objl::Mesh& mesh : meshes)
	{
		for (int k = 0; k < mesh.Indices.size()-3; k++)
		{
			objl::Vector3& p1 = mesh.Vertices[k].Position;
			objl::Vector3& p2 = mesh.Vertices[k+1].Position;
			objl::Vector3& p3 = mesh.Vertices[k+2].Position;

			vec3 v1 = vec3{ p1.X, p1.Y, p1.Z };
			vec3 v2 = vec3{ p2.X, p2.Y, p2.Z };
			vec3 v3 = vec3{ p3.X, p3.Y, p3.Z };

			triangles.push_back(Triangle{ v1+position, v3+position, v2+position });
		}
	}
}
