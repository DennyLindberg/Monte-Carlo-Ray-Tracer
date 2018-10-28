#pragma once
#include "mesh.h"

class Light : public TriangleMesh
{
public:
	vec3 normal;
	vec3 xVector;
	vec3 yVector;

	Light(ColorDbl lightEmission = ColorDbl{ 1.0 })
	{
		emission = lightEmission;
	}

	~Light() = default;

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

		position += lightDirection * INTERSECTION_ERROR_MARGIN;

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