#ifndef INCLUDED_COMMON_FRAGMENT
#define INCLUDED_COMMON_FRAGMENT

#include "common/texture.h"
#include "common/uniform.h"

#if USE_SPIRV

#if USE_DESCRIPTOR_INDEXING
#extension GL_EXT_nonuniform_qualifier : enable
const int DESCRIPTOR_INDEXING_SET_SIZE = 16384;
layout (set = 0, binding = 0) uniform sampler2D textures2D[DESCRIPTOR_INDEXING_SET_SIZE];
layout (set = 0, binding = 1) uniform samplerCube texturesCube[DESCRIPTOR_INDEXING_SET_SIZE];
layout (set = 0, binding = 2) uniform sampler2DShadow texturesShadow[DESCRIPTOR_INDEXING_SET_SIZE];
#endif // USE_DESCRIPTOR_INDEXING

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
