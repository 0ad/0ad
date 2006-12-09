vec4 InstancingPosition(vec4 position);

void main()
{
	vec4 worldPos = InstancingPosition(gl_Vertex);	

	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * worldPos;
}
