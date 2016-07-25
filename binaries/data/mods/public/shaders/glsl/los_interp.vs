#version 110

uniform mat4 transform;
uniform vec2 losTransform;

varying vec2 v_tex;

attribute vec3 a_vertex;
attribute vec2 a_uv0;


varying vec2 v_los;

void main()
{  
  gl_Position = vec4(a_vertex, 1.0);

  v_tex = a_uv0;
}
