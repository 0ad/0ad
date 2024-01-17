#ifndef INCLUDED_COMMON_DESCRIPTOR_INDEXING
#define INCLUDED_COMMON_DESCRIPTOR_INDEXING

#if USE_SPIRV && USE_DESCRIPTOR_INDEXING
#extension GL_EXT_nonuniform_qualifier : enable
const int DESCRIPTOR_INDEXING_SET_SIZE = 16384;
layout (set = 0, binding = 0) uniform sampler2D textures2D[DESCRIPTOR_INDEXING_SET_SIZE];
layout (set = 0, binding = 1) uniform samplerCube texturesCube[DESCRIPTOR_INDEXING_SET_SIZE];
layout (set = 0, binding = 2) uniform sampler2DShadow texturesShadow[DESCRIPTOR_INDEXING_SET_SIZE];
#endif // USE_SPIRV && USE_DESCRIPTOR_INDEXING

#endif // INCLUDED_COMMON_DESCRIPTOR_INDEXING
