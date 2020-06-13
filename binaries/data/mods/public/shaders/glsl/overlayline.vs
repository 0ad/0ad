#version 120

uniform vec2 losTransform;

attribute vec3 a_vertex;
attribute vec2 a_uv0;

#if !USE_OBJECTCOLOR
attribute vec4 a_color;
varying vec4 v_color;
#endif

varying vec2 v_tex;
varying vec2 v_los;

void main()
{
	v_tex = a_uv0;
	v_los = a_vertex.xz * losTransform.x + losTransform.yy;
#if !USE_OBJECTCOLOR
	v_color = a_color;
#endif
	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
}
