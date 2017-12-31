#version 110

uniform sampler2D baseTex;
uniform sampler2D losTex;

varying vec2 v_tex;
varying vec2 v_los;
varying vec4 v_color;

uniform vec3 sunColor;
uniform vec3 fogColor;
uniform vec2 fogParams;

vec4 get_fog(vec4 color)
{
	float density = fogParams.x;
	float maxFog = fogParams.y;
	
	const float LOG2 = 1.442695;
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = exp2(-density * density * z * z * LOG2);
	
	fogFactor = fogFactor * (1.0 - maxFog) + maxFog;
	
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	
	return vec4(mix(fogColor, color.rgb, fogFactor),color.a);
}

void main()
{
	vec4 color = texture2D(baseTex, v_tex) * vec4((v_color.rgb + sunColor)/2.0,v_color.a);
	
    float los = texture2D(losTex, v_los).a;
    los = los < 0.03 ? 0.0 : los;
    color.rgb *= los;

#if USE_FOG
	color = get_fog(color);
#endif

	gl_FragColor = color;
}
