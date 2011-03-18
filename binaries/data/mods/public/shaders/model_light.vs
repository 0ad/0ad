vec3 lighting(vec3 normal);

void main()
{
	gl_FrontColor = clamp(vec4(lighting(gl_Normal), 1.0) * gl_Color, 0.0, 1.0);
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}
