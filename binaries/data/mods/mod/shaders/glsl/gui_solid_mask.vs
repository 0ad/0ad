#version 110

uniform mat4 transform;

varying vec2 v_tex;

attribute vec3 a_vertex;
attribute vec2 a_uv0;

void main()
{
  gl_Position = transform * vec4(a_vertex, 1.0);

  v_tex = a_uv0;
}
