#version 110

uniform vec4 colorAdd;
uniform vec4 colorMul;
uniform sampler2D tex;

varying vec2 v_texcoord;

void main()
{
  gl_FragColor = (texture2D(tex, v_texcoord) + colorAdd) * colorMul;
}
