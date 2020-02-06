#version 110

varying vec2 v_tex;

attribute vec3 a_vertex;
attribute vec2 a_uv0;

void main()
{  
  gl_Position = vec4(a_vertex, 1.0);  

  v_tex = a_uv0;
}
