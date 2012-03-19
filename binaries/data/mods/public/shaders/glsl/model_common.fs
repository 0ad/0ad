#version 110

uniform sampler2D baseTex;
uniform sampler2D losTex;

#ifdef USE_SHADOW
  #ifdef USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

uniform vec3 objectColor;
uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec4 shadowOffsets1;
uniform vec4 shadowOffsets2;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;

float get_shadow()
{
  #ifdef USE_SHADOW
    #ifdef USE_SHADOW_SAMPLER
      #ifdef USE_SHADOW_PCF
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
      #ifdef USE_SHADOW_PCF
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

  #ifdef USE_TRANSPARENT
    gl_FragColor.a = tex.a;
  #else
    gl_FragColor.a = 1.0;
  #endif
  
  #ifdef REQUIRE_ALPHA_GREATER
    if (gl_FragColor.a <= REQUIRE_ALPHA_GREATER)
      discard;
  #endif

  vec3 color = tex.rgb;

  // Apply player-coloring based on texture alpha
  #ifdef USE_OBJECTCOLOR
    color *= mix(objectColor, vec3(1.0, 1.0, 1.0), tex.a);
  #endif

  color *= v_lighting * get_shadow() + ambient;
 
  color *= texture2D(losTex, v_los).a;

  color *= shadingColor;
    
  gl_FragColor.rgb = color;
}
