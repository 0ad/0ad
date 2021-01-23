#ifndef INCLUDED_SHADOWS_FRAGMENT
#define INCLUDED_SHADOWS_FRAGMENT

#if USE_SHADOW
  varying vec4 v_shadow;
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
    #if USE_SHADOW_PCF
      uniform vec4 shadowScale;
    #endif
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

float getShadowImpl(float shadowBias)
{
  #if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
    float biasedShdwZ = v_shadow.z - shadowBias;
    #if USE_SHADOW_SAMPLER
      #if USE_SHADOW_PCF
        vec2 offset = fract(v_shadow.xy - 0.5);
        vec4 size = vec4(offset + 1.0, 2.0 - offset);
        vec4 weight = (vec4(1.0, 1.0, -0.5, -0.5) + (v_shadow.xy - 0.5*offset).xyxy) * shadowScale.zwzw;
        return (1.0/9.0)*dot(size.zxzx*size.wwyy,
          vec4(shadow2D(shadowTex, vec3(weight.zw, biasedShdwZ)).r,
               shadow2D(shadowTex, vec3(weight.xw, biasedShdwZ)).r,
               shadow2D(shadowTex, vec3(weight.zy, biasedShdwZ)).r,
               shadow2D(shadowTex, vec3(weight.xy, biasedShdwZ)).r));
      #else
        return shadow2D(shadowTex, vec3(v_shadow.xy, biasedShdwZ)).r;
      #endif
    #else
      if (biasedShdwZ >= 1.0)
        return 1.0;
      return (biasedShdwZ < texture2D(shadowTex, v_shadow.xy).x ? 1.0 : 0.0);
    #endif
  #else
    return 1.0;
  #endif
}

float getShadow()
{
  float shadowBias = 0.003;
  return getShadowImpl(shadowBias);
}

float getShadowOnLandscape()
{
  float shadowBias = 0.0005;
  return getShadowImpl(shadowBias);
}

#endif // INCLUDED_SHADOWS_FRAGMENT
