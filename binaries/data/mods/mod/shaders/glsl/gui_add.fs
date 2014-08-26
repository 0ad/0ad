#version 110

uniform sampler2D tex;
uniform vec4 color;

varying vec2 v_tex;

void main()
{
  gl_FragColor = texture2D(tex, v_tex) + color;
}
