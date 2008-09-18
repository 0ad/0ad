uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;

vec3 lighting(vec3 normal)
{
	vec3 color = ambient;
	float d = max(0.0, -dot(normal, sunDir));
	color += d * sunColor;
	return color;
}
