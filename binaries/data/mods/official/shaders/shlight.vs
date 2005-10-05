uniform vec3 SHCoefficients[9];

vec3 lighting(vec3 normal)
{
	vec3 color = SHCoefficients[0];
	vec3 normalsq = normal*normal;
	color += SHCoefficients[1]*normal.x;
	color += SHCoefficients[2]*normal.y;
	color += SHCoefficients[3]*normal.z;
	color += SHCoefficients[4]*(normal.x*normal.z);
	color += SHCoefficients[5]*(normal.z*normal.y);
	color += SHCoefficients[6]*(normal.y*normal.x);
	color += SHCoefficients[7]*(3*normalsq.z-1);
	color += SHCoefficients[8]*(normalsq.x-normalsq.y);
	return color;
}
