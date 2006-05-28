uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 cameraPos;
uniform sampler2D normalMap;
uniform sampler2D reflectionMap;
uniform sampler2D refractionMap;
uniform float shininess;

varying vec3 worldPos;
varying vec3 waterColor;
varying float waterDepth;
varying float w;

void main()
{
	vec3 n, l, h, v;		// normal, vector to light, half-vector and view vector (vector to eye)
	float ndotl, ndoth, ndotv;
	float fresnel;
	float temp;
	vec2 reflCoords, refrCoords;
	vec3 reflCol, refrCol, specular;
	
	n = normalize(texture2D(normalMap, gl_TexCoord[0].st).xzy - vec3(0.5, 0.5, 0.5));
	l = -sunDir;
	v = normalize(cameraPos - worldPos);
	h = normalize(l + v);
	
	ndotl = dot(n, l);
	ndoth = dot(n, h);
	ndotv = dot(n, v);
	
	fresnel = pow(1.0 - ndotv, 0.8);	// A rather arbitrary Fresnel approximation
	
	reflCoords = 0.5 * (gl_TexCoord[1].xy / gl_TexCoord[1].w) + 0.5;
	reflCoords += 2.9 * n.xz / w;		// The 2.9 can be tweaked to make reflections "wavier"
	
	refrCoords = 0.5 * (gl_TexCoord[2].xy / gl_TexCoord[2].w) + 0.5;
	refrCoords -= 2.6 * n.xz / w;		// The 2.6 can be tweaked to make refractions "wavier"
	
	reflCol = (0.8 + 0.2*ndotl) * texture2D(reflectionMap, reflCoords).rgb;
	
	refrCol = (ambient + ndotl * sunColor) * mix(texture2D(refractionMap, refrCoords).rgb, waterColor, 0.3);
	
	specular = pow(max(0.0, ndoth), shininess) * sunColor * vec3(0.2, 0.2, 0.2);	// specular color can be changed here
	
	gl_FragColor.rgb = mix(refrCol + 0.3*specular, reflCol + specular, fresnel);
	
	temp = 8.0 * max(0.0, 0.7-v.y);
	gl_FragColor.a = 0.15 * waterDepth * (temp + mix(1.2, 2.2, fresnel));
}
