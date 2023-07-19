#ifndef INCLUDED_COMMON_SHADING
#define INCLUDED_COMMON_SHADING

vec3 calculateNormal(vec3 surfaceNormal, vec3 packedTextureNormal, mat3 tangentBitangentNormal, float textureNormalStrength)
{
	vec3 localNormal = packedTextureNormal.rgb * 2.0 - 1.0;
	localNormal.y = -localNormal.y;
	vec3 normal = normalize(tangentBitangentNormal * localNormal);
#if USE_NORMAL_MAP
	if (textureNormalStrength <= 1.0)
		normal = normalize(mix(surfaceNormal, normal, textureNormalStrength));
	else
		normal = normalize(normal - surfaceNormal * min(1.0, textureNormalStrength - 1.0));
#endif
	return normal;
}

vec3 calculateShading(vec3 albedo, vec3 sunDiffuse, vec3 specular, vec3 ambient, float shadow, float ao)
{
	return (albedo * sunDiffuse + specular.rgb) * shadow + albedo * ambient * ao;
}

#endif // INCLUDED_COMMON_SHADING
