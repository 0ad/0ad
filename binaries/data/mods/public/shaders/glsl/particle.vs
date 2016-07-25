#version 110

uniform mat4 transform;
uniform mat4 modelViewMatrix;
uniform vec2 losTransform;

varying vec2 v_tex;
varying vec2 v_los;
varying vec4 v_color;

attribute vec3 a_vertex;
attribute vec4 a_color;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

void main()
{
  vec3 axis1 = vec3(modelViewMatrix[0][0], modelViewMatrix[1][0], modelViewMatrix[2][0]);
  vec3 axis2 = vec3(modelViewMatrix[0][1], modelViewMatrix[1][1], modelViewMatrix[2][1]);
  vec2 offset = a_uv1;

  vec3 position = axis1*offset.x + axis1*offset.y + axis2*offset.x + axis2*-offset.y + a_vertex;
  
  gl_Position = transform * vec4(position, 1.0);
	
  v_los = position.xz * losTransform.x + losTransform.y;
  v_tex = a_uv0;
  v_color = a_color;
}
