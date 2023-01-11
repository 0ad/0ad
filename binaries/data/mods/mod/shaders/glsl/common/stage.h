#ifndef INCLUDED_COMMON_STAGE
#define INCLUDED_COMMON_STAGE

#include "common/uniform.h"

#if USE_SPIRV

#if STAGE_VERTEX
#define VERTEX_OUTPUT(LOCATION, TYPE, NAME) \
	layout (location = LOCATION) out TYPE NAME
#elif STAGE_FRAGMENT
#define VERTEX_OUTPUT(LOCATION, TYPE, NAME) \
	layout (location = LOCATION) in TYPE NAME
#endif

#else // USE_SPIRV

#define VERTEX_OUTPUT(LOCATION, TYPE, NAME) \
	varying TYPE NAME

#endif // USE_SPIRV

#endif // INCLUDED_COMMON_STAGE
