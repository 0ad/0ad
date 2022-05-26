#version 110

#if MINIMAP_BASE || MINIMAP_LOS
  uniform sampler2D baseTex;
  varying vec2 v_tex;
#endif

#if MINIMAP_MASK
  uniform sampler2D maskTex;
  varying vec2 v_maskUV;
#endif

#if MINIMAP_POINT
  varying vec3 color;
#endif

#if MINIMAP_LINE
  uniform vec4 color;
#endif

void main()
{
  #if MINIMAP_MASK
    float mask = texture2D(maskTex, v_maskUV).a;
  #endif

  #if MINIMAP_BASE
  #if MINIMAP_MASK
    vec4 color = texture2D(baseTex, v_tex);
    gl_FragColor.rgb = color.rgb;
    gl_FragColor.a = color.a * mask;
  #else
    gl_FragColor = texture2D(baseTex, v_tex);
  #endif
  #endif

  #if MINIMAP_LOS
    gl_FragColor = texture2D(baseTex, v_tex).rrrr;
  #endif

  #if MINIMAP_POINT
    gl_FragColor = vec4(color, 1.0);
  #endif

  #if MINIMAP_LINE
    gl_FragColor = color;
  #endif
}
