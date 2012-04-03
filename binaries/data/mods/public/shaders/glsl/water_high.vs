#version 110

uniform mat4 reflectionMatrix;
uniform mat4 refractionMatrix;
uniform mat4 losMatrix;
uniform float repeatScale;
uniform vec2 translation;

varying vec3 worldPos;
varying float waterDepth;

attribute vec3 a_vertex;
attribute vec4 a_encodedDepth;

void main()
{
	worldPos = a_vertex.xyz;
	waterDepth = dot(a_encodedDepth.xyz, vec3(255.0, -255.0, 1.0));
	gl_TexCoord[0].st = a_vertex.xz*repeatScale + translation;
	gl_TexCoord[1] = reflectionMatrix * vec4(a_vertex, 1.0);		// projective texturing
	gl_TexCoord[2] = refractionMatrix * vec4(a_vertex, 1.0);
	gl_TexCoord[3] = losMatrix * vec4(a_vertex, 1.0);

	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
}
