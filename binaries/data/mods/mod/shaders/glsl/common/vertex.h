#ifndef INCLUDED_COMMON_VERTEX
#define INCLUDED_COMMON_VERTEX

#include "common/uniform.h"

#if USE_SPIRV

#define VERTEX_INPUT_ATTRIBUTE(LOCATION, TYPE, NAME) \
	layout (location = LOCATION) in TYPE NAME

#define OUTPUT_VERTEX_POSITION(POSITION) \
	{ \
	vec4 position = (POSITION); \
	position.y = -position.y; \
	position.z = (position.z + position.w) / 2.0; \
	gl_Position = position; \
	}

#else // USE_SPIRV

#define VERTEX_INPUT_ATTRIBUTE(LOCATION, TYPE, NAME) \
	attribute TYPE NAME

#define OUTPUT_VERTEX_POSITION(position) \
	gl_Position = position

#endif // USE_SPIRV

#endif // INCLUDED_COMMON_VERTEX
