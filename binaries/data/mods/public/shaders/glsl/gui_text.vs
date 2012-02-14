#version 110

uniform mat4 transform;

varying vec2 v_texcoord;

void main()
{
  gl_Position = transform * gl_Vertex;
  v_texcoord = gl_MultiTexCoord0.xy;
}
