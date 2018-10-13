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

public:
	PixelBuffer(int width, int height, int channels)
		: imageWidth{ width }, imageHeight{ height }, imageChannelCount{ channels }
	{
		dataSize = imageWidth * imageHeight * imageChannelCount;
		data.resize(dataSize);
	}

	~PixelBuffer() = default;

	int size() { return dataSize; }
	int numPixels() { return imageWidth * imageHeight; }
	int width() { return imageWidth; }
	int height() { return imageHeight; }

	// Returns the dimensions of each pixel in screen space coordinates
	double deltaX() { return 2.0f / (double)imageWidth; }
	double deltaY() { return 2.0f / (double)imageHeight; }

	inline double& operator[] (unsigned int i) { return data[i]; }
	void SetPixel(unsigned int x, unsigned int y, double r, double g, double b, double a);
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

	virtual bool Intersects(const Ray& ray, float& t) {}
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
	glm::mat4 perspectiveMatrix;
	glm::mat4 inverseViewProjectionMatrix;

	float focalOffset;
	float fovY;
	float nearClippingPlane;
	float farClippingPlane;

	float pixelWidth;
	float pixelHeight;

	PixelBuffer pixels;

public:
	Camera(unsigned int width, unsigned int height, unsigned int channelsPerPixel, float focalLength = 1.0f, float fov = 90.0f, float nearPlane = 0.01f, float farPlane = 1000.0f)
		: pixels{width, height, channelsPerPixel}, 
		  focalOffset{focalLength}, fovY{fov}, 
		  nearClippingPlane{nearPlane}, farClippingPlane{farPlane},
		  pixelWidth{2.0f/width}, pixelHeight{2.0f/height}			// coordinates are between [-1,1]
	{
		UpdateMatrices();
	};

	~Camera() = default;

	void SetLookAt(vec4 cameraPosition, vec4 targetPosition, vec4 cameraUp)
	{
		transform.position = cameraPosition;
		vec3 position = glm::vec3(cameraPosition);
		vec3 lookDirection = glm::vec3(targetPosition - cameraPosition);
		viewMatrix = glm::lookAt(position, lookDirection, glm::vec3(cameraUp));

		UpdateMatrices();
	}

	Ray EmitRayThroughPixelCenter(unsigned int x, unsigned int y)
	{
		const float originX  = -1.0f;
		const float originY  = 1.0f;
		const float forwardZ = -1.0f;

		float centerX = originX + x * pixelWidth + pixelWidth / 2.0f;
		float centerY = originY - y * pixelHeight - pixelHeight / 2.0f;

		// TODO: Involve projection (replace glm::inverse(viewMatrix) with inverseViewProjectionMatrix)
		/*
			Note that we use glm::inverse because we are not transforming the world to the camera,
			we are transforming the ray into the world.
		*/
		glm::vec4 endPoint = glm::inverse(viewMatrix) * glm::vec4(centerX, centerY, forwardZ, 1.0f);
		return Ray(transform.position, endPoint);
	}


protected:
	void UpdateMatrices()
	{
		perspectiveMatrix = glm::perspectiveFov(
			fovY, 
			float(pixels.width()), float(pixels.height()), 
			nearClippingPlane, farClippingPlane
		);

		inverseViewProjectionMatrix = glm::inverse(perspectiveMatrix*viewMatrix);
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
		// TODO
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
		T* newObject = T();
		objects.push_back(T);
		return T;
	}

	bool IntersectRay(const Ray& ray, SceneObject*& hitResult, float& hitDistance) const
	{
		hitResult = nullptr;
		hitDistance = std::numeric_limits<float>::max();

		for (SceneObject* object : objects)
		{
			if (object->Intersects(ray, hitDistance))
			{
				hitResult = object;
			}
		}

		return (hitResult != nullptr);
	}
};
