#version 110

uniform sampler2D baseTex;

varying vec2 v_tex;

void main()
{
  gl_FragColor = texture2D(baseTex, v_tex);

  #ifdef REQUIRE_ALPHA_GREATER
    if (gl_FragColor.a <= REQUIRE_ALPHA_GREATER)
      discard;
  #endif
}
