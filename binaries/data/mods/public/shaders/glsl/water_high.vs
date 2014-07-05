#version 110

uniform mat4 reflectionMatrix;
uniform mat4 refractionMatrix;
uniform mat4 losMatrix;
uniform mat4 shadowTransform;
uniform float repeatScale;
uniform float windAngle;
uniform float waviness;			// "Wildness" of the reflections and refractions; choose based on texture

#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
	uniform vec4 shadowScale;
#endif

uniform float time;
uniform float mapSize;

varying vec3 worldPos;
varying float waterDepth;
varying vec4 waterInfo;

#if USE_SHADOW && USE_SHADOWS_ON_WATER
	varying vec4 v_shadow;
#endif
attribute vec3 a_vertex;
attribute vec4 a_waterInfo;
void main()
{
	worldPos = a_vertex;
	waterInfo = a_waterInfo;
	waterDepth = a_waterInfo.a;
	
	float newX = a_vertex.x * cos(-windAngle) - a_vertex.z * sin(-windAngle);
	float newY = a_vertex.x * sin(-windAngle) + a_vertex.z * cos(-windAngle);
	
	gl_TexCoord[0] = vec4(newX,newY,time,0.0);
	gl_TexCoord[0].xy *= repeatScale;
	gl_TexCoord[1] = reflectionMatrix * vec4(a_vertex, 1.0);		// projective texturing
	gl_TexCoord[2] = refractionMatrix * vec4(a_vertex, 1.0);
	gl_TexCoord[3] = losMatrix * vec4(a_vertex, 1.0);

	gl_TexCoord[3].zw = vec2(a_vertex.xz)/mapSize;
	
	#if USE_SHADOW && USE_SHADOWS_ON_WATER
			v_shadow = shadowTransform * vec4(a_vertex, 1.0);
		#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
			v_shadow.xy *= shadowScale.xy;
		#endif
	#endif
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
}
