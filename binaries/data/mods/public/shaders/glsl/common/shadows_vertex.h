#ifndef INCLUDED_SHADOWS_VERTEX
#define INCLUDED_SHADOWS_VERTEX

#if USE_SHADOW
	uniform vec4 cameraForward;
	varying float v_depth;
	#if SHADOWS_CASCADE_COUNT == 1
		uniform mat4 shadowTransform;
		varying vec4 v_shadow;
	#else
		uniform mat4 shadowTransforms[SHADOWS_CASCADE_COUNT];
		varying vec4 v_shadow[SHADOWS_CASCADE_COUNT];
	#endif
	#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
		uniform vec4 shadowScale;
	#endif
#endif // USE_SHADOW

void calculatePositionInShadowSpace(vec4 position)
{
#if USE_SHADOW
	v_depth = dot(cameraForward.xyz, position.xyz) - cameraForward.w;
#if SHADOWS_CASCADE_COUNT == 1
	v_shadow = shadowTransform * position;
	#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
		v_shadow.xy *= shadowScale.xy;
	#endif
#else
	for (int cascade = 0; cascade < SHADOWS_CASCADE_COUNT; ++cascade)
	{
		v_shadow[cascade] = shadowTransforms[cascade] * position;
	#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
		v_shadow[cascade].xy *= shadowScale.xy;
	#endif
	}
#else

#endif
#endif // USE_SHADOW
}

#endif // INCLUDED_SHADOWS_VERTEX
