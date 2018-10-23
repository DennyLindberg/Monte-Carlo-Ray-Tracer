/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include <vector>
#include "arithmetic.h"
#include "rand.h"

#define INTERSECTION_ERROR_MARGIN FLT_EPSILON*20.0f // fulhack: This won't work for steep angles.

enum class SurfaceType	   { Diffuse, Specular, Diffuse_Specular, Refractive, COUNT };
enum class DiffuseType	   { Lambertian, OrenNayar, COUNT };
enum class LightSourceType { Point, Sphere, Rectangle, COUNT };

class PixelBuffer
{
protected:
	std::vector<double> data;
	std::vector<uint64_t> rayCount;

	unsigned int dataSize = 0;
	unsigned int imageWidth = 0;
	unsigned int imageHeight = 0;

	double dx = 0.0;
	double dy = 0.0;
	double aspect = 1.0;

public:
	PixelBuffer(unsigned int width, unsigned int height)
		: imageWidth{ width }, imageHeight{ height }
	{
		dataSize = imageWidth * imageHeight * 3;
		data.resize(dataSize);
		rayCount.resize(dataSize);

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
	void SetPixel(unsigned int pixelIndex, double r, double g, double b);
	void SetPixel(unsigned int pixelIndex, ColorDbl color);
	void AddRayColor(unsigned int pixelIndex, ColorDbl color);
	uint64_t GetRayCount(unsigned int pixelIndex);
	unsigned int PixelArrayIndex(unsigned int x, unsigned int y);

	ColorDbl GetPixelColor(unsigned int x, unsigned int y);
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
		return (t > EPSILON);
	}
};

class SceneObject
{
public:
	vec3 position;
	ColorDbl color = ColorDbl{ 1.0f, 1.0f, 1.0f };
	ColorDbl emission = ColorDbl{ 0.0f, 0.0f, 0.0f };
	SurfaceType surfaceType = SurfaceType::Diffuse;

	SceneObject() = default;
	~SceneObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
};

Ray GetHemisphereRay(vec3& origin, vec3& incomingDirection, vec3& surfaceNormal, float normalInclination, float azimuth);

class Camera
{
protected:
	glm::mat4 viewMatrix;
	float fovPixelScale = 1.0f;
	vec3 position;

public:
	PixelBuffer pixels;

	Camera(unsigned int width, unsigned int height, float fovY)
		: pixels{width, height}
	{
		// Pre-calculate fov scaling for pixel-to-ray generation
		float halfAngle = (fovY * 0.5f);
		float radians = halfAngle / 180.0f * float(M_PI);
		fovPixelScale = tan(radians);
	};

	~Camera() = default;

	void SetView(vec3 position, vec3 lookAtPosition, vec3 cameraUp = vec3{0.0f, 1.0f, 0.0f})
	{
		this->position = position;
		viewMatrix = glm::inverse(glm::lookAtRH(position, lookAtPosition, cameraUp));
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

	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index)
	{
		return triangles[index].normal;
	}

	// The points must be defined in ccw order in respect to their normal
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
		if (radius < FLT_EPSILON) return false;

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
		hitInfo.hitDistance = t0;

		return true;
	}

	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index)
	{
		return glm::normalize(location - position);
	}
};

class LightQuad : public PolygonObject
{
public:
	vec3 normal;
	vec3 xVector;
	vec3 yVector;

	LightQuad(ColorDbl lightEmission = ColorDbl{ 1.0 })
	{
		emission = lightEmission;
	}

	~LightQuad() = default;

	void SetGeometry(vec3 centerPosition, vec3 lightDirection, vec3 sideDirection, vec2 quadDimensions)
	{
		normal = glm::normalize(lightDirection);
		position = centerPosition;

		yVector = glm::normalize(glm::cross(sideDirection, normal));
		xVector = glm::normalize(glm::cross(yVector, normal));

		xVector *= quadDimensions.x / 2.0f;
		yVector *= quadDimensions.y / 2.0f;

		vec3 p1 = position - xVector - yVector;
		vec3 p2 = position - xVector + yVector;
		vec3 p3 = position + xVector + yVector;
		vec3 p4 = position + xVector - yVector;

		position += lightDirection*INTERSECTION_ERROR_MARGIN; // We need to offset the center so that we don't collide with it

		AddQuad(p1, p2, p3, p4);
	}
};

class Scene
{
protected:
	std::vector<SceneObject*> objects;	// TODO: std::pointer type
	std::vector<SceneObject*> lights;	// TODO: std::pointer type
	UniformRandomGenerator uniformGenerator;

public:
	ColorDbl backgroundColor = {0.0f, 0.0f, 0.0f};

	Scene() = default;
	~Scene()
	{
		for (SceneObject* o : objects) delete o;
		// do not delete lights, they are duplicates of objects which are emissive
	}

	template<class T>
	T* CreateObject()					// TODO: Return non-owning pointer
	{
		T* newObject = new T();
		objects.push_back(newObject);
		return newObject;
	}

	void CacheLights()
	{
		lights.clear();
		for (SceneObject* o : objects)
		{
			if (o->emission.r > 0.0 || o->emission.g > 0.0 || o->emission.b > 0.0)
			{
				lights.push_back(o);
			}
		}
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
	bool HasClearPathToLight(vec3 shadowPoint, SceneObject* light) const
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

	ColorDbl TraceUnlit(Ray ray) const
	{
		RayIntersectionInfo hitInfo;
		if (IntersectRay(ray, hitInfo))
		{
			return ColorDbl(hitInfo.object->color.r, hitInfo.object->color.g, hitInfo.object->color.b);
		}

		return ColorDbl{ 0.0f };
	}

	ColorDbl TraceRay(Ray ray, unsigned int traceDepth = 5)
	{
		/*
			Lecture 11 - "We should not stop the ray after a fixed number of iterations. Terminate on light sources or lambertian/ON reflectors."
			Lecture 13 - "We terminate the ray at Yn"
			
			=> We terminate using a max ray depth Yn
		*/
		if (traceDepth == 0)
		{
			return ColorDbl{ 0.0 };
		}

		RayIntersectionInfo hitInfo;
		if (!IntersectRay(ray, hitInfo))
		{
			return backgroundColor;
		}

		SceneObject& object = *hitInfo.object;
		vec3 intersectionPoint = ray.origin + ray.direction * hitInfo.hitDistance;
		vec3 normal = object.GetSurfaceNormal(intersectionPoint, hitInfo.elementIndex);
		intersectionPoint += normal*INTERSECTION_ERROR_MARGIN;

		// Helper function for element wise multiplication
		auto multiply = [](const ColorDbl& u, const ColorDbl& v) {
			return ColorDbl{ u.r*v.r, u.g*v.g, u.b*v.b };
		};

		if (object.surfaceType == SurfaceType::Diffuse)
		{
			// Light on surface contribution ("Diffuse")
			const ColorDbl& f = object.color;
			ColorDbl lightContribution{ 0.0f };
			for (SceneObject* lightSource : lights)
			{
				// "Shadow Ray"
				// TODO: Pick random point on light, then store info for later use
				if (HasClearPathToLight(intersectionPoint, lightSource))
				{
					// Lambertian or OrenNayar?
					// TODO: Non-uniform BRDF
					// Light -> BRDF contribution

					vec3 lightVector = lightSource->position - intersectionPoint;
					const ColorDbl& L = lightSource->color;

					double cos_a_max = sqrt(1 - 1.0 / glm::dot(lightVector, lightVector));
					double omega = M_TWO_PI * (1 - cos_a_max);
					double dotAngle = double(glm::dot(normal, glm::normalize(lightVector)));
					lightContribution += multiply(L * dotAngle /**omega*/, f) * M_ONE_OVER_PI;
				}
			}

			// Bounced ray
			float inclinationAngle = uniformGenerator.RandomFloat(0, float(M_PI_HALF));
			float azimuthAngle = uniformGenerator.RandomFloat(0, float(M_TWO_PI));
			Ray newRay = GetHemisphereRay(intersectionPoint, ray.direction, normal, inclinationAngle, azimuthAngle);

			return object.emission + lightContribution + multiply(f, TraceRay(newRay, --traceDepth));
		}
		else if (object.surfaceType == SurfaceType::Specular)
		{
			vec3 normal = hitInfo.object->GetSurfaceNormal(intersectionPoint, hitInfo.elementIndex);
			vec3 newDirection = glm::reflect(ray.direction, normal);
			return object.emission + TraceRay(Ray(intersectionPoint, newDirection), --traceDepth);
		}
		else if (object.surfaceType == SurfaceType::Refractive)
		{
			/*
				This code is currently broken and behaves like some weird mirror with miscoloring.
				TODO: Make it actually refractive
			*/

			// Perfect reflection
			vec3 I = ray.direction*-1.0f;
			vec3 normal = hitInfo.object->GetSurfaceNormal(intersectionPoint, hitInfo.elementIndex);

			float Nair = 1.0f;
			float Nobject = 1.52f; // window glass

			bool intoObject = glm::dot(ray.direction, normal) > 0.0f;
			float Nr = intoObject? Nair / Nobject : Nobject / Nair;
			vec3 N = intoObject ? normal : normal * -1.0f;

			// https://www.cs.rpi.edu/~cutler/classes/advancedgraphics/F05/lectures/13_ray_tracing.pdf
			// [Nr * cos(w) - sqrt(1-Nr^2*(1-cos(w)^2)]*N - Nr * I
			float NrCos = Nr * glm::dot(N, I);
			float innerRoot = 1 - Nr*Nr*(1 - NrCos * NrCos);

			const ColorDbl& f = object.color;
			vec3 reflectDirection = glm::reflect(ray.direction, N);
			Ray reflection{ intersectionPoint, reflectDirection };
			if (innerRoot < 0.0f)
			{
				// Total internal reflection
				return object.emission + multiply(f, TraceRay(reflection, --traceDepth));
			}
			else
			{
				vec3 T = (NrCos - sqrt(innerRoot)) * N - Nr * I;
				Ray refraction{ intersectionPoint, glm::normalize(T) };
				return TraceRay(refraction, --traceDepth);

				// Approximate Fresnel in specular reflection
				// https://en.wikipedia.org/wiki/Schlick%27s_approximation
				
				// FRESNELL / SCHLICK
				float w = glm::dot(I, N);
				float c = 1 - w;
				
				float R0; // Reflection coefficient ((n1 - n2)/(n1 + n2))^2
				if (intoObject) R0 = ((Nair - Nobject) / (Nair + Nobject));
				else			R0 = ((Nobject - Nair) / (Nair + Nobject));
				R0 *= R0;

				// R(w) = R0 + (1 - R0)(1-cos(w))^5
				double R = double(R0 + (1 - R0)*c*c*c*c*c);
				double Tr = 1.0f-R;
				
				traceDepth--;
				return TraceRay(refraction, traceDepth) * R;// + TraceRay(reflection, traceDepth) * Tr;
			}
		}

		// Failed to get a color, return black
		return ColorDbl{ 0.0 };
	}

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
	}
};
