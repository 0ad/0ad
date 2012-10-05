#version 110

varying vec3 v_tex;
attribute vec3 a_vertex;
attribute vec3 a_uv0;

void main()
{
  gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
  v_tex = gl_MultiTexCoord0.xyz;
}
