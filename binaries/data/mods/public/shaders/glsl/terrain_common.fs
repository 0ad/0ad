#version 110

uniform sampler2D baseTex;
uniform sampler2D blendTex;
#ifdef USE_SHADOW
uniform sampler2DShadow shadowTex;
#endif
uniform sampler2D losTex;
uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec4 shadowOffsets1;
uniform vec4 shadowOffsets2;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
varying vec2 v_blend;

void main()
{

  #ifdef BLEND
    // Use alpha from blend texture
    gl_FragColor.a = 1.0 - texture2D(blendTex, v_blend).a;
  #endif

  // Load diffuse colour
  vec4 color = texture2D(baseTex, v_tex);

  #ifdef DECAL
    // Use alpha from main texture
    gl_FragColor.a = color.a;
  #endif

  #ifdef USE_SHADOW
    #ifdef USE_SHADOW_PCF
      float shadow = (
        shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets1.xy, v_shadow.z)).a +
        shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets1.zw, v_shadow.z)).a +
        shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets2.xy, v_shadow.z)).a +
        shadow2D(shadowTex, vec3(v_shadow.xy + shadowOffsets2.zw, v_shadow.z)).a
      ) * 0.25;
    #else
      float shadow = shadow2D(shadowTex, v_shadow.xyz).a;
    #endif
  #else
    float shadow = 1.0;
  #endif

  color.rgb *= v_lighting * shadow + ambient;
 
  color *= texture2D(losTex, v_los).a;

  #ifdef DECAL
    color.rgb *= shadingColor;
  #endif

  gl_FragColor.rgb = color.rgb;
}
