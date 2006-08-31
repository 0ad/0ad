uniform mat4 reflectionMatrix;
uniform mat4 refractionMatrix;

attribute float vertexDepth;
attribute float losMultiplier;

varying vec3 worldPos;
varying float w;
varying float waterDepth;
varying float losMod;

void main()
{
	worldPos = gl_Vertex.xyz;
	waterDepth = vertexDepth;
	losMod = losMultiplier;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = reflectionMatrix * gl_Vertex;		// projective texturing
	gl_TexCoord[2] = reflectionMatrix * gl_Vertex;
	w = gl_TexCoord[1].w;
    gl_Position = ftransform();
}
