#version 120

uniform sampler2D baseTex;
uniform sampler2D blendTex;
uniform sampler2D losTex;

#if USE_SHADOW
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec4 shadowOffsets1;
uniform vec4 shadowOffsets2;

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
        return 0.25 * (
          shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets1.xy, v_shadow.z)).a +
          shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets1.zw, v_shadow.z)).a +
          shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets2.xy, v_shadow.z)).a +
          shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets2.zw, v_shadow.z)).a
        );
      #else
        return shadow2D(shadowTex, v_shadow.xyz).a;
      #endif
    #else
      if (v_shadow.z >= 1.0)
        return 1.0;
      #if USE_SHADOW_PCF
        return (
          (v_shadow.z <= texture2D(shadowTex, v_shadow.xy + shadowOffsets1.xy).x ? 0.25 : 0.0) +
          (v_shadow.z <= texture2D(shadowTex, v_shadow.xy + shadowOffsets1.zw).x ? 0.25 : 0.0) +
          (v_shadow.z <= texture2D(shadowTex, v_shadow.xy + shadowOffsets2.xy).x ? 0.25 : 0.0) +
          (v_shadow.z <= texture2D(shadowTex, v_shadow.xy + shadowOffsets2.zw).x ? 0.25 : 0.0)
        );
      #else
        return (v_shadow.z <= texture2D(shadowTex, v_shadow.xy).x ? 1.0 : 0.0);
      #endif
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
