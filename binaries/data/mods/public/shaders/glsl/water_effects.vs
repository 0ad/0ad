#version 110

// This is a lightened version of water_high.vs
uniform float repeatScale;
uniform vec2 translation;

uniform float time;
uniform float mapSize;

varying vec3 worldPos;

attribute vec3 a_vertex;

void main()
{
	worldPos = a_vertex;
	
	gl_TexCoord[0] = vec4(a_vertex.xz*repeatScale,translation);
	gl_TexCoord[3].zw = vec2(a_vertex.xz)/mapSize;
		
	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
}
