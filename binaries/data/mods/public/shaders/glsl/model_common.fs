#version 120

uniform sampler2D baseTex;
uniform sampler2D losTex;

#if USE_SHADOW
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

#if USE_OBJECTCOLOR
  uniform vec3 objectColor;
#else
#if USE_PLAYERCOLOR
  uniform vec3 playerColor;
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

#if USE_SPECULAR
  uniform float specularPower;
  uniform vec3 specularColor;
  uniform vec3 sunColor;
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
  vec4 tex = texture2D(baseTex, v_tex);

  // Alpha-test as early as possible
  #ifdef REQUIRE_ALPHA_GEQUAL
    if (tex.a < REQUIRE_ALPHA_GEQUAL)
      discard;
  #endif

  #if USE_TRANSPARENT
    gl_FragColor.a = tex.a;
  #else
    gl_FragColor.a = 1.0;
  #endif
  
  vec3 texdiffuse = tex.rgb;

  // Apply-coloring based on texture alpha
  #if USE_OBJECTCOLOR
    texdiffuse *= mix(objectColor, vec3(1.0, 1.0, 1.0), tex.a);
  #else
  #if USE_PLAYERCOLOR
    texdiffuse *= mix(playerColor, vec3(1.0, 1.0, 1.0), tex.a);
  #endif
  #endif

  vec3 sundiffuse = v_lighting;

  #if USE_SPECULAR
    // Interpolated v_normal needs to be re-normalized since it varies
    // significantly between adjacent vertexes;
    // v_half changes very gradually so don't bother normalizing that
    vec3 specular = sunColor * specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);
  #else
    vec3 specular = vec3(0.0);
  #endif

  vec3 color = (texdiffuse * sundiffuse + specular) * get_shadow() + texdiffuse * ambient;

  #if !IGNORE_LOS
    float los = texture2D(losTex, v_los).a;
    color *= los;
  #endif

  color *= shadingColor;

  gl_FragColor.rgb = color;
}
