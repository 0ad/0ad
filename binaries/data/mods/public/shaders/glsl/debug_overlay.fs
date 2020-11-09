#version 110

uniform sampler2D baseTex;

varying vec2 v_tex;

void main()
{
    vec4 base = texture2D(baseTex, v_tex);
    gl_FragColor = base;
}
