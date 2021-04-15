#version 110

uniform mat4 transform;
#if DEBUG_TEXTURED
uniform mat4 textureTransform;
#endif

attribute vec3 a_vertex;

#if DEBUG_TEXTURED
attribute vec3 a_uv0;

varying vec2 v_tex;
#endif

void main()
{
#if DEBUG_TEXTURED
	v_tex = (textureTransform * vec4(a_uv0, 1.0)).xy;
#endif
	gl_Position = transform * vec4(a_vertex, 1.0);
}
