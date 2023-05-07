#ifndef INCLUDED_COMMON_FOG
#define INCLUDED_COMMON_FOG

vec3 applyFog(vec3 color, vec3 fogColor, vec2 fogParams)
{
#if USE_FOG
	float density = fogParams.x;
	float maxFog = fogParams.y;

	const float LOG2 = 1.442695;
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = exp2(-density * density * z * z * LOG2);

	fogFactor = fogFactor * (1.0 - maxFog) + maxFog;

	fogFactor = clamp(fogFactor, 0.0, 1.0);

	return mix(fogColor, color, fogFactor);
#else
	return color;
#endif
}

#endif // INCLUDED_COMMON_FOG
