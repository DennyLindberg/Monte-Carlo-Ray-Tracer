#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// M_PI definition borrowed from SDL
#ifndef M_PI
#define M_PI    3.14159265358979323846264338327950288
#endif

#define FLOAT_INFINITY std::numeric_limits<float>::max()

/*
	Basic types
*/
typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::dvec4 ColorDbl;
typedef std::int32_t int32;
