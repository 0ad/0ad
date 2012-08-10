#version 110

uniform mat4 transform;

varying vec2 v_tex;
varying vec4 v_color;

attribute vec4 a_vertex;
attribute vec4 a_color;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

void main()
{
  vec3 axis1 = transform[0].xyz;
  vec3 axis2 = transform[1].xyz;

  vec2 offset = a_uv1;

  vec4 position;
  position.xyz = axis1*offset.x + axis1*offset.y + axis2*offset.x + axis2*-offset.y + a_vertex.xyz;
  position.w = a_vertex.w;

  gl_Position = transform * position;
  
  v_tex = a_uv0;
  v_color = a_color;
}