#ifndef INCLUDED_COMMON_FRAGMENT
#define INCLUDED_COMMON_FRAGMENT

#include "common/texture.h"
#include "common/uniform.h"

#define OUTPUT_FRAGMENT_SINGLE_COLOR(COLOR) \
	gl_FragColor = COLOR

#define OUTPUT_FRAGMENT_COLOR(LOCATION, COLOR) \
	gl_FragData[LOCATION] = COLOR

#endif // INCLUDED_COMMON_FRAGMENT
