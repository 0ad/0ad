#version 110

varying vec2 v_tex;

uniform sampler2D losTex1, losTex2;

uniform vec3 delta;


void main(void)
{	
	float los2 = texture2D(losTex1, v_tex).a;
	float los1 = texture2D(losTex2, v_tex).a;

	gl_FragColor.a = mix(los1, los2, clamp(delta.r, 0.0, 1.0));
}

