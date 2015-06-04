#version 110

uniform sampler2D baseTex;
uniform vec4 colorMul;
varying vec2 v_tex;

void main()
{
  gl_FragColor = texture2D(baseTex, v_tex) * colorMul;
}
