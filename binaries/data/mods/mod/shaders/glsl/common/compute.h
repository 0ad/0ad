#ifndef INCLUDED_COMMON_COMPUTE
#define INCLUDED_COMMON_COMPUTE

#include "common/descriptor_indexing.h"
#include "common/texture.h"
#include "common/uniform.h"

#if STAGE_COMPUTE

#if USE_SPIRV
#define STORAGE_2D(LOCATION, FORMAT, NAME) \
	layout(set = 2, binding = LOCATION, FORMAT) uniform image2D NAME
#else
// We use offset to the binding slot for OpenGL to avoid overlapping with other
// textures as OpenGL doesn't have sets.
#define STORAGE_2D(LOCATION, FORMAT, NAME) \
	layout(binding = LOCATION, FORMAT) uniform image2D NAME
#endif

#endif // STAGE_COMPUTE

#endif // INCLUDED_COMMON_COMPUTE
