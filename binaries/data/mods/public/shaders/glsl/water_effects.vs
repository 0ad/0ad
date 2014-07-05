#version 110

// This is a lightened version of water_high.vs
uniform float repeatScale;

uniform float windAngle;
uniform float time;
uniform float mapSize;

varying vec3 worldPos;
varying vec4 waterInfo;

attribute vec3 a_vertex;
attribute vec4 a_waterInfo;


void main()
{
	worldPos = a_vertex;
	waterInfo = a_waterInfo;
	
	float newX = a_vertex.x * cos(-windAngle) - a_vertex.z * sin(-windAngle);
	float newY = a_vertex.x * sin(-windAngle) + a_vertex.z * cos(-windAngle);
	
	gl_TexCoord[0] = vec4(newX,newY,time,0.0);
	gl_TexCoord[0].xy *= repeatScale;
	
	gl_TexCoord[3].zw = vec2(a_vertex.xz)/mapSize;
		
	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
}
