#version 110

uniform mat4 reflectionMatrix;
uniform mat4 refractionMatrix;
uniform mat4 losMatrix;
uniform mat4 shadowTransform;
uniform float repeatScale;
uniform float windAngle;
uniform float waviness;			// "Wildness" of the reflections and refractions; choose based on texture
uniform vec3 sunDir;

#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
	uniform vec4 shadowScale;
#endif

uniform float time;
uniform float mapSize;

uniform mat4 transform;

varying vec3 worldPos;
varying float waterDepth;
varying vec2 waterInfo;

varying float fwaviness;
varying vec2 WindCosSin;

#if USE_SHADOW && USE_SHADOWS_ON_WATER
	varying vec4 v_shadow;
#endif

attribute vec3 a_vertex;
attribute vec2 a_waterInfo;
attribute vec3 a_otherPosition;

void main()
{
	worldPos = vec3(a_vertex.x,15.0,a_vertex.z);
	waterInfo = a_waterInfo;
	waterDepth = a_waterInfo.g;
	
	WindCosSin = vec2(cos(-windAngle),sin(-windAngle));
	
	float newX = a_vertex.x * WindCosSin.x - a_vertex.z * WindCosSin.y;
	float newY = a_vertex.x * WindCosSin.y + a_vertex.z * WindCosSin.x;
	
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
	
	// Fix the waviness for local wind strength
	fwaviness = waviness * ((0.15+a_waterInfo.r/1.15));
	
	gl_Position = transform * vec4(a_vertex, 1.0);
}
