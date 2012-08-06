#version 120

uniform sampler2D baseTex;
uniform sampler2D blendTex;
uniform sampler2D losTex;

#if USE_SHADOW
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
    #if USE_SHADOW_PCF
      uniform vec4 shadowScale;
    #endif
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

uniform vec3 shadingColor;
uniform vec3 ambient;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
varying vec2 v_blend;

#if USE_SPECULAR
  uniform float specularPower;
  uniform vec3 specularColor;
  varying vec3 v_normal;
  varying vec3 v_half;
#endif

float get_shadow()
{
  #if USE_SHADOW
    #if USE_SHADOW_SAMPLER
      #if USE_SHADOW_PCF
        vec2 offset = fract(v_shadow.xy - 0.5);
        vec4 size = vec4(offset + 1.0, 2.0 - offset);
        vec4 weight = (vec4(2.0 - 1.0 / size.xy, 1.0 / size.zw - 1.0) + (v_shadow.xy - offset).xyxy) * shadowScale.zwzw;
        return (1.0/9.0)*dot(size.zxzx*size.wwyy,
          vec4(shadow2D(shadowTex, vec3(weight.zw, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.xw, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.zy, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.xy, v_shadow.z)).r));
      #else
        return shadow2D(shadowTex, v_shadow.xyz).r;
      #endif
    #else
      if (v_shadow.z >= 1.0)
        return 1.0;
      return (v_shadow.z <= texture2D(shadowTex, v_shadow.xy).x ? 1.0 : 0.0);
    #endif
  #else
    return 1.0;
  #endif
}

void main()
{
  #if BLEND
    // Use alpha from blend texture
    gl_FragColor.a = 1.0 - texture2D(blendTex, v_blend).a;
  #endif

  vec4 tex = texture2D(baseTex, v_tex);

  #if DECAL
    // Use alpha from main texture
    gl_FragColor.a = tex.a;
  #endif

  vec3 texdiffuse = tex.rgb;
  vec3 sundiffuse = v_lighting;

  #if USE_SPECULAR
    // Interpolated v_normal needs to be re-normalized since it varies
    // significantly between adjacenent vertexes;
    // v_half changes very gradually so don't bother normalizing that
    vec3 specular = specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);
  #else
    vec3 specular = vec3(0.0);
  #endif

  vec3 color = (texdiffuse * sundiffuse + specular) * get_shadow() + texdiffuse * ambient;

  float los = texture2D(losTex, v_los).a;
  color *= los;

  #if DECAL
    color *= shadingColor;
  #endif

  gl_FragColor.rgb = color;
}
