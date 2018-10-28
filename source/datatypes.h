/*
Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include <vector>
#include "arithmetic.h"
#include "rand.h"

#define INTERSECTION_ERROR_MARGIN FLT_EPSILON*20.0f // fulhack: This won't work for steep angles.

enum class SurfaceType { Diffuse, Specular, Diffuse_Specular, Refractive, COUNT };
enum class DiffuseType { Lambertian, OrenNayar, COUNT };
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

		for (unsigned int i = 0; i < dataSize; ++i)
		{
			data[i] = 0.0;
			rayCount[i] = 0;
		}

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
	float area = 1.0f;

	SceneObject() = default;
	~SceneObject() = default;

	virtual bool Intersects(vec3 rayOrigin, vec3 rayDirection, RayIntersectionInfo& hitInfo) = 0;
	virtual vec3 GetSurfaceNormal(vec3 location, unsigned int index) = 0;
	virtual bool IsLight() { return false; };
	virtual vec3 GetRandomPointOnSurface(UniformRandomGenerator& gen)
	{
		return position;
	}
};

Ray RandomHemisphereRay(vec3& origin, vec3& incomingDirection, vec3& surfaceNormal, UniformRandomGenerator& gen, float& cosTheta);

class Camera
{
protected:
	glm::mat4 viewMatrix;
	float fovPixelScale = 1.0f;
	vec3 position;

public:
	PixelBuffer pixels;

	Camera(unsigned int width, unsigned int height, float fovY)
		: pixels{ width, height }
	{
		// Pre-calculate fov scaling for pixel-to-ray generation
		float halfAngle = (fovY * 0.5f);
		float radians = halfAngle / 180.0f * float(M_PI);
		fovPixelScale = tan(radians);
	};

	~Camera() = default;

	void SetView(vec3 position, vec3 lookAtPosition, vec3 cameraUp = vec3{ 0.0f, 1.0f, 0.0f })
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

		vec3 direction{
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
		for (unsigned int index = 0; index<triangles.size(); ++index)
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

	virtual vec3 GetRandomPointOnSurface(UniformRandomGenerator& gen) override
	{
		float u = gen.RandomFloat();
		float v = gen.RandomFloat();
		float theta = float(M_TWO_PI) * u;
		float phi = acos(2.0f * v - 1.0f);
		float x = position.x + (radius * sin(phi) * cos(theta));
		float y = position.y + (radius * sin(phi) * sin(theta));
		float z = position.z + (radius * cos(phi));
		return vec3(x, y, z);
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
		area = quadDimensions.x * quadDimensions.y;

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

		position += lightDirection * INTERSECTION_ERROR_MARGIN; // We need to offset the center so that we don't collide with it

		AddQuad(p1, p2, p3, p4);
	}

	virtual bool IsLight() override { return true; };

	virtual vec3 GetRandomPointOnSurface(UniformRandomGenerator& gen) override
	{
		float u = gen.RandomFloat();
		float v = gen.RandomFloat();
		vec3 corner = position - xVector / 2.0f - yVector / 2.0f;
		return corner + xVector * u + yVector * v;
	}
};

class CubeObject : public PolygonObject
{
public:
	CubeObject() = default;
	~CubeObject() = default;

	void SetGeometry(vec3 basePosition, vec3 upVector, vec3 sideVector, float width, float depth, float height)
	{
		position = basePosition;

		upVector = glm::normalize(upVector);
		sideVector = glm::normalize(sideVector);

		vec3 localY = upVector;
		vec3 localZ = glm::cross(sideVector, upVector);
		vec3 localX = glm::cross(upVector, localZ);

		float halfWidth = width / 2.0f;
		float halfDepth = depth / 2.0f;

		vec3 base1 = localX * halfWidth + localZ * halfDepth;
		vec3 base2 = localX * halfWidth - localZ * halfDepth;
		vec3 base3 = localX * halfWidth*-1.0f - localZ * halfDepth;
		vec3 base4 = localX * halfWidth*-1.0f + localZ * halfDepth;


		vec3 top1 = base1 + upVector * height;
		vec3 top2 = base2 + upVector * height;
		vec3 top3 = base3 + upVector * height;
		vec3 top4 = base4 + upVector * height;

		base1 += basePosition;
		base2 += basePosition;
		base3 += basePosition;
		base4 += basePosition;

		top1 += basePosition;
		top2 += basePosition;
		top3 += basePosition;
		top4 += basePosition;

		AddQuad(base4, base3, base2, base1);
		AddQuad(top1, top2, top3, top4);
		AddQuad(base1, base2, top2, top1);
		AddQuad(base2, base3, top3, top2);
		AddQuad(base3, base4, top4, top3);
		AddQuad(base4, base1, top1, top4);

	}

};

class Scene
{
protected:
	std::vector<SceneObject*> objects;	// TODO: std::pointer type
	std::vector<SceneObject*> lights;	// TODO: std::pointer type

public:
	ColorDbl backgroundColor = { 0.0f, 0.0f, 0.0f };
	unsigned int LIGHT_SUBSAMPLE_COUNT = 32;

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

	ColorDbl TraceUnlit(Ray ray) const
	{
		RayIntersectionInfo hitInfo;
		if (IntersectRay(ray, hitInfo))
		{
			return ColorDbl(hitInfo.object->color.r, hitInfo.object->color.g, hitInfo.object->color.b);
		}

		return ColorDbl{ 0.0f };
	}

	inline double MaxImportance(ColorDbl& importance)
	{
		return std::max(importance.x, std::max(importance.y, importance.z));
	}

	ColorDbl TraceRay(Ray ray, UniformRandomGenerator& uniformGenerator, unsigned int traceDepth = 5, ColorDbl importance = ColorDbl{ 1.0 })
	{
		RayIntersectionInfo hitInfo;
		if (!IntersectRay(ray, hitInfo))
		{
			return importance * backgroundColor;
		}

		SceneObject& object = *hitInfo.object;
		ColorDbl surfaceColor = object.color;
		vec3 intersectionPoint = ray.origin + ray.direction * hitInfo.hitDistance;
		vec3 normal = object.GetSurfaceNormal(intersectionPoint, hitInfo.elementIndex);

		if (traceDepth == 0 || object.IsLight())
		{
			return importance * object.emission;
		}

		// Lambertian diffuse reflector
		if (object.surfaceType == SurfaceType::Diffuse)
		{
			intersectionPoint += normal * INTERSECTION_ERROR_MARGIN;

			ColorDbl subSampleContribution{ 0.0f };
			ColorDbl directLight{ 0.0f };
			vec3 lightDirection;
			RayIntersectionInfo hitInfo;
			float distanceSq = 0.0;
			float surfaceDot = 0.0f;
			float lightDot = 0.0f;
			double pdf;
			for (SceneObject* lightSource : lights)
			{
				subSampleContribution = ColorDbl{ 0.0f };
				for (unsigned int sample = 0; sample < LIGHT_SUBSAMPLE_COUNT; ++sample)
				{
					lightDirection = lightSource->GetRandomPointOnSurface(uniformGenerator) - intersectionPoint;
					distanceSq = std::max(1.0f, glm::dot(lightDirection, lightDirection));
					lightDirection = glm::normalize(lightDirection);

					// Shadow ray attempt (either a clear path (no collision) or the light is reached)
					Ray shadowRay = Ray(intersectionPoint, lightDirection);
					if (!IntersectRay(shadowRay, hitInfo) || (hitInfo.object == lightSource))
					{
						surfaceDot = std::max(0.0f, glm::dot(normal, lightDirection));
						lightDot = std::max(0.0f, glm::dot(vec3(0.0f, -1.0f, 0.0f), lightDirection*-1.0f));

						// Each subsample has the same multiplication of lightEmission / pdf,
						// it has therefore been moved outside the inner loop to be multiplied only once.
						subSampleContribution += double(surfaceDot * lightDot / distanceSq);
					}
				}

				pdf = 1.0 / (lightSource->area);
				directLight += lightSource->emission/pdf * subSampleContribution/double(LIGHT_SUBSAMPLE_COUNT);
			}

			/*
				Modify importance value
			*/
			double rho = 1.0;
			ColorDbl brdf = surfaceColor * rho / M_PI;
			pdf = 1.0 / (2.0 * M_PI);

			importance = importance * brdf / pdf;

			double p = MaxImportance(importance);
			if (uniformGenerator.RandomDouble(0.0, 1.0) > p)
			{
				return importance * object.emission; // Russian roulette terminated the path
			}
			importance *= 1.0 / p;

			// Indirect lighting
			Ray bouncedRay = RandomHemisphereRay(intersectionPoint, ray.direction, normal, uniformGenerator, surfaceDot);
			ColorDbl indirectLight = TraceRay(bouncedRay, uniformGenerator, --traceDepth, importance);

			// Return all light contribution
			return importance*(object.emission + directLight + indirectLight);
		}
		else if (object.surfaceType == SurfaceType::Specular)
		{
			intersectionPoint += normal * INTERSECTION_ERROR_MARGIN;

			vec3 newDirection = glm::reflect(ray.direction, normal);
			return object.emission + TraceRay(Ray(intersectionPoint, newDirection), uniformGenerator, --traceDepth, importance);
		}
		else if (object.surfaceType == SurfaceType::Refractive)
		{
			vec3& I = ray.direction;
			float n1 = 1.0f;	// air
			float n2 = 1.52f;	// window glass

			// Ray aiming out of the material? (swap normal and coefficients to match ray direction)
			if (glm::dot(normal, I) >= 0)
			{
				normal = normal * -1.0f;
				std::swap(n1, n2);
			}
			vec3 errorMargin = normal * INTERSECTION_ERROR_MARGIN;
			float n = n1 / n2;

			// Determine if the incoming angle is beyond the limit for total internal reflection
			float cosI = glm::dot(I, normal);
			float cos2t = 1.0f - n * n * (1.0f - cosI * cosI);
			if (cos2t < 0.0f)
			{
				// Return total internal reflection
				return importance * (object.emission + TraceRay(Ray{intersectionPoint+errorMargin, glm::reflect(I, normal) }, uniformGenerator, --traceDepth, importance));
			}

			// Use Schlick's approximation of the Fresnel equation to determine reflection and refraction contributions.
			// R determines amount of reflection (1-R determines refraction)
			vec3 tdir = I * n - normal * (cosI * n + sqrt(cos2t));
			float R0 = (n2 - n1) / (n2 + n1);
			R0 *= R0;
			float c = 1.0f - (-cosI);
			float R = R0 + (1 - R0) * c * c * c * c * c;

			// Use probability to determine if the ray is important enough to need a detailed contribution
			if (uniformGenerator.RandomDouble() < MaxImportance(importance))
			{
				// Blend refraction and reflection
				return TraceRay(Ray{intersectionPoint+errorMargin, glm::reflect(I, normal) }, uniformGenerator, traceDepth-1, importance*double(R))
					   + TraceRay(Ray(intersectionPoint-errorMargin, tdir), uniformGenerator, traceDepth - 1, importance*double(1.0-R));
			}
			else
			{
				// Ray has a weak contribution, only calculate one of the paths
				double P = .25 + .5 * R;
				if (uniformGenerator.RandomDouble() > P)
				{
					importance *= R / P;
					return TraceRay(Ray{intersectionPoint+errorMargin, glm::reflect(I, normal)}, uniformGenerator, --traceDepth, importance);
				}
				else
				{
					importance *= (1.0 - R) / (1.0 - P);
					return TraceRay(Ray(intersectionPoint-errorMargin, tdir), uniformGenerator, --traceDepth, importance);
				}
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
