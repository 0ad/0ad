#version 110

uniform vec4 transform;
uniform vec4 translation;
uniform vec4 textureTransform;
#if MINIMAP_POINT && USE_GPU_INSTANCING
uniform float width;
#endif

attribute vec2 a_vertex;

#if MINIMAP_BASE || MINIMAP_LOS
attribute vec2 a_uv0;
#endif

#if MINIMAP_POINT
attribute vec3 a_color;
#if USE_GPU_INSTANCING
attribute vec2 a_uv1;
#endif
#endif

#if MINIMAP_BASE || MINIMAP_LOS
varying vec2 v_tex;
#endif

#if MINIMAP_POINT
varying vec3 color;
#endif

void main()
{
#if MINIMAP_BASE || MINIMAP_LOS
	vec2 position = a_vertex;
	v_tex = mat2(textureTransform.xy, textureTransform.zw) * a_uv0 + translation.zw;
#endif

#if MINIMAP_POINT
#if USE_GPU_INSTANCING
	vec2 position = a_vertex * width + a_uv1;
#else
	vec2 position = a_vertex;
#endif
	color = a_color;
#endif // MINIMAP_POINT

	gl_Position = vec4(mat2(transform.xy, transform.zw) * position + translation.xy, 0.0, 1.0);
}
