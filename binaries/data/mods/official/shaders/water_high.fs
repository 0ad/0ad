uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 cameraPos;
uniform sampler2D normalMap;
uniform float shininess;

varying vec3 worldPos;
varying vec3 waterColor;	/* Water colour with LOS multiplied in */
varying float waterDepth;

void main()
{
	vec3 n, l, h, v;		/* normal, vector to light, half-vector and view vector (vector to eye) */
	float ndotl, ndoth, ndotv;
	float fresnel;
	float t;
	vec3 color;
	vec3 specular;
	
	n = normalize(texture2D(normalMap, gl_TexCoord[0].st).xzy - vec3(0.5, 0.5, 0.5));
	l = -sunDir;
	v = normalize(cameraPos - worldPos);
	h = normalize(l + v);
	
	ndotl = dot(n, l);
	ndoth = dot(n, h);
	ndotv = dot(n, v);
	
	fresnel = pow(1.0 - ndotv, 0.8);	/* A rather arbitrary Fresnel approximation */
	
	color = ambient * waterColor;
	specular = vec3(0.0, 0.0, 0.0);
	if(ndotl > 0.0)
	{
		color += ndotl * sunColor * waterColor;
		specular = pow(ndoth, shininess) * sunColor * 0.6;		/* Assume specular color is (.6,.6,.6) for now */
	}
	
    gl_FragColor.rgb = color + specular;
	
	t = 8.0 * max(0.0, 0.7-v.y);
	gl_FragColor.a = mix(0.15*waterDepth*(1.2 + t), 0.15*waterDepth*(2.0 + t), fresnel) * fresnel;
}
