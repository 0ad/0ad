#version 110

uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 cameraPos;
uniform sampler2D normalMap;
uniform sampler2D reflectionMap;
uniform sampler2D refractionMap;
uniform sampler2D losMap;
uniform float shininess;		// Blinn-Phong specular strength
uniform float specularStrength;	// Scaling for specular reflection (specular color is (this,this,this))
uniform float waviness;			// "Wildness" of the reflections and refractions; choose based on texture
uniform vec3 tint;				// Tint for refraction (used to simulate particles in water)
uniform float murkiness;		// Amount of tint to blend in with the refracted colour
uniform float fullDepth;		// Depth at which to use full murkiness (shallower water will be clearer)
uniform vec3 reflectionTint;	// Tint for reflection (used for really muddy water)
uniform float reflectionTintStrength;	// Strength of reflection tint (how much of it to mix in)

uniform vec3 fogColor;
uniform vec2 fogParams;

varying vec3 worldPos;
varying float waterDepth;

vec3 get_fog(vec3 color)
{
  float density = fogParams.x;
  float maxFog = fogParams.y;

  const float LOG2 = 1.442695;
  float z = gl_FragCoord.z / gl_FragCoord.w;
  float fogFactor = exp2(-density * density * z * z * LOG2);

  fogFactor = fogFactor * (1.0 - maxFog) + maxFog;

  fogFactor = clamp(fogFactor, 0.0, 1.0);

  return mix(fogColor, color, fogFactor);
}

void main()
{
	vec3 n, l, h, v;		// Normal, light vector, half-vector and view vector (vector to eye)
	float ndotl, ndoth, ndotv;
	float fresnel;
	float t;				// Temporary variable
	vec2 reflCoords, refrCoords;
	vec3 reflColor, refrColor, specular;
	float losMod;

	n = normalize(texture2D(normalMap, gl_TexCoord[0].st).xzy - vec3(0.5, 0.5, 0.5));
	l = -sunDir;
	v = normalize(cameraPos - worldPos);
	h = normalize(l + v);
	
	ndotl = dot(n, l);
	ndoth = dot(n, h);
	ndotv = dot(n, v);
	
	fresnel = pow(1.0 - ndotv, 0.8);	// A rather random Fresnel approximation
	
	refrCoords = (0.5*gl_TexCoord[2].xy - 0.8*waviness*n.xz) / gl_TexCoord[2].w + 0.5;	// Unbias texture coords
	reflCoords = (0.5*gl_TexCoord[1].xy + waviness*n.xz) / gl_TexCoord[1].w + 0.5;	// Unbias texture coords
	
	reflColor = mix(texture2D(reflectionMap, reflCoords).rgb, sunColor * reflectionTint, 
					reflectionTintStrength);
	
	refrColor = (0.5 + 0.5*ndotl) * mix(texture2D(refractionMap, refrCoords).rgb, sunColor * tint,
					murkiness * clamp(waterDepth / fullDepth, 0.0, 1.0)); // Murkiness and tint at this pixel (tweaked based on lighting and depth)
	
	specular = pow(max(0.0, ndoth), shininess) * sunColor * specularStrength;

	losMod = texture2D(losMap, gl_TexCoord[3].st).a;
	losMod = losMod < 0.03 ? 0.0 : losMod;

	gl_FragColor.rgb = mix(refrColor + 0.3*specular, reflColor + specular, fresnel);

	gl_FragColor.rgb = get_fog(gl_FragColor.rgb);

	gl_FragColor.rgb *= losMod;
	
	// Make alpha vary based on both depth (so it blends with the shore) and view angle (make it
	// become opaque faster at lower view angles so we can't look "underneath" the water plane)
	t = 18.0 * max(0.0, 0.7 - v.y);
	gl_FragColor.a = 0.15 * waterDepth * (1.2 + t + fresnel);
}
