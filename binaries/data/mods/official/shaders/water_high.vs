uniform mat4 reflectionMatrix;
uniform mat4 refractionMatrix;

attribute float vertexDepth;

varying vec3 worldPos;
varying vec3 waterColor;
varying float waterDepth;
varying float w;

void main()
{
	worldPos = gl_Vertex.xyz;
	waterColor = gl_Color.xyz;
	waterDepth = vertexDepth;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = reflectionMatrix * gl_Vertex;
	gl_TexCoord[2] = reflectionMatrix * gl_Vertex;
	w = gl_TexCoord[1].w;
    gl_Position = ftransform();
}
