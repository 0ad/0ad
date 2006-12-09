vec3 lighting(vec3 normal);
vec4 postouv1(vec4 pos);

void main()
{
	gl_FrontColor = vec4(lighting(gl_Normal),1.0) * gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = postouv1(gl_Vertex);
	gl_Position = ftransform();
}
