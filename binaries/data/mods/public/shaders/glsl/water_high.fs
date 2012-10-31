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
uniform vec3 reflectionTint;	// Tint for reflection (used for really muddy water)
uniform float reflectionTintStrength;	// Strength of reflection tint (how much of it to mix in)
uniform vec3 color;				// color of the water

uniform vec3 fogColor;
uniform vec2 fogParams;

uniform vec2 screenSize;
uniform float time;

#if USE_SUPERFANCYWATER
	uniform sampler2D heightmap;
	uniform sampler2D infoTex;
	uniform sampler2D normalMap2;
	uniform sampler2D Foam;
	uniform sampler2D depthTex;
	uniform sampler2D waveTex;

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
#endif


varying vec3 worldPos;
varying float waterDepth;
varying vec4 v_shadow;

#if USE_SUPERFANCYWATER
float get_shadow(vec4 coords)
{
	#if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
		#if USE_SHADOW_SAMPLER
			#if USE_SHADOW_PCF
				vec2 offset = fract(coords.xy - 0.5);
				vec4 size = vec4(offset + 1.0, 2.0 - offset);
				vec4 weight = (vec4(2.0 - 1.0 / size.xy, 1.0 / size.zw - 1.0) + (coords.xy - offset).xyxy) * shadowScale.zwzw;
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
#endif

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
	#if USE_SUPERFANCYWATER
		vec4 heightmapval = texture2D(heightmap, gl_TexCoord[3].zw);
		vec2 beachOrientation = heightmapval.rb - vec2(0.5,0.5);
		float distToShore = heightmapval.a;
	#endif
	
  	vec3 n, l, h, v;		// Normal, light vector, half-vector and view vector (vector to eye)
	float ndotl, ndoth, ndotv;
	float fresnel;
	float t;				// Temporary variable
	vec2 reflCoords, refrCoords;
	vec3 reflColor, refrColor, specular;
	float losMod;
	
	vec3 ww = texture2D(normalMap, (gl_TexCoord[0].st) * mix(2.0,0.8,waviness/10.0) +gl_TexCoord[0].zw).xzy;

	#if USE_SUPERFANCYWATER
		vec3 ww2 = texture2D(normalMap2, (gl_TexCoord[0].st) * mix(2.0,0.8,waviness/10.0) +gl_TexCoord[0].zw).xzy;
		ww = mix(ww, ww2, mod(time * 60.0, 8.0) / 8.0);
		vec3 waves = texture2D(waveTex, gl_FragCoord.xy/screenSize).rbg - vec3(0.5,0.5,0.5);
		float waveFoam = 0.0;//texture2D(waveTex, gl_FragCoord.xy/screenSize).a;
		n = normalize(mix(waves, ww - vec3(0.5, 0.5, 0.5) , clamp(distToShore*3.0,0.4,1.0)));
	#else
		n = normalize(ww - vec3(0.5, 0.5, 0.5));
	#endif
	
	float wavyFactor = waviness * 0.125;
	
	n = mix(vec3(0.0,1.0,0.0),n,wavyFactor);
	
	l = -sunDir;
	v = normalize(cameraPos - worldPos);
	h = normalize(l + v);
	
	ndotl = (dot(n, l) + 1.0) /2.0;
	ndoth = dot(n, h);
	ndotv = dot(n, v);
	
	
	#if USE_SUPERFANCYWATER
		// Don't change these two. They should match the values in the config (TODO: dec uniforms).
		float zNear = 2.0;
		float zFar = 4096.0;
		
		// Okay so here it's a tad complicated. I want to distort the depth buffer along the waves for a nice effect.
		// However this causes a problem around underwater objects (think fishes): on some pixels, the depth will be seen as the same as the fishes'
		// and the color will be grass ( cause I don't distort the refraction coord by exactly the same stuff)
		// Also, things like towers with the feet in water would cause the buffer to see the depth as actually negative in some places.
		// So what I do is first check the undistorted depth, then I compare with the distorted value and fix.
		float water_b = gl_FragCoord.z;
		float water_n = 2.0 * water_b - 1.0;
		float waterDBuffer = 2.0 * zNear * zFar / (zFar + zNear - water_n * (zFar - zNear));
		
		float undistortedBuffer = texture2D(depthTex, (gl_FragCoord.xy) / screenSize).x;
		
		float undisto_z_b = texture2D(depthTex, (gl_FragCoord.xy) / screenSize).x;
		float undisto_z_n = 2.0 * undisto_z_b - 1.0;
		float waterDepth_undistorted = (2.0 * zNear * zFar / (zFar + zNear - undisto_z_n * (zFar - zNear)) - waterDBuffer);
		
		vec2 depthCoord = clamp((gl_FragCoord.xy) / screenSize - n.xz*clamp( waterDepth_undistorted/400.0,0.0,0.05) , 0.001, 0.999);
		
		float z_b = texture2D(depthTex, depthCoord).x;
		
		if (z_b < undisto_z_b)
			z_b = undisto_z_b;
		float z_n = 2.0 * z_b - 1.0;
		float waterDepth2 = (2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear)) - waterDBuffer);
		
		float distoFactor = clamp(waterDepth2/10.0,0.0,7.0);
	#endif
  	
	fresnel = pow(1.0 - ndotv, 1.3333);// approximation
	
	#if USE_SUPERFANCYWATER
		// texture is rotated 90°, moves slowly.
		vec2 foam1RC = vec2(-gl_TexCoord[0].t,gl_TexCoord[0].s)*1.3  - 0.012*n.xz + vec2(time*0.004,time*0.003);
		// texture is not rotated, moves twice faster in the opposite direction, translated.
		vec2 foam2RC = gl_TexCoord[0].st*1.8 + vec2(time*-0.019,time*-0.012) - 0.012*n.xz + vec2(0.4,0.2);
		
		vec2 WaveRocking = cos(time*1.2566) * beachOrientation * clamp(1.0 - distToShore*2.0,0.1,1.0)/3.0;
		vec4 foam1 = texture2D(Foam, foam1RC + vec2(-WaveRocking.t,WaveRocking.s));
		vec4 foam2 = foam1.r*texture2D(Foam, foam2RC + WaveRocking);
		
		vec3 finalFoam = min((foam2).rrr * texture2D(infoTex,gl_TexCoord[3].zw).g,1.0);
		
		if ((1.0 - finalFoam.r) >= wavyFactor)
			finalFoam = vec3(0.0);
		
		// waves bypass the regular foam restrictions.
		finalFoam += min( max(0.0,-waves.b) * texture2D(Foam, foam1RC).r, 1.0)*3.0 * max(0.0,wavyFactor-0.1);
		finalFoam *= sunColor;
	#endif
		
	#if USE_SUPERFANCYWATER
		refrCoords = clamp( (0.5*gl_TexCoord[2].xy - n.xz * distoFactor) / gl_TexCoord[2].w + 0.5,0.0,1.0);	// Unbias texture coords
	#else
		refrCoords = clamp( (0.5*gl_TexCoord[2].xy - n.xz * 6.0) / gl_TexCoord[2].w + 0.5,0.0,1.0);	// Unbias texture coords
	#endif
	
	reflCoords = clamp( (0.5*gl_TexCoord[1].xy + 10.0*n.xz) / gl_TexCoord[1].w + 0.5,0.0,1.0);	// Unbias texture coords
	reflColor = mix(texture2D(reflectionMap, reflCoords).rgb, sunColor * reflectionTint, reflectionTintStrength);
	
	#if USE_SUPERFANCYWATER
		vec3 refColor = texture2D(refractionMap, refrCoords).rgb;
		float luminance = (1.0 - clamp((waterDepth2/mix(300.0,1.0, pow(murkiness,0.2) )), 0.0, 1.0));
		float colorExtinction = clamp(waterDepth2*murkiness/5.0,0.0,1.0);
		refrColor = (0.5 + 0.5*ndotl) * mix(color,mix(refColor,refColor*tint,colorExtinction),luminance*luminance);
	#else
		// fake it. It won't look as good but it will give a similar result.
		float perceivedDepth = waterDepth / v.y;
		vec3 refColor = texture2D(refractionMap, refrCoords).rgb;
		float luminance = (1.0 - clamp((perceivedDepth/mix(300.0,1.0, pow(murkiness,0.2) )), 0.0, 1.0));
		float colorExtinction = clamp(perceivedDepth*murkiness/5.0,0.0,1.0);
		refrColor = (0.5 + 0.5*ndotl) * mix(color,mix(refColor,refColor*tint,colorExtinction),luminance*luminance);
	#endif
	
	specular = pow(ndoth, shininess) * sunColor * specularStrength;
	
	losMod = texture2D(losMap, gl_TexCoord[3].st).a;
	losMod = losMod < 0.03 ? 0.0 : losMod;
	
	#if USE_SUPERFANCYWATER
		float shadow = get_shadow(vec4(v_shadow.xy - 8.0*waviness*n.xz, v_shadow.zw));
		float fresShadow = mix(fresnel, fresnel*shadow, 0.05 + (murkiness * 0.15));
		vec3 colour = mix(refrColor*(shadow/5.0 + 0.8) + fresnel*shadow*specular, reflColor + fresnel*shadow*specular, fresShadow) + max(ndotl,0.4)*(finalFoam)*(shadow/2.0 + 0.5);
		colour = mix( texture2D(refractionMap, (0.5*gl_TexCoord[2].xy) / gl_TexCoord[2].w + 0.5).rgb ,colour, clamp(waterDepth2,0.0,1.0));
	#else
		vec3 colour = mix(refrColor + fresnel*specular, reflColor + fresnel*specular, fresnel);
	#endif
	
	gl_FragColor.rgb = get_fog(colour) * losMod;

	#if USE_SUPERFANCYWATER
		gl_FragColor.a = clamp(distToShore-0.012,waterDepth2/2.0,1.0) + finalFoam.r;
	#else
		// Make alpha vary based on both depth (so it blends with the shore) and view angle (make it
		// become opaque faster at lower view angles so we can't look "underneath" the water plane)
		t = 18.0 * max(0.0, 0.9 - v.y);
		gl_FragColor.a = clamp(0.15 * waterDepth * (1.2 + t + fresnel),0.0,1.0);
	#endif
}
