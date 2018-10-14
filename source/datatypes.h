/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include <vector>
#include "arithmetic.h"

class PixelBuffer
{
protected:
	std::vector<double> data;

	int dataSize = 0;
	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannelCount = 0;

	double dx = 0.0;
	double dy = 0.0;
	double aspect = 1.0;

public:
	PixelBuffer(int width, int height, int channels)
		: imageWidth{ width }, imageHeight{ height }, imageChannelCount{ channels }
	{
		dataSize = imageWidth * imageHeight * imageChannelCount;
		data.resize(dataSize);

		dx = 2.0 / (double)imageWidth;
		dy = 2.0 / (double)imageHeight;
		aspect = imageWidth / (double)imageHeight;
	}

	~PixelBuffer() = default;

	int size()		const { return dataSize; }
	int numPixels() const { return imageWidth * imageHeight; }
	int width()		const { return imageWidth; }
	int height()	const { return imageHeight; }

	// Returns the dimensions of each pixel in screen space coordinates
	double deltaX() const { return dx; }
	double deltaY() const { return dy; }
	double aspectRatio() const { return aspect; }

	inline double& operator[] (unsigned int i) { return data[i]; }
	void SetPixel(unsigned int x, unsigned int y, double r, double g, double b, double a);
	void SetPixel(unsigned int x, unsigned int y, ColorDbl color);
	unsigned int PixelArrayIndex(unsigned int x, unsigned int y);
};

struct Pixel
{
	ColorDbl color;
	int32 rayCount = 0;

	ColorDbl&& AverageColor()
	{
		ColorDbl temp = color;
		temp /= rayCount;
		return std::move(temp);
	}
};

struct Triangle; // forward declaration
struct Ray
{
	ColorDbl color;

	vec4 start;
	vec4 end;
	Triangle* triangle;

	Ray() = default;

	Ray(vec4 p1, vec4 p2, Triangle* surface = nullptr)
		: start{ p1 }, end{ p2 }, triangle{ surface }
	{}

	vec4 Direction() const { return vec4(glm::normalize(vec3(end - start)), 1.0f); }
};

struct Triangle
{
	vec4 vertex0;
	vec4 vertex1;
	vec4 vertex2;

	vec4 normal;

	// The points must be defined in clockwise order in respect to their normal
	Triangle(vec3& v0, vec3& v1, vec3& v2)
		: vertex0{ v0, 1.0f }, vertex1{ v1, 1.0f }, vertex2{ v2, 1.0f }
	{
		vec3 u = v1 - v0;
		vec3 v = v2 - v0;
		normal = glm::normalize(vec4(glm::cross(u, v), 0.0f));
	}

	bool Intersects(const Ray& ray, float& t)
	{
		// Code referenced from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

		const float EPSILON = 0.0000001f;
		vec3 rayOrigin = ray.start;
		vec3 rayDirection = ray.Direction();

		/*
			Detect if ray is parallel to the triangle
		*/
		vec3 edge1 = vertex1 - vertex0;
		vec3 edge2 = vertex2 - vertex0;
		vec3 h = glm::cross(rayDirection, edge2);
		float a = glm::dot(edge1, h);

		if (abs(a) < EPSILON) // approximately zero
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
		return (t > EPSILON);
	}
};

class Transform
{
public:
	vec4 position;
	vec3 rotation;
	vec3 scale;

	Transform() = default;
	~Transform() = default;
};

struct BBOX
{
	// TODO: Molly
};

class SceneObject
{
public:
	Transform transform;
	ColorDbl color;

	SceneObject() = default;
	~SceneObject() = default;

	virtual bool Intersects(Ray& ray, float& t) = 0;
	virtual BBOX BoundingBox() = 0;
};

class LightObject : public SceneObject
{
public:
	LightObject(ColorDbl newColor, double intensity)
	{
		color = newColor * intensity;
	}

	~LightObject() = default;
};

class Camera
{
protected:
	glm::mat4 viewMatrix;
	float fovPixelScale = 1.0f;
	vec3 position;

public:
	PixelBuffer pixels;

	Camera(unsigned int width, unsigned int height, unsigned int channelsPerPixel, float fovY)
		: pixels{width, height, channelsPerPixel}
	{
		// Pre-calculate fov scaling for pixel-to-ray generation
		float halfAngle = (fovY * 0.5f);
		float radians = halfAngle / 180.0f * float(M_PI);
		fovPixelScale = tan(radians);
	};

	~Camera() = default;

	void SetView(vec3 position, vec3 lookAtPosition, vec3 cameraUp)
	{
		this->position = position;
		viewMatrix = glm::lookAt(position, lookAtPosition, cameraUp);
	}

	inline Ray GetPixelRay(float x, float y) const
	{
		/*
			When we create a ray through a pixel, we get a pinhole camera.
			This gives us a projection by default.
		*/
		const float xOrigin = -1.0f;
		const float yOrigin = 1.0f;

		vec4 direction {
			xOrigin + x * pixels.deltaX(),	// x
			yOrigin - y * pixels.deltaY(),	// y
			-1.0f,							// z
			0.0f							// w - homogenous coordinate
		};

		// Correct ray direction to match field of view and non-square image output
		direction.x *= fovPixelScale * float(pixels.aspectRatio());
		direction.y *= fovPixelScale;

		// Rotate ray to face camera direction
		direction = glm::normalize(viewMatrix * direction);

		return Ray(vec4(position, 1.0f), vec4(position, 1.0f) + direction);
	}
};

class PolygonObject : public SceneObject
{
public:
	std::vector<Triangle> triangles;

	PolygonObject() = default;
	~PolygonObject() = default;

	virtual bool Intersects(Ray& ray, float& t)
	{
		ray.triangle = nullptr;

		t = std::numeric_limits<float>::max();
		float hitDistance = 0.0f;
		for (Triangle& triangle : triangles)
		{
			if (triangle.Intersects(ray, hitDistance) && hitDistance < t)
			{
				t = hitDistance;
				ray.triangle = &triangle;
			}
		}

		return (ray.triangle != nullptr);
	}

	virtual BBOX BoundingBox() { return BBOX(); }	// TODO: Molly

	// The points must be defined in clockwise order in respect to their normal
	void AddQuad(vec3 p1, vec3 p2, vec3 p3, vec3 p4)
	{
		triangles.push_back(Triangle{ p1, p2, p3 });
		triangles.push_back(Triangle{ p3, p4, p1 });
	}
};

class ImplicitObject : public SceneObject
{
public:
	ImplicitObject() = default;
	~ImplicitObject() = default;

	virtual bool Intersects(Ray& ray, float& t) = 0;
	virtual BBOX BoundingBox() = 0;
};

class SphereObject : public ImplicitObject
{
public:
	float radius = 1.0f;

	SphereObject() = default;
	~SphereObject() = default;

	virtual bool Intersects(Ray& ray, float& t)
	{
		vec3 orig = vec3(ray.start);
		vec3 dir = vec3(ray.Direction());
		float radiusSq = radius * radius;

		/*
			Code based on ScratchAPixel guide
		*/
		float t0, t1;

		vec3 L = vec3(transform.position) - orig;

		float tca = glm::dot(L, dir);
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

		t = t0;

		return true;
	}

	virtual BBOX BoundingBox() { return BBOX(); }	// TODO: Molly
};

class PlaneObject : public ImplicitObject
{
public:
	PlaneObject() = default;
	~PlaneObject() = default;

	virtual bool Intersects(Ray& ray, float& t)
	{
		// TODO
	}

	virtual BBOX BoundingBox() { return BBOX(); }	// TODO: Molly
};

class Scene
{
protected:
	std::vector<SceneObject*> objects;	// TODO: std::pointer type

public:
	Scene() = default;
	~Scene()
	{
		for (SceneObject* o : objects)
		{
			delete o;
		}
	}

	template<class T>
	T* CreateObject()					// TODO: Return non-owning pointer
	{
		T* newObject = new T();
		objects.push_back(newObject);
		return newObject;
	}

	bool IntersectRay(Ray& ray, SceneObject*& hitResult, float& t) const
	{
		hitResult = nullptr;
		t = std::numeric_limits<float>::max();

		float hitDistance = 0.0f;
		for (SceneObject* object : objects)
		{
			if (object->Intersects(ray, hitDistance) && hitDistance < t)
			{
				t = hitDistance;
				hitResult = object;
			}
		}

		return (hitResult != nullptr);
	}

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
	}
};
