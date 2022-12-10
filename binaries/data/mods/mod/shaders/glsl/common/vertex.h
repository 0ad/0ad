#ifndef INCLUDED_COMMON_VERTEX
#define INCLUDED_COMMON_VERTEX

#include "common/uniform.h"

#define VERTEX_INPUT_ATTRIBUTE(LOCATION, TYPE, NAME) \
	attribute TYPE NAME

#define OUTPUT_VERTEX_POSITION(position) \
	gl_Position = position

#endif // INCLUDED_COMMON_VERTEX
