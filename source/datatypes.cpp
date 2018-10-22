/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "datatypes.h"

#include <iostream>
#include <memory>

void PixelBuffer::SetPixel(unsigned int pixelIndex, double r, double g, double b, double a)
{
	data[pixelIndex] = r;
	data[pixelIndex + 1] = g;
	data[pixelIndex + 2] = b;
	data[pixelIndex + 3] = a;
}

void PixelBuffer::SetPixel(unsigned int pixelIndex, ColorDbl color)
{
	data[pixelIndex] = color.r;
	data[pixelIndex + 1] = color.g;
	data[pixelIndex + 2] = color.b;
	data[pixelIndex + 3] = color.a;
}

void PixelBuffer::AddRayColor(unsigned int pixelIndex, ColorDbl color)
{
	data[pixelIndex] += color.r;
	data[pixelIndex + 1] += color.g;
	data[pixelIndex + 2] += color.b;
	data[pixelIndex + 3] += color.a;

	rayCount[pixelIndex]++;
}

uint64_t PixelBuffer::GetRayCount(unsigned int pixelIndex)
{
	return rayCount[pixelIndex];
}

unsigned int PixelBuffer::PixelArrayIndex(unsigned int x, unsigned int y)
{
	return y * imageWidth * imageChannelCount + x * imageChannelCount;
}

ColorDbl PixelBuffer::GetPixelColor(unsigned int x, unsigned int y)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	return ColorDbl(data[pixelIndex], data[pixelIndex+1], data[pixelIndex+2], data[pixelIndex+3]);
}

Ray GetHemisphereRay(vec3 & origin, vec3 & incomingDirection, vec3 & surfaceNormal, float normalInclination, float azimuth)
{
	const vec3& up = surfaceNormal;
	const vec3 side = glm::cross(up, incomingDirection);
	const vec3 forward = glm::cross(side, up);

	vec3 x = side * sin(normalInclination) * sin(azimuth);
	vec3 y = up * cos(normalInclination);
	vec3 z = forward * sin(normalInclination) * cos(azimuth);

	// our
	vec3 newDirection = x + y + z;
	return Ray(origin, glm::normalize(newDirection));
}