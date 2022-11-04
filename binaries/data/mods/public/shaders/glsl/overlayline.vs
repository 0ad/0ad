#version 120

#include "common/los_vertex.h"
#include "common/vertex.h"

uniform mat4 transform;

attribute vec3 a_vertex;
attribute vec2 a_uv0;

#if !USE_OBJECTCOLOR
attribute vec4 a_color;
varying vec4 v_color;
#endif

varying vec2 v_tex;

void main()
{
	v_tex = a_uv0;
	calculateLOSCoordinates(a_vertex.xz);
#if !USE_OBJECTCOLOR
	v_color = a_color;
#endif
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
