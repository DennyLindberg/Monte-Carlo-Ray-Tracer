#pragma once

#include <glm/glm.hpp>

/*
	Basic types
*/
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::dvec4 ColorDbl;
typedef std::int32_t int32;

class direction_vec4 : public glm::vec4
{
public:
	direction_vec4()
	{
		w = 0.0f;
	}
};

class position_vec4 : public glm::vec4
{
public:
	position_vec4()
	{
		w = 1.0f;
	}
};

/*
	
*/


//bool TriangleRayIntersection(triangle, ray, float t&)
//{
//	t = ? ;
//	return true false;
//}