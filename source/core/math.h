/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// M_PI definition borrowed from SDL
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifndef M_ONE_OVER_PI
#define M_ONE_OVER_PI 0.31830988618379067153776752674502872
#endif

#ifndef M_ONE_OVER_TWO_PI
#define M_ONE_OVER_TWO_PI 0.15915494309189533576888376337251436
#endif

#ifndef M_TWO_PI
#define M_TWO_PI 6.28318530717958647692528676655900576
#endif

#ifndef M_PI_HALF
#define M_PI_HALF 1.57079632679489661923132169163975144
#endif

#define FLOAT_INFINITY std::numeric_limits<float>::max()

#define INTERSECTION_ERROR_MARGIN FLT_EPSILON*20.0f

/*
	Basic types
*/
typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::dvec3 ColorDbl;
typedef std::int32_t int32;