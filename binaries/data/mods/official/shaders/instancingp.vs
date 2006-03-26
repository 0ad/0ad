vec4 InstancingPosition(vec4 position);
vec4 postouv1(vec4 pos);

void main()
{
	vec4 worldPos = InstancingPosition(gl_Vertex);

	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = postouv1(worldPos);
	gl_Position = gl_ModelViewProjectionMatrix * worldPos;
}
