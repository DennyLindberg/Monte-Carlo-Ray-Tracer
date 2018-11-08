/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include "pixelbuffer.h"
#include "../core/ray.h"

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
		// When we create a ray through a pixel, we get a pinhole camera.
		// This gives us a projection by default.
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