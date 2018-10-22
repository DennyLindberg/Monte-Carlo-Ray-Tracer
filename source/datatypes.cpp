/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "datatypes.h"

#include <iostream>
#include <memory>

void PixelBuffer::SetPixel(unsigned int x, unsigned int y, double r, double g, double b, double a)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	data[pixelIndex] = r;
	data[pixelIndex + 1] = g;
	data[pixelIndex + 2] = b;
	data[pixelIndex + 3] = a;
}

void PixelBuffer::SetPixel(unsigned int x, unsigned int y, ColorDbl color)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	data[pixelIndex] = color.r;
	data[pixelIndex + 1] = color.g;
	data[pixelIndex + 2] = color.b;
	data[pixelIndex + 3] = color.a;
}

unsigned int PixelBuffer::PixelArrayIndex(unsigned int x, unsigned int y)
{
	return y * imageWidth * imageChannelCount + x * imageChannelCount;
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
