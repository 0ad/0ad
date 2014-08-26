#version 110

uniform mat4 transform;

attribute vec3 a_vertex;

void main()
{
  gl_Position = transform * vec4(a_vertex, 1.0);
}
