#version 110

uniform mat4 reflectionMatrix;
uniform mat4 refractionMatrix;
uniform mat4 losMatrix;
uniform float repeatScale;
uniform vec2 translation;

varying vec3 worldPos;
varying float waterDepth;

void main()
{
	worldPos = gl_Vertex.xyz;
	waterDepth = dot(gl_Color.xyz, vec3(255.0, -255.0, 1.0));
	gl_TexCoord[0].st = gl_Vertex.xz*repeatScale + translation;
	gl_TexCoord[1] = reflectionMatrix * gl_Vertex;		// projective texturing
	gl_TexCoord[2] = refractionMatrix * gl_Vertex;
	gl_TexCoord[3] = losMatrix * gl_Vertex;
	gl_Position = ftransform();
}
