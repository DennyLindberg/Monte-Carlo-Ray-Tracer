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

	vec4* start;
	vec4* end;
	Triangle* triangle;
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

	virtual bool Intersects(Ray& ray) {}
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
public:
	float focalLength = 1.0f;
	PixelBuffer pixels;

	Camera() = default;
	~Camera() = default;

	glm::mat4 ViewProjectionMatrix()
	{
		// do stuff
	}
};

class PolygonObject : public SceneObject
{
public:
	std::vector<Triangle> triangles;

	PolygonObject() = default;
	~PolygonObject() = default;

	virtual bool Intersects(Ray& ray) 
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

	virtual bool Intersects(Ray& ray) = 0;
	virtual BBOX BoundingBox() = 0;
};

class SphereObject : public ImplicitObject
{
public:
	SphereObject() = default;
	~SphereObject() = default;

	virtual bool Intersects(Ray& ray)
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

	virtual bool Intersects(Ray& ray)
	{
		// TODO
	}

	virtual BBOX BoundingBox() { return BBOX(); }	// TODO: Molly
};
