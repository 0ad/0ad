#ifndef INCLUDED_SHADOWS_VERTEX
#define INCLUDED_SHADOWS_VERTEX

#if USE_SHADOW
uniform mat4 shadowTransform;
varying vec4 v_shadow;
#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
	uniform vec4 shadowScale;
#endif
#endif

void calculatePositionInShadowSpace(vec4 position)
{
#if USE_SHADOW
	v_shadow = shadowTransform * position;
#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
	v_shadow.xy *= shadowScale.xy;
#endif
#endif
}

#endif // INCLUDED_SHADOWS_VERTEX
