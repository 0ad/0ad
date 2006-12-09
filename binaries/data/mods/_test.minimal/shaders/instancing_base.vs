
// 3x4 part of the model-to-world matrix
attribute vec4 Instancing1;
attribute vec4 Instancing2;
attribute vec4 Instancing3;

// Calculate a normal that has been transformed for instancing
vec3 InstancingNormal(vec3 normal)
{
	vec3 tmp;

	tmp.x = dot(vec3(Instancing1), normal);
	tmp.y = dot(vec3(Instancing2), normal);
	tmp.z = dot(vec3(Instancing3), normal);
	
	return tmp;
}

// Calculate position, transformed for instancing
vec4 InstancingPosition(vec4 position)
{
	vec3 tmp;

	tmp.x = dot(Instancing1, position);
	tmp.y = dot(Instancing2, position);
	tmp.z = dot(Instancing3, position);
	
	return vec4(tmp, 1.0);
}
