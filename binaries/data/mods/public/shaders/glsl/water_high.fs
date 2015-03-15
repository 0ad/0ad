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
	float losMod;
	
	vec3 l = -sunDir;
	vec3 v = normalize(cameraPos - worldPos);
	vec3 h = normalize(l + v);
	
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

	n = vec3(-n.x,n.y,-n.z);
	
	// simulates how parallel the "point->sun", "view->point" vectors are.
	float ndoth = dot(n , h);
	
	// how perpendicular to the normal our view is. Used for fresnel.
	float ndotv = clamp(dot(n, v),0.0,1.0);
	
	// diffuse lighting-like. used for shadows?
	float ndotl = (dot(n, l) + 1.0)/2.0;
	
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
	
	// Fresnel for "how much reflection vs how much refraction".
	// Since we're not trying to simulate a realistic ocean 100%, aim for something that gives a little too much reflection
	// because we're not used to seeing the see from above.
	fresnel = clamp(pow(1.05 - ndotv, 1.1),0.0,0.8); // approximation. I'm using 1.05 and not 1.0 because it causes artifacts, see #1714
	// multiply by v.y so that in the distance refraction wins.
	// TODO: this is a hack because reflections don't work in the distance.
	fresnel = clamp(fresnel*1.5,0.0,0.9);
	fresnel *= min(1.0,log(1.0 + v.y*5.0));
	
	//gl_FragColor = vec4(fresnel,fresnel,fresnel,1.0);
	//return;
	
#if USE_SHADOWS_ON_WATER && USE_SHADOW
	float shadow = get_shadow(vec4(v_shadow.xy, v_shadow.zw));
#endif
	
	// for refraction, we want to adjust the value by v.y slightly otherwise it gets too different between "from above" and "from the sides".
	// And it looks weird (again, we are not used to seeing water from above).
	float fixedVy = max(v.y,0.1);
	
	float distoFactor = clamp(depth/2.0,0.0,7.0);
	
	float murky = mix(200.0,0.1,pow(murkiness,0.25));
	
#if USE_REFRACTION
	refrCoords = clamp( (0.5*refractionCoords.xy - n.xz * distoFactor*7.0) / refractionCoords.z + 0.5,0.0,1.0);	// Unbias texture coords
	vec3 refColor = texture2D(refractionMap, refrCoords).rgb;
	if (refColor.r > refColor.g + refColor.b + 0.25)
	{
		refrCoords = clamp( (0.5*refractionCoords.xy + n.xz) / refractionCoords.z + 0.5,0.0,1.0);	// Unbias texture coords
		refColor = texture2D(refractionMap, refrCoords).rgb;
	}
	
	// TODO: make murkiness (both types rematter on that.
	// linearly extinct the water. This is how quickly we see nothing but the pure water color
	float extFact = max(0.0,1.0 - (depth*fixedVy/murky));
	// This is how tinted the water is, ie how quickly the refracted floor takes the tint of the water
	float ColextFact = max(0.0,1.0 - (depth*fixedVy/murky));
	vec3 colll = mix(refColor*tint,refColor,ColextFact);
	
#if USE_SHADOWS_ON_WATER && USE_SHADOW
	// TODO:
	refrColor = mix(color, colll, extFact);
#else
	refrColor = mix(color, colll, extFact);
#endif
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
	//float disttt = distance(worldPos,cameraPos);
	//tex = mix(tex,vec3(0.7,0.7,0.9),clamp(disttt/300.0*disttt/300.0*disttt/300.0,0.0,0.9));
	//gl_FragColor = vec4(clamp(disttt/300.0*disttt/300.0,0.0,1.0),clamp(disttt/300.0*disttt/300.0,0.0,1.0),clamp(disttt/300.0*disttt/300.0,0.0,1.0),1.0);
	//return;
	
	reflCoords = clamp( (0.5*reflectionCoords.xy - 40.0 * n.zx) / reflectionCoords.z + 0.5,0.0,1.0);	// Unbias texture coords
	vec4 refTex = texture2D(reflectionMap, reflCoords);
	fresnel = clamp(fresnel+refTex.a/3.0,0.0,1.0);
	reflColor = refTex.rgb * refTex.a + reflColor*(1.0-refTex.a);
	
#else
	// Temp fix for some ATI cards (see irc logs on th 1st of august betwee, fexor and wraitii)
	//reflCoords = clamp( (0.5*reflectionCoords.xy - waviness * mix(1.0, 20.0,waviness/10.0) * n.zx) / reflectionCoords.z + 0.5,0.0,1.0);	// Unbias texture coords
	//vec3 refTex = texture2D(reflectionMap, reflCoords).rgb;
	//reflColor = refTex.rgb;
	reflColor = vec3(0.15, 0.7, 0.82);
#endif
	
	// TODO: At very low angles the reflection stuff doesn't really work any more:
	// IRL you would get a blur of the sky, but we don't have that precision (would require mad oversampling)
	// So tend towards a predefined color (per-map) which looks like what the skybox would look like if you really blurred it.
	// The TODO here would be to precompute a band (1x32?) that represents the average color around the map.
	// TODO: another issue is that at high distances (half map) the texture blurs into flatness. Using better mipmaps won't really solve it
	// So we'll need to stop showing reflections and default to sky color there too.
	// Unless maybe waviness is so low that you would see like in a mirror anyways.
	//float disttt = distance(worldPos,cameraPos);
	//reflColor = mix(vec3(0.5,0.5,0.55), reflColor, clamp(1.0-disttt/600.0*disttt/600.0,0.0,1.0));//clamp(-0.05 + v.y*20.0,0.0,1.0));
	
	// Specular.
	specular = pow(ndoth, mix(5.0,2000.0, clamp(v.y*v.y*2.0,0.0,1.0)))*sunColor * 1.5;// * sunColor * 1.5 * ww.r;
	
	losMod = texture2D(losMap, losCoords.st).a;
	losMod = losMod < 0.03 ? 0.0 : losMod;
	
	float wavesFresnel = 1.0;
	
#if USE_FANCY_EFFECTS
	wavesFresnel = mix(1.0-fancyeffects.a,1.0,clamp(depth,0.0,1.0));
#endif
	
	vec3 color;
#if USE_SHADOWS_ON_WATER && USE_SHADOW
	float fresShadow = mix(fresnel, fresnel*shadow, 0.05 + murkiness*0.2);
	color = mix(refrColor, reflColor, fresShadow * wavesFresnel);
#else
	color = mix(refrColor, reflColor, fresnel * wavesFresnel);
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
	//foam1.z = foaminterp.x * WindCosSin.y + foaminterp.z * WindCosSin.x;
	//foam1.y = foaminterp.y;
	float foam = FoamEffects.r * FoamEffects.a*0.4 + pow(foam1.x*(5.0+waviness),(2.6 - waviness/5.5));
	foam *= ndotl;
	
	gl_FragColor.rgb = get_fog(color) * losMod + foam * losMod;// + fancyeffects.a * losMod;
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
