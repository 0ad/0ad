#version 110

uniform sampler2D tex;
uniform vec4 color;

varying vec2 v_tex;

void main()
{
	vec4 t = texture2D(tex, v_tex);
	gl_FragColor.rgb = t.rgb + color.rgb * t.a;
	gl_FragColor.a = t.a;
}
