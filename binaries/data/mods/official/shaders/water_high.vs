attribute float vertexDepth;

varying vec3 worldPos;
varying vec3 waterColor;	/* Water colour with LOS multiplied in */
varying float waterDepth;

void main()
{
	worldPos = gl_Vertex.xyz;
	waterColor = gl_Color.xyz;
	waterDepth = vertexDepth;
	gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = ftransform();
}
