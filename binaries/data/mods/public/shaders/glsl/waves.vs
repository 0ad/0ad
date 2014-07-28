#version 110

attribute vec2 a_uv0;

attribute vec2 a_normal;

attribute vec3 a_basePosition;
attribute vec3 a_apexPosition;
attribute vec3 a_splashPosition;
attribute vec3 a_retreatPosition;

uniform float time;
uniform float translation;
uniform float width;

uniform mat4 transform;

varying float ttime;
varying vec2 normal;

void main()
{
	normal = a_normal;
	gl_TexCoord[0].xy = a_uv0.xy;
	
	float tttime = mod(time + translation ,10.0);
	ttime = tttime;
		
	vec3 pos = mix(a_basePosition,a_apexPosition, clamp(ttime/3.0,0.0,1.0));
	pos = mix (pos, a_splashPosition, clamp(sin((min(ttime,6.1415926536)-3.0)/2.0),0.0,1.0));
	pos = mix (pos, a_retreatPosition, clamp(  1.0 - cos(max(0.0,ttime-6.1415926536)/2.0)  ,0.0,1.0));
	
	gl_Position = transform * vec4(pos, 1.0);
}
