#ifndef INCLUDED_COMMON_FRAGMENT
#define INCLUDED_COMMON_FRAGMENT

#include "common/descriptor_indexing.h"
#include "common/texture.h"
#include "common/uniform.h"

#if USE_SPIRV

layout (location = 0) out vec4 fragmentColor;

#define OUTPUT_FRAGMENT_SINGLE_COLOR(COLOR) \
	fragmentColor = COLOR

#define OUTPUT_FRAGMENT_COLOR(LOCATION, COLOR) \
	gl_FragData[LOCATION] = COLOR

#else // USE_SPIRV

#define OUTPUT_FRAGMENT_SINGLE_COLOR(COLOR) \
	gl_FragColor = COLOR

#define OUTPUT_FRAGMENT_COLOR(LOCATION, COLOR) \
	gl_FragData[LOCATION] = COLOR

#endif // USE_SPIRV

#endif // INCLUDED_COMMON_FRAGMENT
