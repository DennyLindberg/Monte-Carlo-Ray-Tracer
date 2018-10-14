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

struct Triangle
{
	vec4 vertex1;
	vec4 vertex2;
	vec4 vertex3;

	vec4 normal;

	Triangle(vec3& v1, vec3& v2, vec3& v3)
		: vertex1{ v1, 1.0f }, vertex2{ v2, 1.0f }, vertex3{ v3, 1.0f }
	{
		vec3 u = v2 - v1;
		vec3 v = v3 - v1;
		normal = vec4(glm::cross(u, v), 0.0f);
	}
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

struct Ray
{
	ColorDbl color;

	vec4 start;
	vec4 end;
	Triangle* triangle;

	Ray() = default;

	Ray(vec4 p1, vec4 p2, Triangle* surface = nullptr)
		: start{p1}, end{p2}, triangle{surface}
	{}

	vec4 Direction() const { return vec4(glm::normalize(vec3(end - start)), 1.0f); }
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

	virtual bool Intersects(const Ray& ray, float& t) { return false; }
	virtual BBOX BoundingBox() { return BBOX(); }

	glm::mat4 ModelTransform()
	{
		// do stuff
	}
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

class Camera : public SceneObject
{
protected:
	glm::mat4 viewMatrix;
	float fovPixelScale = 1.0f;

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

	void LookAt(vec4 targetPosition, vec4 cameraUp)
	{
		viewMatrix = glm::lookAt(glm::vec3(transform.position), vec3(targetPosition), glm::vec3(cameraUp));
	}

	Ray EmitRayThroughPixelCenter(unsigned int x, unsigned int y) const
	{
		return GetPixelRay(x + 0.5f, y + 0.5f);
	}


protected:
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

		return Ray(transform.position, transform.position + direction);
	}
};

class PolygonObject : public SceneObject
{
public:
	std::vector<Triangle> triangles;

	PolygonObject() = default;
	~PolygonObject() = default;

	virtual bool Intersects(const Ray& ray, float& t)
	{
		// TODO
	}

	virtual BBOX BoundingBox() { return BBOX(); }	// TODO: Molly
};

class ImplicitObject : public SceneObject
{
public:
	ImplicitObject() = default;
	~ImplicitObject() = default;

	virtual bool Intersects(const Ray& ray, float& t) = 0;
	virtual BBOX BoundingBox() = 0;
};

class SphereObject : public ImplicitObject
{
public:
	float radius = 1.0f;

	SphereObject() = default;
	~SphereObject() = default;

	virtual bool Intersects(const Ray& ray, float& t)
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

	virtual bool Intersects(const Ray& ray, float& t)
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

	bool IntersectRay(const Ray& ray, SceneObject*& hitResult, float& hitDistance) const
	{
		hitResult = nullptr;
		hitDistance = std::numeric_limits<float>::max();

		float t = 0.0f;
		for (SceneObject* object : objects)
		{
			if (object->Intersects(ray, t) && t < hitDistance)
			{
				hitDistance = t;
				hitResult = object;
			}
		}

		return (hitResult != nullptr);
	}
};
