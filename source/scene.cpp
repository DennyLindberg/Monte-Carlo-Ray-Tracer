/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "scene.h"
#include "core/ray.h"
#include "objects/sphere.h"
#include "objects/box.h"
#include "objects/light.h"

Ray Scene::RandomHemisphereRay(vec3& origin, vec3& incomingDirection, vec3& surfaceNormal, UniformRandomGenerator& gen, float& cosTheta)
{
	vec3 Nx, Nz, Ny = surfaceNormal;
	if (fabs(Ny.x) > fabs(Ny.y)) Nx = vec3(Ny.z, 0, -Ny.x);
	else Nx = vec3(0, -Ny.z, Ny.y);
	Nx = glm::normalize(Nx);
	Nz = glm::normalize(glm::cross(Ny, Nx));

	cosTheta = gen.RandomFloat(0.0f, 1.0f);
	float sinTheta = sqrtf(1 - cosTheta * cosTheta);
	float phi = float(M_TWO_PI) * gen.RandomFloat(0.0f, 1.0f);
	vec3 sample(sinTheta * cosf(phi), cosTheta, sinTheta * sinf(phi));
	vec3 sample_transformed = vec3(
		sample.x * Nx.x + sample.y * Ny.x + sample.z * Nz.x,
		sample.x * Nx.y + sample.y * Ny.y + sample.z * Nz.y,
		sample.x * Nx.z + sample.y * Ny.z + sample.z * Nz.z
	);

	return Ray(origin, sample_transformed);
}

Scene::~Scene()
{
	for (Object* o : objects) delete o;
	// do not delete lights, they are duplicates of objects which are emissive
}

void Scene::PrepareForRayTracing()
{
	// Cache lights
	lights.clear();
	for (Object* o : objects)
	{
		Material& m = o->material;
		if (m.emission.r > 0.0 || m.emission.g > 0.0 || m.emission.b > 0.0)
		{
			lights.push_back(o);
		}
	}

	// Update AABBs
	for (Object* o : objects)
	{
		o->UpdateAABB();
	}

	// Generate Octree
	octree.Fill(objects);
}


bool Scene::IntersectRay(Ray& ray, RayIntersectionInfo& hitInfo) const
{
	//return octree.Intersect(ray, hitInfo);

	hitInfo.object = nullptr;
	hitInfo.elementIndex = 0;
	hitInfo.hitDistance = FLOAT_INFINITY;

	RayIntersectionInfo hitTest;
	for (Object* object : objects)
	{
		if (object->Intersects(ray.origin, ray.direction, hitTest) && hitTest.hitDistance < hitInfo.hitDistance)
		{
			hitInfo = hitTest;
		}
	}

	return (hitInfo.object != nullptr);
}

ColorDbl Scene::TraceUnlit(Ray ray) const
{
	RayIntersectionInfo hitInfo;
	if (IntersectRay(ray, hitInfo))
	{
		Material& material = hitInfo.object->material;
		return ColorDbl(material.color.r, material.color.g, material.color.b);
	}

	return ColorDbl{ 0.0f };
}

ColorDbl Scene::TraceRay(Ray ray, UniformRandomGenerator& uniformGenerator, unsigned int traceDepth, ColorDbl importance)
{
	RayIntersectionInfo hitInfo;
	if (!IntersectRay(ray, hitInfo))
	{
		return importance * backgroundColor;
	}

	Object& object = *hitInfo.object;
	Material& surface = object.material;
	ColorDbl surfaceColor = surface.color;
	vec3 intersectionPoint = ray.origin + ray.direction * hitInfo.hitDistance;
	vec3 normal = object.GetSurfaceNormal(intersectionPoint, hitInfo.elementIndex);

	if (traceDepth == 0 || object.IsLight())
	{
		return importance * surface.emission;
	}

	// Lambertian diffuse reflector
	if (surface.type == SurfaceType::Diffuse)
	{
		intersectionPoint += normal * INTERSECTION_ERROR_MARGIN;

		ColorDbl subSampleContribution{ 0.0f };
		ColorDbl directLight{ 0.0f };
		vec3 lightDirection;
		RayIntersectionInfo hitInfo;
		float distanceSq = 0.0;
		float surfaceDot = 0.0f;
		float lightDot = 0.0f;
		for (Object* lightSource : lights)
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
			directLight += lightSource->material.emission / lightSource->PDF() * subSampleContribution / double(LIGHT_SUBSAMPLE_COUNT);
		}

		/*
			Modify importance value
		*/
		Ray bouncedRay = RandomHemisphereRay(intersectionPoint, ray.direction, normal, uniformGenerator, surfaceDot);
		double hemispherePDF = 1.0 / (2.0 * M_PI);
		double BRDF = surface.BRDF(ray.direction, bouncedRay.direction, normal);
		importance = importance/hemispherePDF * surface.color*BRDF;

		double p = MaxImportance(importance);
		if (uniformGenerator.RandomDouble(0.0, 1.0) > p)
		{
			return importance * surface.emission; // Russian roulette terminated the path
		}
		importance *= 1.0 / p;

		// Indirect lighting
		ColorDbl indirectLight = TraceRay(bouncedRay, uniformGenerator, --traceDepth, importance);

		// Return all light contribution
		return importance * (surface.emission + directLight + indirectLight);
	}
	else if (surface.type == SurfaceType::Specular)
	{
		intersectionPoint += normal * INTERSECTION_ERROR_MARGIN;

		vec3 newDirection = glm::reflect(ray.direction, normal);
		return surface.emission + TraceRay(Ray(intersectionPoint, newDirection), uniformGenerator, --traceDepth, importance);
	}
	else if (surface.type == SurfaceType::Refractive)
	{
		vec3& I = ray.direction;
		float n1 = 1.0f;					// air
		float n2 = surface.refractiveIndex;

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
			return importance * (surface.emission + TraceRay(Ray{ intersectionPoint + errorMargin, glm::reflect(I, normal) }, uniformGenerator, --traceDepth, importance));
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
			return TraceRay(Ray{ intersectionPoint + errorMargin, glm::reflect(I, normal) }, uniformGenerator, traceDepth - 1, importance*double(R))
				+ TraceRay(Ray(intersectionPoint - errorMargin, tdir), uniformGenerator, traceDepth - 1, importance*double(1.0 - R));
		}
		else
		{
			// Ray has a weak contribution, only calculate one of the paths
			double P = .25 + .5 * R;
			if (uniformGenerator.RandomDouble() < P)
			{
				importance *= R / P;
				return TraceRay(Ray{ intersectionPoint + errorMargin, glm::reflect(I, normal) }, uniformGenerator, --traceDepth, importance);
			}
			else
			{
				importance *= (1.0 - R) / (1.0 - P);
				return TraceRay(Ray(intersectionPoint - errorMargin, tdir), uniformGenerator, --traceDepth, importance);
			}
		}
	}

	// Failed to get a color, return black
	return ColorDbl{ 0.0 };
}

void Scene::MoveCameraToRecommendedPosition(Camera& camera)
{
	camera.SetView(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
}


HexagonScene::HexagonScene()
{
	ceiling = CreateObject<TriangleMesh>();
	floor = CreateObject<TriangleMesh>();
	walls1 = CreateObject<TriangleMesh>();
	walls2 = CreateObject<TriangleMesh>();
	walls3 = CreateObject<TriangleMesh>();

	ceiling->material.color = ColorDbl(0.2);
	floor->material.color = ColorDbl(0.2);
	walls1->material.color = ColorDbl(0.2, 0.01, 0.01);
	walls2->material.color = ColorDbl(0.01, 0.2, 0.01);
	walls3->material.color = ColorDbl(0.2);

	// Ceiling corners
	//	   {  width, height, length }
	vec3 c1{ 0.0f,   5.0f,  -3.0f };
	vec3 c2{ 6.0f,   5.0f,   0.0f };
	vec3 c3{ 6.0f,   5.0f,  10.0f };
	vec3 c4{ 0.0f,   5.0f,  13.0f };
	vec3 c5{ -6.0f,   5.0f,  10.0f };
	vec3 c6{ -6.0f,   5.0f,   0.0f };

	// Floor corners (same, but height is flipped)
	vec3 f1 = c1; f1.y *= -1.0f;
	vec3 f2 = c2; f2.y *= -1.0f;
	vec3 f3 = c3; f3.y *= -1.0f;
	vec3 f4 = c4; f4.y *= -1.0f;
	vec3 f5 = c5; f5.y *= -1.0f;
	vec3 f6 = c6; f6.y *= -1.0f;

	floor->AddQuad(f1, f4, f3, f2);
	floor->AddQuad(f1, f6, f5, f4);

	ceiling->AddQuad(c1, c2, c3, c4);
	ceiling->AddQuad(c4, c5, c6, c1);

	walls1->AddQuad(f2, f3, c3, c2);
	walls2->AddQuad(f1, f2, c2, c1);
	walls3->AddQuad(f3, f4, c4, c3);

	walls2->AddQuad(f5, f6, c6, c5);
	walls3->AddQuad(f4, f5, c5, c4);
	walls3->AddQuad(f6, f1, c1, c6);
}

void HexagonScene::MoveCameraToRecommendedPosition(Camera& camera)
{
	camera.SetView(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 10.0f), vec3(0.0f, 1.0f, 0.0f));
}

void HexagonScene::AddExampleObjects(float radius)
{
	SphereObject* leftSphere = CreateObject<SphereObject>();
	SphereObject* middleSphere = CreateObject<SphereObject>();
	SphereObject* rightSphere = CreateObject<SphereObject>();

	leftSphere->material.type = SurfaceType::Diffuse;
	middleSphere->material.type = SurfaceType::Specular;
	rightSphere->material.type = SurfaceType::Refractive;


	leftSphere->radius = radius;
	middleSphere->radius = radius;
	rightSphere->radius = radius;

	float widthOffset = 6.0f - radius;
	float heightOffset = 5.0f - radius;
	leftSphere->position = vec3(3.0, 2.0, 10.0f);
	middleSphere->position = vec3(-3.0f, 0.0, 8.0f);
	rightSphere->position = vec3(1.0f, -3.0, 6.0f);

	leftSphere->material.color = ColorDbl(0.5);
	middleSphere->material.color = ColorDbl(0.5);
	rightSphere->material.color = ColorDbl(0.5);

	// box1
	Box* box = CreateObject<Box>();
	box->SetGeometry({ 3.0, -5.0, 10.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 2.0f, 2.0f, 7.0f - radius);
	box->material.color = ColorDbl(0.01, 0.3, 0.8);
	box->material.type = SurfaceType::Diffuse;

	// box2
	Box* box2 = CreateObject<Box>();
	box2->SetGeometry({ -3.0, -5.0, 8.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 2.0f, 2.0f, 5.0f - radius);
	box2->material.color = ColorDbl(0.8, 0.4, 0.01);
	box2->material.type = SurfaceType::Refractive;

	// box2 - middle
	Box* box3 = CreateObject<Box>();
	box3->SetGeometry({ 1.0, -5.0, 6.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 4.0f, 4.0f, 2.0f - radius);
	box3->material.color = ColorDbl(0.5, 0.2, 0.8);
	box3->material.type = SurfaceType::Diffuse;

	// 1.15 = unknown 
	// 1.31 = ice
	// 1.52 = window glass 
	// 2.417 = diamond
	rightSphere->material.refractiveIndex = 1.52f;
	box2->material.refractiveIndex = 1.52f;
}

void HexagonScene::AddExampleLight(ColorDbl lightColor, bool usePoint)
{
	vec3 roofCenter{ 0.0f, 5.0f - 0.001f, 8.0f };

	if (usePoint)
	{
		SphereObject* pointLight = CreateObject<SphereObject>();
		pointLight->radius = 0.0f;
		pointLight->material.color = lightColor;
		pointLight->material.emission = lightColor;
		pointLight->position = roofCenter;
	}
	else
	{
		Light* light = CreateObject<Light>();
		light->SetGeometry(roofCenter,				// position
			{ 0.0f, -1.0f, 0.0f },	// direction
			{ 1.0f, 0.0f, 0.0f },	// side
			{ 1.0f, 1.0f });		// dimensions
		light->material.color = lightColor;
		light->material.emission = lightColor;
	}
}



CornellBoxScene::CornellBoxScene(float length, float width, float height)
{
	halfLength = length / 2.0f;
	halfWidth = width / 2.0f;
	halfHeight = height / 2.0f;

	/*
		See room definition at
			https://en.wikipedia.org/wiki/Cornell_box
	*/
	leftWall = CreateObject<TriangleMesh>();
	rightWall = CreateObject<TriangleMesh>();
	whiteSegments = CreateObject<TriangleMesh>();

	leftWall->material.color = ColorDbl(0.2, 0.01, 0.01);
	rightWall->material.color = ColorDbl(0.01, 0.2, 0.01);
	whiteSegments->material.color = ColorDbl(0.2);

	// Box corners
	vec3 c1{ -halfWidth, halfHeight,  halfLength };
	vec3 c2{ halfWidth, halfHeight,  halfLength };
	vec3 c3{ halfWidth, halfHeight, -halfLength };
	vec3 c4{ -halfWidth, halfHeight, -halfLength };

	// Floor corners (same, but height is flipped)
	vec3 f1 = c1; f1.y *= -1.0f;
	vec3 f2 = c2; f2.y *= -1.0f;
	vec3 f3 = c3; f3.y *= -1.0f;
	vec3 f4 = c4; f4.y *= -1.0f;

	leftWall->AddQuad(f2, c2, c3, f3);
	rightWall->AddQuad(f1, f4, c4, c1);

	whiteSegments->AddQuad(c4, c3, c2, c1); // Ceiling
	whiteSegments->AddQuad(f4, f3, c3, c4);	// Back wall
	whiteSegments->AddQuad(f1, f2, f3, f4);	// Floor
}

void CornellBoxScene::MoveCameraToRecommendedPosition(Camera& camera)
{
	camera.SetView(vec3(0.0f, 0.0f, halfLength), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

void CornellBoxScene::AddExampleObjects(float radius)
{
	SphereObject* lambertianSphere = CreateObject<SphereObject>();
	SphereObject* specularSphere = CreateObject<SphereObject>();
	SphereObject* orenNayarSphere = CreateObject<SphereObject>();
	SphereObject* refractionSphere = CreateObject<SphereObject>();

	lambertianSphere->radius = radius;
	specularSphere->radius = radius;
	orenNayarSphere->radius = radius;
	refractionSphere->radius = radius;

	float widthOffset = halfWidth - radius;
	float depthOffset = halfLength - radius;
	float heightOffset = halfHeight - radius;
	lambertianSphere->position = vec3(-widthOffset, 0.0f, -depthOffset / 2.0);
	specularSphere->position = vec3(0, 2.0, -halfLength + specularSphere->radius);
	orenNayarSphere->position = vec3(widthOffset, 0.0f, -depthOffset / 2.0);
	refractionSphere->position = vec3(0, -halfHeight + refractionSphere->radius + 1.5, -2);

	lambertianSphere->material.color = ColorDbl(0.5);
	specularSphere->material.color = ColorDbl(0.5);
	orenNayarSphere->material.color = ColorDbl(0.5);
	refractionSphere->material.color = ColorDbl(0.5);

	// Left box
	Box* lambertianBox = CreateObject<Box>();
	lambertianBox->SetGeometry({ halfWidth - 1.5f, -halfHeight, -depthOffset / 2.0 }, { 0.0f, 1.0f, 0.0f }, { -0.5f, 0.0f, 1.0f }, 2.0f, 2.0f, halfHeight - radius);
	lambertianBox->material.color = ColorDbl(0.01, 0.3, 0.8);

	// Right box
	Box* orenNayarBox = CreateObject<Box>();
	orenNayarBox->SetGeometry({ -halfWidth + 1.5f, -halfHeight, -depthOffset / 2.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 2.0f, 2.0f, halfHeight - radius);
	orenNayarBox->material.color = ColorDbl(0.8, 0.4, 0.01);

	// Center box
	Box* middleBox = CreateObject<Box>();
	middleBox->SetGeometry({ 0, -halfHeight, -3 }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, 4.0f, 4.0f, halfHeight - refractionSphere->radius - 2.3f);
	middleBox->material.color = ColorDbl(0.5, 0.2, 0.8);

	// Set surface types
	lambertianSphere->material.type = SurfaceType::Diffuse;
	orenNayarSphere->material.type = SurfaceType::Diffuse;
	lambertianBox->material.type = SurfaceType::Diffuse;
	orenNayarBox->material.type = SurfaceType::Diffuse;
	middleBox->material.type = SurfaceType::Diffuse;

	middleBox->material.diffuse = DiffuseType::Lambertian;
	lambertianBox->material.diffuse = DiffuseType::Lambertian;
	lambertianSphere->material.diffuse = DiffuseType::Lambertian;
	orenNayarBox->material.diffuse = DiffuseType::OrenNayar;
	orenNayarSphere->material.diffuse = DiffuseType::OrenNayar;

	specularSphere->material.type = SurfaceType::Specular;
	refractionSphere->material.type = SurfaceType::Refractive;

	orenNayarSphere->material.roughness = 0.5f;
	orenNayarBox->material.roughness = 0.5f;

	// Test load mesh
	//TriangleMesh* mesh = CreateObject<TriangleMesh>();
	//mesh->position = vec3(0, 0, 0.0);
	//mesh->material.type = SurfaceType::Diffuse;
	//mesh->LoadMesh("box_stack.obj");
}

void CornellBoxScene::AddExampleLight(ColorDbl lightColor, bool usePoint)
{
	vec3 roofCenter{ 0.0f, halfHeight - 0.001f, 0.0f };

	if (usePoint)
	{
		SphereObject* pointLight = CreateObject<SphereObject>();
		pointLight->radius = 0.0f;
		pointLight->material.color = lightColor;
		pointLight->material.emission = lightColor;
		pointLight->position = roofCenter;
	}
	else
	{
		Light* light = CreateObject<Light>();
		light->SetGeometry(roofCenter,									// position
			{ 0.0f, -1.0f, 0.0f },						// direction
			{ 1.0f, 0.0f, 0.0f },						// side
			{ halfWidth / 3.0f, halfHeight / 3.0f });	// dimensions
		light->material.color = lightColor;
		light->material.emission = lightColor;
	}
}
