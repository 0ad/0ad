vec3 lighting(vec3 normal);

vec3 InstancingNormal(vec3 normal);
vec4 InstancingPosition(vec4 position);

void main()
{
	vec3 normal = InstancingNormal(gl_Normal);
	vec4 worldPos = InstancingPosition(gl_Vertex);	

	gl_FrontColor = vec4(lighting(normal),1.0) * gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * worldPos;
}
