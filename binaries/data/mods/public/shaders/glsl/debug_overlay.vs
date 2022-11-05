#version 110

#include "common/vertex.h"

uniform mat4 transform;
#if DEBUG_TEXTURED
uniform mat4 textureTransform;
#endif

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);

#if DEBUG_TEXTURED
VERTEX_INPUT_ATTRIBUTE(1, vec3, a_uv0);

varying vec2 v_tex;
#endif

void main()
{
#if DEBUG_TEXTURED
	v_tex = (textureTransform * vec4(a_uv0, 1.0)).xy;
#endif
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
