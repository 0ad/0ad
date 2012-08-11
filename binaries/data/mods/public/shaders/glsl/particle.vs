#version 110

uniform mat4 transform;

varying vec2 v_tex;
varying vec4 v_color;

attribute vec3 a_vertex;
attribute vec4 a_color;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

void main()
{
  vec3 axis1 = vec3(gl_ModelViewMatrix[0][0], gl_ModelViewMatrix[1][0], gl_ModelViewMatrix[2][0]);
  vec3 axis2 = vec3(gl_ModelViewMatrix[0][1], gl_ModelViewMatrix[1][1], gl_ModelViewMatrix[2][1]);
  vec2 offset = a_uv1;

  vec3 position = axis1*offset.x + axis1*offset.y + axis2*offset.x + axis2*-offset.y + a_vertex;
  
  gl_Position = transform * vec4(position, 1.0);
  
  v_tex = a_uv0;
  v_color = a_color;
}
