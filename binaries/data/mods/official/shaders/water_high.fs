uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 cameraPos;
uniform sampler2D normalMap;
uniform sampler2D reflectionMap;
uniform sampler2D refractionMap;
uniform float shininess;
uniform float waviness;
uniform vec3 tint;
uniform float murkiness;
uniform float fullDepth;

varying vec3 worldPos;
varying float waterDepth;
varying float w;

const vec3 specularColor = vec3(0.15, 0.15, 0.15);

void main()
{
	vec3 n, l, h, v;		// Normal, light vector, half-vector and view vector (vector to eye)
	float ndotl, ndoth, ndotv;
	float fresnel;
	float myMurkiness;		// Murkiness and tint at this pixel (tweaked based on lighting and depth)
	vec3 myTint;
	float t;
	vec2 reflCoords, refrCoords;
	vec3 reflColor, refrColor, specular;
	
	n = normalize(texture2D(normalMap, gl_TexCoord[0].st).xzy - vec3(0.5, 0.5, 0.5));
	l = -sunDir;
	v = normalize(cameraPos - worldPos);
	h = normalize(l + v);
	
	ndotl = dot(n, l);
	ndoth = dot(n, h);
	ndotv = dot(n, v);
	
	fresnel = pow(1.0 - ndotv, 0.8);	// A rather random Fresnel approximation
	
	reflCoords = 0.5 * (gl_TexCoord[1].xy / gl_TexCoord[1].w) + 0.5;	// Unbias texture coords
	reflCoords += waviness * n.xz / w;
	
	refrCoords = 0.5 * (gl_TexCoord[2].xy / gl_TexCoord[2].w) + 0.5;	// Unbias texture coords
	refrCoords -= 0.8 * waviness * n.xz / w;		// Refractions can be slightly less wavy
	
	reflColor = texture2D(reflectionMap, reflCoords).rgb;
	
	myMurkiness = murkiness * min(waterDepth / fullDepth, 1.0);
	myTint = (ambient + ndotl * sunColor) * tint;
	refrColor = (0.6 + 0.4*ndotl) * mix(texture2D(refractionMap, refrCoords).rgb, myTint, myMurkiness);
	
	specular = pow(max(0.0, ndoth), shininess) * sunColor * specularColor;
	
	gl_FragColor.rgb = mix(refrColor + 0.3*specular, reflColor + specular, fresnel);
	
	// Make alpha vary based on both depth (so it blends with the shore) and view angle (make it
	// become opaque faster at lower view angles so we can't look "underneath" the water plane)
	t = 18.0 * max(0.0, 0.7 - v.y);
	gl_FragColor.a = 0.15 * waterDepth * (1.2 + t + fresnel);
}
