#version 110

uniform sampler2D tex;

varying vec2 v_tex;

void main()
{
  gl_FragColor = texture2D(tex, v_tex);
}
