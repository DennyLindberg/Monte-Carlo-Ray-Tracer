/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "datatypes.h"

#include <iostream>
#include <memory>

void PixelBuffer::SetPixel(unsigned int pixelIndex, double r, double g, double b)
{
	data[pixelIndex] = r;
	data[pixelIndex + 1] = g;
	data[pixelIndex + 2] = b;
}

void PixelBuffer::SetPixel(unsigned int pixelIndex, ColorDbl color)
{
	data[pixelIndex] = color.r;
	data[pixelIndex + 1] = color.g;
	data[pixelIndex + 2] = color.b;
}

void PixelBuffer::AddRayColor(unsigned int pixelIndex, ColorDbl color)
{
	data[pixelIndex] += color.r;
	data[pixelIndex + 1] += color.g;
	data[pixelIndex + 2] += color.b;

	rayCount[pixelIndex]++;
}

uint64_t PixelBuffer::GetRayCount(unsigned int pixelIndex)
{
	return rayCount[pixelIndex];
}

unsigned int PixelBuffer::PixelArrayIndex(unsigned int x, unsigned int y)
{
	return y * imageWidth * 3 + x * 3;
}


ColorDbl PixelBuffer::GetPixelColor(unsigned int x, unsigned int y)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	return ColorDbl(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2]);
}

Ray RandomHemisphereRay(vec3& origin, vec3& incomingDirection, vec3& surfaceNormal, UniformRandomGenerator& gen)
{
	const bool useOurMethod = false;
	if constexpr (useOurMethod)
	{
		float normalInclination = gen.RandomFloat(0, float(M_PI_HALF));
		float azimuth = gen.RandomFloat(0, float(M_TWO_PI));

		const vec3& up = surfaceNormal;
		const vec3 side = glm::cross(up, incomingDirection);
		const vec3 forward = glm::cross(side, up);

		vec3 x = side * sin(normalInclination) * sin(azimuth);
		vec3 y = up * cos(normalInclination);
		vec3 z = forward * sin(normalInclination) * cos(azimuth);

		vec3 newDirection = x + y + z;
		return Ray(origin, glm::normalize(newDirection));
	}
	else
	{
		vec3 Nx, Nz, Ny = surfaceNormal;
		if (fabs(Ny.x) > fabs(Ny.y)) Nx = vec3(Ny.z, 0, -Ny.x);
		else Nx = vec3(0, -Ny.z, Ny.y);
		Nx = glm::normalize(Nx);
		Nz = glm::normalize(glm::cross(Ny, Nx));

		float cosTheta = gen.RandomFloat(0.0f, 1.0f);
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
}



// Lambertian or OrenNayar?

/* perfectReflectens(Reflexiveness albedo) return albedo;
 
 lambertian(glm::vec3 Reflect, glm::vec3 Incidient, glm::vec3 normal, Reflectivness albedo) return albeodo/M_PI;
 
 orenNayar(glm::vec3 Reflect, glm::vec3 Incidient, glm::vec3 normal, Reflectivness albedo, float rough){
 float sigma2 = rough * rough;
 float A = 1 - 0.5 * sigma2 / (sigma2 + 0.57);
 float B = 0.45 * sigma2 / (sigma2 + 0.09);
 float angleReflect = glm::dot(Reflect, normal);
 float angleIncidient = glm::dot(Incidient, normal);
 float thetaReflect = glm::acos(cos_theta_d2);
 float thetaIncidient = glm::acos(thetaIncidient);
 float alpha = glm::max(thetaReflect, thetaIncidient);
 float beta = glm::min(thetaReflect, thetaIncidient);
 float cosReIn = glm::dot(Reflect, Incidient);
 
 
 } return albedo/M_PI* (A + (B * glm::max(0.0f, cosReIn)) * glm::sin(alpha) * glm::tan(beta));
 */
