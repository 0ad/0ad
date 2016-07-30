#version 120

uniform sampler2D baseTex;
uniform sampler2D losTex;
uniform sampler2D aoTex;
uniform sampler2D normTex;
uniform sampler2D specTex;

uniform sampler2D waterTex;
uniform samplerCube skyCube;

#if USE_SHADOW
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
    #if USE_SHADOW_PCF
      uniform vec4 shadowScale;
    #endif
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

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
varying vec4 v_shadow;
varying vec2 v_los;


float get_shadow(vec4 coords)
{
  #if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
    #if USE_SHADOW_SAMPLER
      #if USE_SHADOW_PCF
        vec2 offset = fract(coords.xy - 0.5);
        vec4 size = vec4(offset + 1.0, 2.0 - offset);
		vec4 weight = (vec4(1.0, 1.0, -0.5, -0.5) + (coords.xy - 0.5*offset).xyxy) * shadowScale.zwzw;
        return (1.0/9.0)*dot(size.zxzx*size.wwyy,
          vec4(shadow2D(shadowTex, vec3(weight.zw, coords.z)).r,
               shadow2D(shadowTex, vec3(weight.xw, coords.z)).r,
               shadow2D(shadowTex, vec3(weight.zy, coords.z)).r,
               shadow2D(shadowTex, vec3(weight.xy, coords.z)).r));
      #else
        return shadow2D(shadowTex, coords.xyz).r;
      #endif
    #else
      if (coords.z >= 1.0)
        return 1.0;
      return (coords.z <= texture2D(shadowTex, coords.xy).x ? 1.0 : 0.0);
    #endif
  #else
    return 1.0;
  #endif
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

	losMod = texture2D(losTex, v_los).a;

	//losMod = texture2D(losMap, gl_TexCoord[3].st).a;

#if USE_SHADOW
	float shadow = get_shadow(vec4(v_shadow.xy - 8*waviness*n.xz, v_shadow.zw));
	float fresShadow = mix(fresnel, fresnel*shadow, dot(sunColor, vec3(0.16666)));
#else
	float fresShadow = fresnel;
#endif
	
	vec3 color = mix(refrColor + 0.3*specular, reflColor + specular, fresShadow);

	gl_FragColor.rgb = color * losMod;


	//gl_FragColor.rgb = mix(refrColor + 0.3*specular, reflColor + specular, fresnel) * losMod;
	
	// Make alpha vary based on both depth (so it blends with the shore) and view angle (make it
	// become opaque faster at lower view angles so we can't look "underneath" the water plane)
	t = 18.0 * max(0.0, 0.7 - v.y);
	gl_FragColor.a = 0.15 * waterDepth * (1.2 + t + fresnel);
}

