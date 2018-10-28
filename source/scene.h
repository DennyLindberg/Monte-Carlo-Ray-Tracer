/*
Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include <vector>
#include "core/math.h"
#include "core/randomization.h"
#include "core/camera.h"
#include "objects/object.h"
#include "objects/mesh.h"
#include <algorithm>


class Scene
{
protected:
	std::vector<Object*> objects;	// TODO: std::pointer type
	std::vector<Object*> lights;	// TODO: std::pointer type

	struct Ray RandomHemisphereRay(vec3& origin, vec3& incomingDirection, vec3& surfaceNormal, UniformRandomGenerator& gen, float& cosTheta);

public:
	ColorDbl backgroundColor = { 0.0f, 0.0f, 0.0f };
	unsigned int LIGHT_SUBSAMPLE_COUNT = 32;

	Scene() = default;
	~Scene();

	template<class T>
	T* CreateObject()					// TODO: Return non-owning pointer
	{
		T* newObject = new T();
		objects.push_back(newObject);
		return newObject;
	}

	void CacheLights();

	bool IntersectRay(Ray& ray, RayIntersectionInfo& hitInfo) const;

	ColorDbl TraceUnlit(Ray ray) const;

	inline double MaxImportance(ColorDbl& importance)
	{
		return std::max(importance.x, std::max(importance.y, importance.z));
	}

	ColorDbl TraceRay(Ray ray, UniformRandomGenerator& uniformGenerator, unsigned int traceDepth = 5, ColorDbl importance = ColorDbl{ 1.0 });

	virtual void MoveCameraToRecommendedPosition(Camera& camera);
};


class HexagonScene : public Scene
{
protected:
	TriangleMesh* ceiling;
	TriangleMesh* floor;
	TriangleMesh* walls1;
	TriangleMesh* walls2;
	TriangleMesh* walls3;

public:
	HexagonScene();
	~HexagonScene() = default;

	virtual void MoveCameraToRecommendedPosition(Camera& camera);
	void AddExampleSpheres(float radius = 1.5f);
	void AddExampleLight(ColorDbl lightColor, bool usePoint = false);
};

class CornellBoxScene : public Scene
{
protected:
	float halfLength = 0.0f;
	float halfWidth = 0.0f;
	float halfHeight = 0.0f;

	TriangleMesh* leftWall;
	TriangleMesh* rightWall;
	TriangleMesh* whiteSegments;

public:
	CornellBoxScene(float length, float width, float height);
	~CornellBoxScene() = default;

	virtual void MoveCameraToRecommendedPosition(Camera& camera);
	void AddExampleSpheres(float radius = 1.5f);
	void AddExampleLight(ColorDbl lightColor, bool usePoint = false);
};
