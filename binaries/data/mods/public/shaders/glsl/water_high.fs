#version 110

// Environment settings
uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform mat4 skyBoxRot;

uniform vec3 cameraPos;

uniform sampler2D losMap;

uniform float waviness;			// "Wildness" of the reflections and refractions; choose based on texture
uniform vec3 color;				// color of the water
uniform vec3 tint;				// Tint for refraction (used to simulate particles in water)
uniform float murkiness;		// Amount of tint to blend in with the refracted color

uniform float windAngle;
varying vec2 WindCosSin;

uniform vec3 fogColor;
uniform vec2 fogParams;

uniform vec2 screenSize;
uniform float time;

varying vec3 worldPos;
varying float waterDepth;
varying vec2 waterInfo;

varying vec4 normalCoords;
varying vec3 reflectionCoords;
varying vec3 refractionCoords;
varying vec2 losCoords;

varying float fwaviness;

uniform float mapSize;

uniform samplerCube skyCube;

uniform sampler2D normalMap;
uniform sampler2D normalMap2;

#if USE_FANCY_EFFECTS
	uniform sampler2D waterEffectsTexNorm;
	uniform sampler2D waterEffectsTexOther;
#endif

uniform vec4 waveParams1; // wavyEffect, BaseScale, Flattenism, Basebump
uniform vec4 waveParams2; // Smallintensity, Smallbase, Bigmovement, Smallmovement

uniform sampler2D reflectionMap;

#if USE_REFRACTION
	uniform sampler2D refractionMap;
#endif
#if USE_REAL_DEPTH
	uniform sampler2D depthTex;
#endif

#if USE_SHADOWS_ON_WATER && USE_SHADOW
	varying vec4 v_shadow;
	#if USE_SHADOW_SAMPLER
		uniform sampler2DShadow shadowTex;
		#if USE_SHADOW_PCF
			uniform vec4 shadowScale;
		#endif
	#else
		uniform sampler2D shadowTex;
	#endif
	float get_shadow(vec4 coords)
	{
		#if USE_SHADOWS_ON_WATER && !DISABLE_RECEIVE_SHADOWS
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
#endif

// TODO: convert this to something not only for AABBs
struct Ray {
    vec3 Origin;
    vec3 Direction;
};

float IntersectBox (in Ray ray, in vec3 minimum, in vec3 maximum)
{
    vec3 OMIN = ( minimum - ray.Origin ) / ray.Direction;
    vec3 OMAX = ( maximum - ray.Origin ) / ray.Direction;
    vec3 MAX = max ( OMAX, OMIN );
    return min ( MAX.x, min ( MAX.y, MAX.z ) );
}

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
	//gl_FragColor = texture2D(waterEffectsTex, gl_FragCoord.xy/screenSize);
	//return;
	
	float fresnel;
	float t;				// Temporary variable
	vec2 reflCoords, refrCoords;
	vec3 reflColor, refrColor, specular;
	float losMod, reflMod;
	
	vec3 v = normalize(cameraPos - worldPos);
	
	// Calculate water normals.

	float wavyEffect = waveParams1.r;
	float baseScale = waveParams1.g;
	float flattenism = waveParams1.b;
	float baseBump = waveParams1.a;

	float smallIntensity = waveParams2.r;
	float smallBase = waveParams2.g;
	float BigMovement = waveParams2.b;
	float SmallMovement = waveParams2.a;
	
	float moddedTime = mod(time * 60.0, 8.0) / 8.0;
	
	// This method uses 60 animated water frames. We're blending between each two frames
	// TODO: could probably have fewer frames thanks to this blending.
	// Scale the normal textures by waviness so that big waviness means bigger waves.
	vec3 ww1 = texture2D(normalMap, (normalCoords.st + normalCoords.zw * BigMovement*waviness/10.0) * (baseScale - waviness/wavyEffect)).xzy;
	vec3 ww2 = texture2D(normalMap2, (normalCoords.st + normalCoords.zw * BigMovement*waviness/10.0) * (baseScale - waviness/wavyEffect)).xzy;
	vec3 wwInterp = mix(ww1, ww2, moddedTime) - vec3(0.5,0.0,0.5);

	ww1.x = wwInterp.x * WindCosSin.x - wwInterp.z * WindCosSin.y;
	ww1.z = wwInterp.x * WindCosSin.y + wwInterp.z * WindCosSin.x;
	ww1.y = wwInterp.y;
	
	vec3 smallWW = texture2D(normalMap, (normalCoords.st + normalCoords.zw * SmallMovement*waviness/10.0) * baseScale*3.0).xzy;
	vec3 smallWW2 = texture2D(normalMap2, (normalCoords.st + normalCoords.zw * SmallMovement*waviness/10.0) * baseScale*3.0).xzy;
	vec3 smallWWInterp = mix(smallWW, smallWW2, moddedTime) - vec3(0.5,0.0,0.5);

	smallWW.x = smallWWInterp.x * WindCosSin.x - smallWWInterp.z * WindCosSin.y;
	smallWW.z = smallWWInterp.x * WindCosSin.y + smallWWInterp.z * WindCosSin.x;
	smallWW.y = smallWWInterp.y;

	ww1 += vec3(smallWW)*(fwaviness/10.0*smallIntensity + smallBase);
	
	ww1 = mix(smallWW, ww1, waterInfo.r);
		
	// Flatten them based on waviness.
	vec3 n = normalize(mix(vec3(0.0,1.0,0.0),ww1, clamp(baseBump + fwaviness/flattenism,0.0,1.0)));
	
	#if USE_FANCY_EFFECTS
		vec4 fancyeffects = texture2D(waterEffectsTexNorm, gl_FragCoord.xy/screenSize);
		n = mix(vec3(0.0,1.0,0.0), n,0.5 + waterInfo.r/2.0);
		n.xz = mix(n.xz, fancyeffects.rb,fancyeffects.a/2.0);
	#else
		n = mix(vec3(0.0,1.0,0.0), n,0.5 + waterInfo.r/2.0);
	#endif

	n = vec3(-n.x,n.y,-n.z); // the final wave normal vector
	
	// how perpendicular to the normal our view is. Used for fresnel.
	float ndotv = clamp(dot(n, v),0.0,1.0);
	
	// Fresnel for "how much reflection vs how much refraction".
	fresnel = clamp((pow(1.1 - ndotv, 3.0)) * 1.5,0.0,1.0); // approximation. I'm using 1.1 and not 1.0 because it causes artifacts, see #1714
	
	// Specular lighting vectors
	vec3 specVector = normalize(reflect(sunDir, n));
	float specIntensity = pow(abs(dot(specVector, v)), 200.0);

	specular = specIntensity * sunColor;
	
	float depth;
#if USE_REAL_DEPTH
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
	
	depth = (2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear)) - waterDBuffer);
#else
	depth = waterDepth / (min(0.5,v.y)*1.5*min(0.5,v.y)*2.0);
#endif
	
#if USE_FANCY_EFFECTS
	depth = max(depth,fancyeffects.a);
#endif
	
#if USE_SHADOWS_ON_WATER && USE_SHADOW
	float shadow = get_shadow(vec4(v_shadow.xy, v_shadow.zw));
#endif
	
	// for refraction, we want to adjust the value by v.y slightly otherwise it gets too different between "from above" and "from the sides".
	// And it looks weird (again, we are not used to seeing water from above).
	float fixedVy = max(v.y,0.01);
	
	float distoFactor = clamp(depth/2.0,0.0,7.0);
	
	float murky = mix(200.0,0.1,pow(murkiness,0.25));
	
#if USE_REFRACTION
	// distort the texture coords under where the water is to simulate refraction.
	refrCoords = (0.5 * refractionCoords.xy - n.xz * distoFactor * 3.5) / refractionCoords.z + 0.5;
	vec3 refColor = texture2D(refractionMap, refrCoords).rgb;
	
	// Apply water tint and murk color.
	float extFact = max(0.0,1.0 - (depth*fixedVy/murky));
	float ColextFact = max(0.0,1.0 - (depth*fixedVy/murky));
	vec3 colll = mix(refColor*tint,refColor,ColextFact);
	
	refrColor = mix(color, colll, extFact);
#else
	// linearly extinct the water. This is how quickly we see nothing but the pure water color
	float extFact = max(0.0,1.0 - (depth*fixedVy/20.0));
	// using both those factors, get our transparency.
	// This will be our base transparency on top.
	float base = 0.4 + depth*fixedVy/15.0; // TODO: murkiness.
	float alphaCoeff = mix(1.0, base, extFact);
	refrColor = color;
#endif
	
#if USE_REFLECTION
	// Reflections
	// we use real reflections against th skybox, and distort a texture of objects closer.
	vec3 eye = reflect(v,n);
	//eye.y = min(-0.2,eye.y);
	// let's calculate where we intersect with the skycube.
	Ray myRay = Ray(vec3(worldPos.x/4.0,worldPos.y,worldPos.z/4.0),eye);
	vec3 start = vec3(-1500.0 + mapSize/2.0,-100.0,-1500.0 + mapSize/2.0);
	vec3 end = vec3(1500.0 + mapSize/2.0,500.0,1500.0 + mapSize/2.0);
	float tmin = IntersectBox(myRay,start,end);
	vec4 newpos = vec4(-worldPos.x/4.0,worldPos.y,-worldPos.z/4.0,1.0) + vec4(eye * tmin,0.0) - vec4(-mapSize/2.0,worldPos.y,-mapSize/2.0,0.0);
	//newpos = normalize(newpos);
	newpos *= skyBoxRot;
	newpos.y *= 4.0;
	reflColor = textureCube(skyCube, newpos.rgb).rgb;
	
	// Reflections appear more distorted when viewed from a lower angle. Simulate this.
	//float angleEffect = clamp(1.6 - dot(vec3(0.0,1.0,0.0), v),0.0,1.0);

	// Distort the reflection coords based on waves.
	reflCoords = (0.5*reflectionCoords.xy - 15.0 * n.zx) / reflectionCoords.z + 0.5;
	vec4 refTex = texture2D(reflectionMap, reflCoords);
	
	// interpolate between the sky color and nearby objects.
	reflColor = mix(reflColor.rgb, refTex.rgb, refTex.a);
	// reflMod is used to reduce the intensity of sky reflections, which otherwise are too extreme.
	reflMod = max(refTex.a, 0.55);
	
#else
	// Temp fix for some ATI cards (see irc logs on th 1st of august betwee, fexor and wraitii)
	//reflCoords = clamp( (0.5*reflectionCoords.xy - waviness * mix(1.0, 20.0,waviness/10.0) * n.zx) / reflectionCoords.z + 0.5,0.0,1.0);	// Unbias texture coords
	//vec3 refTex = texture2D(reflectionMap, reflCoords).rgb;
	//reflColor = refTex.rgb;
	reflColor = vec3(0.15, 0.7, 0.82);
#endif
	
	losMod = texture2D(losMap, losCoords.st).a;
	losMod = losMod < 0.03 ? 0.0 : losMod;
	
	float wavesFresnel = 1.0;
	
#if USE_FANCY_EFFECTS
	//wavesFresnel = mix(1.0-fancyeffects.a,1.0,clamp(depth,0.0,1.0));
#endif
	
	vec3 color;
#if USE_SHADOWS_ON_WATER && USE_SHADOW
	float fresShadow = mix(fresnel, fresnel*shadow, 0.05 + murkiness*0.2);
	color = mix(refrColor, reflColor, fresShadow * wavesFresnel);
#else
	color = mix(refrColor, reflColor, fresnel * reflMod);
#endif
	
#if USE_SHADOWS_ON_WATER && USE_SHADOW
	color += shadow*specular;
#else
	color += specular;
#endif
	
#if USE_FANCY_EFFECTS
	vec4 FoamEffects = texture2D(waterEffectsTexOther, gl_FragCoord.xy/screenSize);
	
	vec3 foam1 = texture2D(normalMap, (normalCoords.st + normalCoords.zw * BigMovement*waviness/10.0) * (baseScale - waviness/wavyEffect)).aaa;
	vec3 foam2 = texture2D(normalMap2, (normalCoords.st + normalCoords.zw * BigMovement*waviness/10.0) * (baseScale - waviness/wavyEffect)).aaa;
	vec3 foam3 = texture2D(normalMap, normalCoords.st/6.0 - normalCoords.zw * 0.02).aaa;
	vec3 foam4 = texture2D(normalMap2, normalCoords.st/6.0 - normalCoords.zw * 0.02).aaa;
	vec3 foaminterp = mix(foam1, foam2, moddedTime);
	foaminterp *= mix(foam3, foam4, moddedTime);
	
	foam1.x = foaminterp.x * WindCosSin.x - foaminterp.z * WindCosSin.y;
	
	float foam = FoamEffects.r * FoamEffects.a*0.4 + pow(foam1.x*(5.0+waviness),(2.6 - waviness/5.5));
	
	gl_FragColor.rgb = get_fog(color) * losMod + foam * losMod;
#else
	gl_FragColor.rgb = get_fog(color) * losMod;
#endif
	
#if !USE_REFRACTION
	gl_FragColor.a = clamp(depth*2.0,0.0,1.0) * alphaCoeff;
#else
	gl_FragColor.a = clamp(depth*5.0,0.0,1.0);
#endif
	
#if USE_FANCY_EFFECTS
	if (fancyeffects.a < 0.05 && waterDepth < -1.0 )
		gl_FragColor.a = 0.0;
#endif
	
	//gl_FragColor = vec4(sunColor,1.0);
}
