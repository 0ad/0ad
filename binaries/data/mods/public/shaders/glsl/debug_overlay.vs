#version 110

#include "common/vertex.h"

uniform mat4 transform;
#if DEBUG_TEXTURED
uniform vec2 textureTransform;
#endif

#if DEBUG_TEXTURED
varying vec2 v_tex;
#endif

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
#if DEBUG_TEXTURED
VERTEX_INPUT_ATTRIBUTE(1, vec3, a_uv0);
#endif

void main()
{
#if DEBUG_TEXTURED
	v_tex = textureTransform.xy * a_uv0.xz;
#endif
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
