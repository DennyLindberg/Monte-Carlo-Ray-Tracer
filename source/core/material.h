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
	float albedo = 1.0;
	float roughness = 1.0;

	double BRDF(vec3& incident, vec3& reflection, vec3& normal)
	{
		switch (diffuse)
		{
		case DiffuseType::OrenNayar: 
		{
			float sigma2 = roughness * roughness;
			float A = 1 - 0.5 * sigma2 / (sigma2 + 0.57);
			float B = 0.45 * sigma2 / (sigma2 + 0.09);

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
