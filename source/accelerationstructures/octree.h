/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "../core/math.h"
#include "../core/aabb.h"
#include "../objects/object.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <set>

#define SUBNODE_COUNT 8

class Octree
{
protected:
	bool isSubdivided = false;
	std::vector<Object*> objects;
	Octree* subnodes[SUBNODE_COUNT];

	struct OctreeIntersectionInfo
	{
		Octree* octree = nullptr;
		float hitDistance = FLOAT_INFINITY;
	};

public:
	unsigned int maxCount = 1;
	AABB aabb;

	Octree()
	{}

	~Octree()
	{
		Clear();
	}

	void Clear()
	{
		Merge();
		aabb.Reset();
		objects.clear();
	}

	// Recursive
	void InsertIfOverlaps(Object* newObject)
	{
		if (!newObject) return;

		if (aabb.Overlaps(newObject->aabb))
		{
			AddUnique(newObject);

			if (isSubdivided)
			{
				for (int i = 0; i < SUBNODE_COUNT; ++i)
				{
					subnodes[i]->InsertIfOverlaps(newObject);
				}
			}
			else if (objects.size() > maxCount)
			{
				Subdivide();
			}
		}
	}

	bool HasObject(Object* object)
	{
		return std::find(objects.begin(), objects.end(), object) != objects.end();
	}

	void Fill(std::vector<Object*>& newObjects, unsigned int maxCountPerSubdivision = 1)
	{
		Clear();

		maxCount = maxCountPerSubdivision;
		objects = std::vector<Object*>(newObjects.begin(), newObjects.end());
		for (Object* object : objects)
		{
			aabb.Encapsulate(object->aabb);
		}

		if (objects.size() > maxCount)
		{
			Subdivide();
		}
	}

	bool Intersect(Ray& ray, RayIntersectionInfo& hitInfo) const
	{
		if (aabb.IntersectsRay(ray.origin, ray.direction, hitInfo.hitDistance))
		{
			hitInfo.Reset();
			RayIntersectionInfo newHit;

			if (isSubdivided)
			{
				std::vector<OctreeIntersectionInfo> attempts(8);
				for (int i = 0; i < SUBNODE_COUNT; ++i)
				{
					attempts[i].octree = subnodes[i];
					subnodes[i]->aabb.IntersectsRay(ray.origin, ray.direction, attempts[i].hitDistance);
				}

				std::sort(attempts.begin(), attempts.end(), [](OctreeIntersectionInfo& a, OctreeIntersectionInfo& b) {
					return a.hitDistance <= b.hitDistance;
				});

				for (OctreeIntersectionInfo& info : attempts)
				{
					if (info.octree->Intersect(ray, newHit))
					{
						return true;
					}
				}
				
				return false;
			}
			else
			{
				for (Object* object : objects)
				{
					if (object->Intersects(ray.origin, ray.direction, newHit) && newHit.hitDistance < hitInfo.hitDistance)
					{
						hitInfo = newHit;
					}
				}
			}

			return (hitInfo.object != nullptr);
		}

		return false;
	}

	void PrintDebug(int depth = 0)
	{
		for (int i = 0; i < depth; i++)
		{
			std::cout << "  ";
		}

		std::cout << objects.size() << "\r\n";

		if (isSubdivided)
		{
			for (int i = 0; i < SUBNODE_COUNT; ++i)
			{
				subnodes[i]->PrintDebug(depth + 1);
			}
		}
	}

protected:
	void AddUnique(Object* newObject)
	{
		if (!HasObject(newObject))
		{
			objects.push_back(newObject);
		}
	}

	void Subdivide()
	{
		assert(!isSubdivided);
		isSubdivided = true;

		int i = 0;
		for (int z = 0; z < 2; ++z)
		{
			for (int y = 0; y < 2; ++y)
			{
				for (int x = 0; x < 2; ++x)
				{
					subnodes[i] = new Octree();
					subnodes[i]->aabb.min.x = aabb.min.x + aabb.max.x / 2.0f * x;
					subnodes[i]->aabb.max.x = aabb.min.x + aabb.max.x / 2.0f * (x + 1);

					subnodes[i]->aabb.min.y = aabb.min.y + aabb.max.y / 2.0f * y;
					subnodes[i]->aabb.max.y = aabb.min.y + aabb.max.y / 2.0f * (y + 1);

					subnodes[i]->aabb.min.z = aabb.min.z + aabb.max.z / 2.0f * z;
					subnodes[i]->aabb.max.z = aabb.min.z + aabb.max.z / 2.0f * (z + 1);

					++i;
				}
			}
		}

		// Avoid infinite recursion by testing if subdivision will generate a new subset
		int splitAttemptCount = 0;
		for (Object* object : objects)
		{
			for (int i = 0; i < SUBNODE_COUNT; ++i)
			{
				if (subnodes[i]->aabb.Overlaps(object->aabb))
				{
					splitAttemptCount++;
				}
			}
		}

		if (splitAttemptCount >= objects.size()*8)
		{
			// Failed to subdivide, all children in subdivisions keep repeating all objects
			Merge();
		}
		else
		{
			for (Object* object : objects)
			{
				for (int i = 0; i < SUBNODE_COUNT; ++i)
				{
					subnodes[i]->InsertIfOverlaps(object);
				}
			}
		}
	}

	void Merge()
	{
		if (isSubdivided)
		{
			for (int i = 0; i < SUBNODE_COUNT; ++i)
			{
				delete subnodes[i];
			}
		}

		isSubdivided = false;
	}
};
