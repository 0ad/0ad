#ifndef INCLUDED_SHADOWS_VERTEX
#define INCLUDED_SHADOWS_VERTEX

#include "common/shadows.h"

vec4 calculatePositionInShadowSpaceImpl(
	vec4 position, mat4 shadowTransform, vec4 shadowScale)
{
	vec4 shadow = shadowTransform * position;
	#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
		shadow.xy *= shadowScale.xy;
	#endif
	return shadow;
}

void calculatePositionInShadowSpace(vec4 position)
{
#if USE_SHADOW
	v_depth = dot(cameraForward.xyz, position.xyz) - cameraForward.w;
#if SHADOWS_CASCADE_COUNT == 1
	v_shadow = calculatePositionInShadowSpaceImpl(position, shadowTransform, shadowScale);
#else
	for (int cascade = 0; cascade < SHADOWS_CASCADE_COUNT; ++cascade)
	{
		v_shadow[cascade] = calculatePositionInShadowSpaceImpl(
			position, shadowTransforms[cascade], shadowScale);
	}
#endif
#endif // USE_SHADOW
}

#endif // INCLUDED_SHADOWS_VERTEX
