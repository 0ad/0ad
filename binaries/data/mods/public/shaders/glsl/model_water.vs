#if USE_GPU_SKINNING
// Skinning requires GLSL 1.30 for ivec4 vertex attributes
#version 130
#else
#version 120
#endif

uniform mat4 transform;
uniform vec3 cameraPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec2 losTransform;
uniform mat4 shadowTransform;
uniform mat4 instancingTransform;

uniform float sim_time;
uniform vec2 translation;

#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
  uniform vec4 shadowScale;
#endif


attribute vec3 a_vertex;
attribute vec3 a_normal;
#if USE_INSTANCING
  attribute vec4 a_tangent;
#endif
attribute vec2 a_uv0;
attribute vec2 a_uv1;

#if USE_GPU_SKINNING
  const int MAX_INFLUENCES = 4;
  const int MAX_BONES = 64;
  uniform mat4 skinBlendMatrices[MAX_BONES];
  attribute ivec4 a_skinJoints;
  attribute vec4 a_skinWeights;
#endif


varying vec4 worldPos;
varying vec4 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;


vec4 fakeCos(vec4 x)
{
	vec4 tri = abs(fract(x + 0.5) * 2.0 - 1.0);
	return tri * tri *(3.0 - 2.0 * tri);  
}



void main()
{
	worldPos = instancingTransform * vec4(a_vertex, 1.0);
	
	v_tex.xy = (a_uv0 + worldPos.xz) / 5.0 + sim_time * translation;

	v_tex.zw = a_uv0;

	#if USE_SHADOW
		v_shadow = shadowTransform * worldPos;
		#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
			v_shadow.xy *= shadowScale.xy;
		#endif  
	#endif

	v_los = worldPos.xz * losTransform.x + losTransform.y;

	gl_Position = transform * worldPos;
}

