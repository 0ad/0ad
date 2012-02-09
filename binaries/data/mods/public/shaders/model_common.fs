#version 110

uniform sampler2D baseTex;
uniform sampler2DShadow shadowTex;
uniform sampler2D losTex;
uniform vec3 objectColor;
uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec4 shadowOffsets1;
uniform vec4 shadowOffsets2;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;

void main()
{
  vec4 tex = texture2D(baseTex, v_tex);

  #ifdef USE_TRANSPARENT
    gl_FragColor.a = tex.a;
  #else
    gl_FragColor.a = 1.0;
  #endif

  vec3 color = tex.rgb;

  // Apply player-coloring based on texture alpha
  #ifdef USE_OBJECTCOLOR
    color *= mix(objectColor, vec3(1.0, 1.0, 1.0), tex.a);
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

  color *= v_lighting * shadow + ambient;
 
  color *= texture2D(losTex, v_los).a;

  color *= shadingColor;
    
  gl_FragColor.rgb = color;
}
