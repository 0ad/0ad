#version 110

#if MINIMAP_BASE || MINIMAP_LOS
  uniform sampler2D baseTex;
  varying vec2 v_tex;
#endif

#if MINIMAP_POINT
  varying vec3 color;
#endif

#if MINIMAP_LINE
  uniform vec4 color;
#endif

void main()
{
  #if MINIMAP_BASE
    gl_FragColor = texture2D(baseTex, v_tex);
  #endif

  #if MINIMAP_LOS
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0 - texture2D(baseTex, v_tex).a);
  #endif

  #if MINIMAP_POINT
    gl_FragColor = vec4(color, 1.0);
  #endif

  #if MINIMAP_LINE
    gl_FragColor = color;
  #endif
}
