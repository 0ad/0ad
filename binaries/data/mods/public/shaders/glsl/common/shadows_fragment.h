#ifndef INCLUDED_SHADOWS_FRAGMENT
#define INCLUDED_SHADOWS_FRAGMENT

#if USE_SHADOW
  varying float v_depth;
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
    #if USE_SHADOW_PCF
      uniform vec4 shadowScale;
    #endif
  #else
    uniform sampler2D shadowTex;
  #endif
  #if SHADOWS_CASCADE_COUNT == 1
    uniform float shadowDistance;
    varying vec4 v_shadow;
  #else
    uniform float shadowDistances[SHADOWS_CASCADE_COUNT];
    varying vec4 v_shadow[SHADOWS_CASCADE_COUNT];
  #endif
#endif

float getShadowImpl(vec4 shadowVertex, float shadowBias)
{
#if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
  float biasedShdwZ = shadowVertex.z - shadowBias;
  #if USE_SHADOW_SAMPLER
    #if USE_SHADOW_PCF
      vec2 offset = fract(shadowVertex.xy - 0.5);
      vec4 size = vec4(offset + 1.0, 2.0 - offset);
      vec4 weight = (vec4(1.0, 1.0, -0.5, -0.5) + (shadowVertex.xy - 0.5*offset).xyxy) * shadowScale.zwzw;
      return (1.0/9.0)*dot(size.zxzx*size.wwyy,
        vec4(shadow2D(shadowTex, vec3(weight.zw, biasedShdwZ)).r,
             shadow2D(shadowTex, vec3(weight.xw, biasedShdwZ)).r,
             shadow2D(shadowTex, vec3(weight.zy, biasedShdwZ)).r,
             shadow2D(shadowTex, vec3(weight.xy, biasedShdwZ)).r));
    #else
      return shadow2D(shadowTex, vec3(shadowVertex.xy, biasedShdwZ)).r;
    #endif
  #else
    if (biasedShdwZ >= 1.0)
      return 1.0;
    return (biasedShdwZ < texture2D(shadowTex, shadowVertex.xy).x ? 1.0 : 0.0);
  #endif
#else
  return 1.0;
#endif // USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
}

float getShadowWithFade(float shadowBias)
{
#if USE_SHADOW
#if SHADOWS_CASCADE_COUNT == 1
  float blendWidth = 8.0;
  float distanceBlend = clamp((shadowDistance - v_depth - blendWidth) / blendWidth, 0.0, 1.0);
  float shadow = getShadowImpl(v_shadow, shadowBias);
  return mix(1.0, shadow, distanceBlend);
#else
  int firstCascade = SHADOWS_CASCADE_COUNT;
  for (int cascade = 0; cascade < SHADOWS_CASCADE_COUNT; ++cascade)
  {
    if (v_depth <= shadowDistances[cascade])
    {
      firstCascade = cascade;
      break;
    }
  }
  if (firstCascade == SHADOWS_CASCADE_COUNT)
    return 1.0;
  float shadow = getShadowImpl(v_shadow[firstCascade], shadowBias);
  float blendWidth = clamp(shadowDistances[firstCascade] / 8.0, 2.0, 32.0);
  float cascadeBlend = clamp(
    (shadowDistances[firstCascade] - v_depth - blendWidth) / blendWidth, 0.0, 1.0);
  if (firstCascade == SHADOWS_CASCADE_COUNT - 1)
    return mix(1.0, shadow, cascadeBlend);
  else
    return mix(getShadowImpl(v_shadow[firstCascade + 1], shadowBias), shadow, cascadeBlend);
#endif
#else
  return 1.0;
#endif // USE_SHADOW
}

float getShadow()
{
  float shadowBias = 0.003;
  return getShadowWithFade(shadowBias);
}

float getShadowOnLandscape()
{
  float shadowBias = 0.0005;
  return getShadowWithFade(shadowBias);
}

#endif // INCLUDED_SHADOWS_FRAGMENT
