#version 120

#include "common/debug_fragment.h"
#include "common/los_fragment.h"
#include "common/shadows_fragment.h"

uniform sampler2D baseTex;
uniform sampler2D aoTex;
uniform sampler2D normTex;
uniform sampler2D specTex;

uniform sampler2D waterTex;
uniform samplerCube skyCube;

#if USE_OBJECTCOLOR
  uniform vec3 objectColor;
#else
#if USE_PLAYERCOLOR
  uniform vec3 playerColor;
#endif
#endif

uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec3 sunColor;
uniform vec3 sunDir;
uniform vec3 cameraPos;

uniform float specularStrength;
uniform float waviness;
uniform vec3 waterTint;
uniform float murkiness;
uniform vec3 reflectionTint;
uniform float reflectionTintStrength;

float waterDepth = 4.0;
float fullDepth = 5.0;		// Depth at which to use full murkiness (shallower water will be clearer)

varying vec4 worldPos;
varying vec4 v_tex;

void main()
{
	vec3 n, l, h, v;		// Normal, light vector, half-vector and view vector (vector to eye)
	float ndotl, ndoth, ndotv;
	float fresnel;
	float t;				// Temporary variable
	vec2 reflCoords, refrCoords;
	vec3 reflColor, refrColor, specular;

	//vec4 wtex = textureGrad(waterTex, vec3(fract(v_tex.xy), v_tex.z), dFdx(v_tex.xy), dFdy(v_tex.xy));
	vec4 wtex = texture2D(waterTex, fract(v_tex.xy));

	n = normalize(wtex.xzy - vec3(0.5, 0.5, 0.5));
	l = -sunDir;
	v = normalize(cameraPos - worldPos.xyz);
	h = normalize(l + v);

	ndotl = dot(n, l);
	ndoth = dot(n, h);
	ndotv = dot(n, v);

	fresnel = pow(1.0 - ndotv, 0.8);	// A rather random Fresnel approximation

	//refrCoords = (0.5*gl_TexCoord[2].xy - 0.8*waviness*n.xz) / gl_TexCoord[2].w + 0.5;	// Unbias texture coords
	//reflCoords = (0.5*gl_TexCoord[1].xy + waviness*n.xz) / gl_TexCoord[1].w + 0.5;	// Unbias texture coords

	//vec3 dir = normalize(v + vec3(waviness*n.x, 0.0, waviness*n.z));

	vec3 eye = reflect(v, n);

	vec3 tex = textureCube(skyCube, eye).rgb;

	reflColor = mix(tex, sunColor * reflectionTint,
					reflectionTintStrength);

	//waterDepth = 4.0 + 2.0 * dot(abs(v_tex.zw - 0.5), vec2(0.5));
	waterDepth = 4.0;

	//refrColor = (0.5 + 0.5*ndotl) * mix(texture2D(refractionMap, refrCoords).rgb, sunColor * tint,
	refrColor = (0.5 + 0.5*ndotl) * mix(vec3(0.3), sunColor * waterTint,
					murkiness * clamp(waterDepth / fullDepth, 0.0, 1.0)); // Murkiness and tint at this pixel (tweaked based on lighting and depth)

	specular = pow(max(0.0, ndoth), 150.0f) * sunColor * specularStrength;

#if USE_SHADOW
	float shadow = getShadowOnLandscape();
	float fresShadow = mix(fresnel, fresnel*shadow, dot(sunColor, vec3(0.16666)));
#else
	float fresShadow = fresnel;
#endif

	vec3 color = mix(refrColor + 0.3*specular, reflColor + specular, fresShadow);
	color *= getLOS();

	gl_FragColor.rgb = applyDebugColor(color, 1.0);

	// Make alpha vary based on both depth (so it blends with the shore) and view angle (make it
	// become opaque faster at lower view angles so we can't look "underneath" the water plane)
	t = 18.0 * max(0.0, 0.7 - v.y);
	gl_FragColor.a = 0.15 * waterDepth * (1.2 + t + fresnel);
}
