/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include <vector>
#include "arithmetic.h"

#define INTERSECTION_ERROR_MARGIN FLT_EPSILON*20.0f // fulhack: This won't work for steep angles.

enum class SurfaceType	   { Diffuse, Specular, Diffuse_Specular, COUNT };
enum class DiffuseType	   { Lambertian, OrenNayar, COUNT };
enum class LightSourceType { Point, Sphere, Rectangle, COUNT };

class PixelBuffer
{
protected:
	std::vector<double> data;

	unsigned int dataSize = 0;
	unsigned int imageWidth = 0;
	unsigned int imageHeight = 0;
	unsigned int imageChannelCount = 0;

	double dx = 0.0;
	double dy = 0.0;
	double aspect = 1.0;

public:
	PixelBuffer(unsigned int width, unsigned int height, unsigned int channels)
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

struct Ray
{
	ColorDbl color;
	vec3 origin;
	vec3 direction;

	Ray() = default;
	Ray(vec3 start, vec3 dir) : origin{ start }, direction{ dir } {}
};

struct RayIntersectionInfo
{
	class SceneObject* object = nullptr;
	unsigned int elementIndex = 0;
	float hitDistance = 0.0f;

	void Reset()
	{
		object = nullptr;
		elementIndex = 0;
		hitDistance = 0.0f;
	}
};

struct Triangle
{
	vec3 vertex0;
	vec3 vertex1;
	vec3 vertex2;

	vec3 normal;

	// The points must be defined in clockwise order in respect to their normal
	Triangle(vec3& v0, vec3& v1, vec3& v2)
		: vertex0{ v0 }, vertex1{ v1 }, vertex2{ v2 }
	{
		vec3 u = v1 - v0;
		vec3 v = v2 - v0;
		normal = glm::normalize(glm::cross(u, v));
	}

	bool Intersects(vec3 rayOrigin, vec3 rayDirection, float& t)
	{
		// Code referenced from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
		const float EPSILON = 0.0000001f;

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
		t -= INTERSECTION_ERROR_MARGIN;
		return (t > EPSILON);
	}
};

struct BBOX
{
	// TODO: Molly
};

class SceneObject
{
public:
	vec3 position;
	ColorDbl color;
	SurfaceType surfaceType = SurfaceType::Diffuse;

	SceneObject() = default;
	~SceneObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual BBOX BoundingBox() = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
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

		vec3 direction {
			xOrigin + x * pixels.deltaX(),	// x
			yOrigin - y * pixels.deltaY(),	// y
			-1.0f,							// z
		};

		// Correct ray direction to match field of view and non-square image output
        direction.x *= fovPixelScale * float(pixels.aspectRatio());
		direction.y *= fovPixelScale;

		// Rotate ray to face camera direction
		direction = vec3(viewMatrix * glm::vec4(direction, 0.0f));

		return Ray(position, glm::normalize(direction));
	}
};

class PolygonObject : public SceneObject
{
public:
	std::vector<Triangle> triangles;

	PolygonObject() = default;
	~PolygonObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo)
	{
		int elementIndex = 0;
		float hitDistance = 0.0f;
		float nearestDistance = FLOAT_INFINITY;
		for (unsigned int index=0; index<triangles.size(); ++index)
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

	virtual BBOX BoundingBox() { return BBOX(); }	// TODO: Molly

	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index)
	{
		return triangles[index].normal;
	}

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

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual BBOX BoundingBox() = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
};

class SphereObject : public ImplicitObject
{
public:
	float radius = 1.0f;

	SphereObject() = default;
	~SphereObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo)
	{
		float radiusSq = radius * radius;

		/*
			Code based on ScratchAPixel guide
		*/
		float t0, t1;

		vec3 L = position - rayOrigin;

		float tca = glm::dot(L, rayDirection);
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

		hitInfo.object = this;
		hitInfo.elementIndex = 0;
		hitInfo.hitDistance = t0 - INTERSECTION_ERROR_MARGIN;

		return true;
	}

	virtual BBOX BoundingBox() { return BBOX(); }	// TODO: Molly

	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index)
	{
		return glm::normalize(location - position);
	}
};

struct LightSource
{
	LightSourceType type = LightSourceType::Rectangle;
	ColorDbl color{ 1.0f };								// alpha is the intensity

	vec3 position;
	vec3 direction;
	vec2 dimensions;

	// TODO: Get random point on light surface (PDF)
	// TODO: Emit ray for photon mapping
};

class Scene
{
protected:
	std::vector<SceneObject*> objects;	// TODO: std::pointer type
	std::vector<LightSource*> lights;	// TODO: std::pointer type

public:
	Scene() = default;
	~Scene()
	{
		for (SceneObject* o : objects) delete o;
		for (LightSource* l : lights)  delete l;
	}

	template<class T>
	T* CreateObject()					// TODO: Return non-owning pointer
	{
		T* newObject = new T();
		objects.push_back(newObject);
		return newObject;
	}

	LightSource* CreateLightSource()
	{
		LightSource* newLight = new LightSource();
		lights.push_back(newLight);
		return newLight;
	}

	bool IntersectRay(Ray& ray, RayIntersectionInfo& hitInfo) const
	{
		hitInfo.object = nullptr;
		hitInfo.elementIndex = 0;
		hitInfo.hitDistance = FLOAT_INFINITY;

		RayIntersectionInfo hitTest;
		for (SceneObject* object : objects)
		{
			if (object->Intersects(ray.origin, ray.direction, hitTest) && hitTest.hitDistance < hitInfo.hitDistance)
			{
				hitInfo = hitTest;
			}
		}

		return (hitInfo.object != nullptr);
	}

	// "Shadow Ray" function
	bool HasClearPathToLight(vec3 shadowPoint, LightSource* light) const
	{
		RayIntersectionInfo hitInfo;
		vec3 lightDirection = glm::normalize(light->position - shadowPoint);
        Ray shadowRay = Ray(shadowPoint, lightDirection);
        if (!IntersectRay(shadowRay, hitInfo))
		{
			// There is definitely a clear path to the light
			return true;
		}
		else
		{
			// We need to determine if the nearest intersected object is
			// closer than the light source.

			vec3 lightVector = light->position - shadowPoint;
			float lightDistanceSq = glm::dot(lightVector, lightVector);
			float hitDistanceSq = hitInfo.hitDistance * hitInfo.hitDistance;

			return lightDistanceSq < hitDistanceSq;
		}
	}

	ColorDbl TraceRay(Ray ray, unsigned int traceDepth = 5) const
	{
		float lightIntensity = 0.0f;
		ColorDbl accumulatedColor{ 0.0f };

		RayIntersectionInfo hitInfo;
		if (IntersectRay(ray, hitInfo))
		{
			const SceneObject& object = *hitInfo.object;
			vec3 intersectionPoint = ray.origin + ray.direction * hitInfo.hitDistance;

			// Diffuse contribution
			switch (object.surfaceType)
			{
			case SurfaceType::Diffuse:
			case SurfaceType::Diffuse_Specular:
                {
                    // Surface emission contribution ("if surface is emissive")
                    ColorDbl e = object.color.a * object.color;
                    //accumulatedColor += e;
                    
                    // Light on surface contribution ("Diffuse")
                    for (LightSource* lightSource : lights)
                    {
                        // "Shadow Ray"
                        if (HasClearPathToLight(intersectionPoint, lightSource))
                        {
                            // Lambertian or OrenNayar?
                            
                            // TODO: Non-uniform BRDF
                            // Light -> BRDF contribution
                            ColorDbl l = lightSource->color.a * lightSource->color;
                            accumulatedColor += ColorDbl{ l.r*e.r, l.g*e.g, l.b*e.b, l.a };
                        }
                    }
                    break;
                }
			case SurfaceType::Specular:
			default:
				// No surface/emission information to take into account.
				break;
			}

			// Specular contribution
			switch (object.surfaceType)
			{
			case SurfaceType::Diffuse_Specular:
			case SurfaceType::Specular:
				if (traceDepth > 0)
				{
					vec3 normal = hitInfo.object->GetSurfaceNormal(intersectionPoint, hitInfo.elementIndex);
					vec3 newDirection = glm::reflect(ray.direction, normal);
					accumulatedColor += TraceRay(Ray(intersectionPoint, newDirection), --traceDepth);
				}
				break;

			case SurfaceType::Diffuse:
			default:
				// No reflection part
				break;
			}
		}
		
		return accumulatedColor;
	}

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
	}
};
