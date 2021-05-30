#version 110

uniform vec4 colorAdd;
uniform vec4 colorMul;
uniform sampler2D tex;

varying vec2 v_uv;

void main()
{
	gl_FragColor = clamp(texture2D(tex, v_uv) * colorMul + colorAdd, 0.0, 1.0);
}
