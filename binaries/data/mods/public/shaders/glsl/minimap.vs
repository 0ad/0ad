#version 110

#if MINIMAP_BASE || MINIMAP_LOS
  attribute vec3 a_vertex;
  attribute vec2 a_uv0;
  varying vec2 v_tex;
#endif

#if MINIMAP_POINT
  attribute vec2 a_vertex;
  attribute vec3 a_color;
  varying vec3 color;
#endif

#if MINIMAP_LINE
  attribute vec2 a_vertex;
#endif

void main()
{
  #if MINIMAP_BASE || MINIMAP_LOS
    gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
    vec4 temp = gl_TextureMatrix[0] * vec4(a_uv0, 0.0, 0.0);
    v_tex = temp.xy;
  #endif

  #if MINIMAP_POINT
    gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 0.0, 1.0);
    color = a_color;
  #endif

  #if MINIMAP_LINE
    gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 0.0, 1.0);
  #endif
}
