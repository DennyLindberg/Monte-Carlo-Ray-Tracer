#pragma once
#include "../core/math.h"

enum class SurfaceType { Diffuse, Specular, Diffuse_Specular, Refractive, COUNT };
enum class DiffuseType { Lambertian, OrenNayar, COUNT };

struct Material
{
	ColorDbl color = ColorDbl{ 1.0f, 1.0f, 1.0f };
	ColorDbl emission = ColorDbl{ 0.0f, 0.0f, 0.0f };
	SurfaceType type = SurfaceType::Diffuse;
	DiffuseType diffuse = DiffuseType::Lambertian;
	float albedo = 1.0f;
	float roughness = 1.0f;
	float refractiveIndex = 1.52f; // window glass

	double BRDF(vec3& incident, vec3& reflection, vec3& normal)
	{
		switch (diffuse)
		{
		case DiffuseType::OrenNayar: 
		{
			float sigma2 = roughness * roughness;
			float A = 1.0f - 0.5f * sigma2 / (sigma2 + 0.57f);
			float B = 0.45f * sigma2 / (sigma2 + 0.09f);

			float cos_in = glm::dot(incident, normal);
			float cos_out = glm::dot(reflection, normal);
			float cos_in_out = glm::dot(incident, reflection);

			float theta_in = glm::acos(cos_in);
			float theta_out = glm::acos(cos_out);

			float alpha = glm::max(theta_out, theta_in);
			float beta = glm::min(theta_out, theta_in);

			double ON = (A + (B * glm::max(0.0f, cos_in_out)) * glm::sin(alpha) * glm::tan(beta));
			return albedo / M_PI * ON;
		}
		case DiffuseType::Lambertian: 
		default:
			return albedo / M_PI;
		}

		return 1.0;
	}
};
