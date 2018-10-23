#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// M_PI definition borrowed from SDL
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifndef M_ONE_OVER_PI
#define M_ONE_OVER_PI 0.318309886183790671538
#endif

#ifndef M_ONE_OVER_TWO_PI
#define M_ONE_OVER_TWO_PI 0.318309886183790671538*2.0
#endif

#ifndef M_TWO_PI
#define M_TWO_PI 3.14159265358979323846264338327950288*2.0
#endif

#ifndef M_PI_HALF
#define M_PI_HALF 3.14159265358979323846264338327950288/2.0
#endif

#define FLOAT_INFINITY std::numeric_limits<float>::max()

/*
	Basic types
*/
typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::dvec3 ColorDbl;
typedef std::int32_t int32;