#version 110

uniform sampler2D baseTex;

varying vec2 v_tex;
varying vec4 v_color;

void main()
{
  gl_FragColor = texture2D(baseTex, v_tex) * v_color;
}
