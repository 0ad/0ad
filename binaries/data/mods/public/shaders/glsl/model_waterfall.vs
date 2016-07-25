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
attribute vec2 a_uv0;
attribute vec2 a_uv1;

varying vec4 worldPos;
varying vec4 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
varying vec3 v_half;
varying vec3 v_normal;
varying float v_transp;
varying vec3 v_lighting;


void main()
{
	worldPos = instancingTransform * vec4(a_vertex, 1.0);
	
	v_tex.xy = a_uv0 + sim_time * translation;
	v_transp = a_uv1.x;

	#if USE_SHADOW
		v_shadow = shadowTransform * worldPos;
		#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
			v_shadow.xy *= shadowScale.xy;
		#endif  
	#endif

	v_los = worldPos.xz * losTransform.x + losTransform.y;

	vec3 eyeVec = cameraPos.xyz - worldPos.xyz;
        vec3 sunVec = -sunDir;
        v_half = normalize(sunVec + normalize(eyeVec));

	mat3 normalMatrix = mat3(instancingTransform[0].xyz, instancingTransform[1].xyz, instancingTransform[2].xyz);
	v_normal = normalMatrix * a_normal;
	v_lighting = max(0.0, dot(v_normal, -sunDir)) * sunColor;

	gl_Position = transform * worldPos;
}

