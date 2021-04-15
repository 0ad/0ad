#version 110

#if DEBUG_TEXTURED
uniform sampler2D baseTex;

varying vec2 v_tex;
#else
uniform vec4 color;
#endif

void main()
{
#if DEBUG_TEXTURED
    gl_FragColor = texture2D(baseTex, v_tex);
#else
    gl_FragColor = color;
#endif
}
