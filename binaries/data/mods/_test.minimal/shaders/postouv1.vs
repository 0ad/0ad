uniform vec4 TextureMatrix1;
uniform vec4 TextureMatrix2;
uniform vec4 TextureMatrix3;

vec4 postouv1(vec4 pos)
{
	vec3 tmp;

	tmp.x = dot(TextureMatrix1, pos);
	tmp.y = dot(TextureMatrix2, pos);
	tmp.z = dot(TextureMatrix3, pos);

	return vec4(tmp, 1.0);
}
