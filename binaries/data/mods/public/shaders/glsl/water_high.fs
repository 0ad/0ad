#version 110

uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 cameraPos;
uniform sampler2D losMap;
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

varying vec3 worldPos;
varying float waterDepth;
varying vec4 waterInfo;

uniform float mapSize;

uniform samplerCube skyCube;

uniform sampler2D normalMap;
uniform sampler2D normalMap2;

#if USE_FANCY_EFFECTS
	uniform sampler2D waterEffectsTex;
#endif

#if USE_REFLECTION
	uniform sampler2D reflectionMap;
#endif
#if USE_REFRACTION
	uniform sampler2D refractionMap;
#endif
#if USE_REAL_DEPTH
	uniform sampler2D depthTex;
#endif

/*#if USE_FOAM || USE_WAVES
	uniform sampler2D Foam;
	uniform sampler2D waveTex;
#endif*/

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
// This is sorta useless since it's hardcoded for 45°. Change it to sun orientation too.
mat3 rotationMatrix()
{
    vec3 axis = vec3(0.0,1.0,0.0);
    float s = sin(1.12);
    float c = cos(1.12);
    
    return mat3(c, 0.0, s,
                0.0,  1.0, 0.0,
                -s, 0.0, c);
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
	//float fwaviness = 1;
	
	float fresnel;
	float t;				// Temporary variable
	vec2 reflCoords, refrCoords;
	vec3 reflColor, refrColor, specular;
	float losMod;
	
	// Correct the waviness (range [0,10]) to something slightly more convenient (range [0,1.2] so you can treat it like [0.1] with overdraft).
	float wavyFactor = waviness * 0.125;

	vec3 l = -sunDir;
	vec3 v = normalize(cameraPos - worldPos);
	vec3 h = normalize(l + v);
	
	// Calculate water normals.
	
#if USE_FANCY_EFFECTS
	vec4 fancyeffects = texture2D(waterEffectsTex, gl_FragCoord.xy/screenSize);
	vec3 n = fancyeffects.rgb;
#else
	// This method uses 60 animated water frames. We're blending between each two frames
	// TODO: could probably have fewer frames thanks to this blending.
	// Scale the normal textures by waviness so that big waviness means bigger waves.
	vec3 ww1 = texture2D(normalMap, (gl_TexCoord[0].st) * (0.7 - waviness/20.0)).xzy;
	vec3 ww2 = texture2D(normalMap2, (gl_TexCoord[0].st) * (0.7 - waviness/20.0)).xzy;
	
	//vec3 smallWW = texture2D(normalMap, (gl_TexCoord[0].st - gl_TexCoord[0].wz*6.0) * 1.2).xzy;
	//vec3 smallWW2 = texture2D(normalMap2, (gl_TexCoord[0].st - gl_TexCoord[0].wz*6.0) * 1.2).xzy;

	ww1 = mix(ww1, ww2, mod(time * 60.0, 8.0) / 8.0);
	//smallWW = mix(smallWW, smallWW2, mod(time * 60.0, 8.0) / 8.0) - vec3(0.5);
	//ww += vec3(smallWW.x,0.0,smallWW.z)*0.5;
	
	/*#if USE_WAVES
		vec3 waves = texture2D(waveTex, gl_FragCoord.xy/screenSize).rbg - vec3(0.5,0.5,0.5);
		float waveFoam = 0.0;//texture2D(waveTex, gl_FragCoord.xy/screenSize).a;
		n = normalize(mix(waves, ww - vec3(0.5, 0.5, 0.5) , clamp(distToShore*3.0,0.4,1.0)));
	#else*/
	// Fix our normals.
	vec3  n = normalize(ww1 - vec3(0.5, 0.5, 0.5));

	// Flatten them based on waviness.
	n = mix(vec3(0.0,1.0,0.0),n,waviness/10.0);

#endif

	// simulates how parallel the "point->sun", "view->point" vectors are.
	// To always have a bit of that, we don't use the "n" we calculated above.
	float ndoth = dot( mix(vec3(0.0,1.0,0.0),n,0.1 + min(0.8,waviness*waviness/70.0)) , h);
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
  	
	// Fresnel for "how much reflection vs how much refraction".
	// Since we're not trying to simulate a realistic ocean 100%, aim for something that gives a little too much reflection
	// because we're not used to seeing the see from above.
	fresnel = clamp(pow(1.05 - ndotv, 1.3),0.0,0.8); // approximation. I'm using 1.05 and not 1.0 because it causes artifacts, see #1714
	// multiply by v.y so that in the distance refraction wins.
	// TODO: this is a hack because reflections don't work in the distance.
	fresnel *= min(1.0,log(1.0 + v.y*5.0));
	fresnel = 0.2 + fresnel * 0.8;
	
	//gl_FragColor = vec4(fresnel,fresnel,fresnel,1.0);
	//return;
	
	/*#if USE_FOAM
		// texture is rotated 90°, moves slowly.
		vec2 foam1RC = vec2(-gl_TexCoord[0].t,gl_TexCoord[0].s)*1.3  - 0.012*n.xz + vec2(time*0.004,time*0.003);
		// texture is not rotated, moves twice faster in the opposite direction, translated.
		vec2 foam2RC = gl_TexCoord[0].st*1.8 + vec2(time*-0.019,time*-0.012) - 0.012*n.xz + vec2(0.4,0.2);
		
		vec2 WaveRocking = cos(time*1.2566) * beachOrientation * clamp(1.0 - distToShore*0.8,0.1,1.0)/3.0;
		vec4 foam1 = texture2D(Foam, foam1RC + vec2(-WaveRocking.t,WaveRocking.s));
		vec4 foam2 = foam1.r*texture2D(Foam, foam2RC + WaveRocking);
		
		vec3 finalFoam = min((foam2).rrr * waterInfo.a,1.0);
		
		if ((1.0 - finalFoam.r) >= wavyFactor)
			finalFoam = vec3(0.0);
		#if USE_WAVES && USE_NORMALS
			// waves bypass the regular foam restrictions.
			finalFoam += min( max(0.0,-waves.b) * texture2D(Foam, foam1RC).r, 1.0)*3.0 * max(0.0,wavyFactor-0.1);
		#endif
		finalFoam *= sunColor;
	#endif*/
	
	#if USE_SHADOWS_ON_WATER && USE_SHADOW
		float shadow = get_shadow(vec4(v_shadow.xy, v_shadow.zw));
	#endif
	
	// for refraction, we want to adjust the value by v.y slightly otherwise it gets too different between "from above" and "from the sides".
	// And it looks weird (again, we are not used to seeing water from above).
	float fixedVy = clamp(v.y,0.1,1.0);

	float distoFactor = clamp(depth/2.0,0.0,7.0);
	
	float murky = mix(200.0,0.1,pow(murkiness,0.25));

	#if USE_REFRACTION
		refrCoords = clamp( (0.5*gl_TexCoord[2].xy - n.xz * distoFactor*10.0) / gl_TexCoord[2].w + 0.5,0.0,1.0);	// Unbias texture coords
		vec3 refColor = texture2D(refractionMap, refrCoords).rgb;
		
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
		reflCoords = clamp( (0.5*gl_TexCoord[1].xy - waviness * 2.0 * n.xz) / gl_TexCoord[1].w + 0.5,0.0,1.0);	// Unbias texture coords
		reflColor = mix(texture2D(reflectionMap, reflCoords).rgb, sunColor * reflectionTint, reflectionTintStrength);
		// TODO: At very low angles the reflection stuff doesn't really work any more:
		// IRL you would get a blur of the sky, but we don't have that precision (would require mad oversampling)
		// So tend towards a predefined color (per-map) which looks like what the skybox would look like if you really blurred it.
		// The TODO here would be to precompute a band (1x32?) that represents the average color around the map.
		// TODO: another issue is that at high distances (half map) the texture blurs into flatness. Using better mipmaps won't really solve it
		// So we'll need to stop showing reflections and default to sky color there too.
		// Unless maybe waviness is so low that you would see like in a mirror anyways.
		//float disttt = distance(worldPos,cameraPos);
		//reflColor = mix(vec3(0.5,0.5,0.55), reflColor, clamp(1.0-disttt/600.0*disttt/600.0,0.0,1.0));//clamp(-0.05 + v.y*20.0,0.0,1.0));
	#else
		vec3 eye = reflect(v,n);
		eye.y = min(-0.1,eye.y);
		// let's calculate where we intersect with the skycube.
		Ray myRay = Ray(vec3(worldPos.x/4.0,worldPos.y,worldPos.z/4.0),eye);
		vec3 start = vec3(-1500.0 + mapSize/2.0,-100.0,-1500.0 + mapSize/2.0);
		vec3 end = vec3(1500.0 + mapSize/2.0,500.0,1500.0 + mapSize/2.0);
		float tmin = IntersectBox(myRay,start,end);
		vec3 newpos = vec3(-worldPos.x/4.0,worldPos.y,-worldPos.z/4.0) + eye * tmin - vec3(-mapSize/2.0,worldPos.y,-mapSize/2.0);
		//newpos = normalize(newpos);
		newpos.y *= 6.0;
		newpos *= rotationMatrix();
		vec3 tex = textureCube(skyCube, newpos).rgb;
		//float disttt = distance(worldPos,cameraPos);
		//tex = mix(tex,vec3(0.7,0.7,0.9),clamp(disttt/300.0*disttt/300.0*disttt/300.0,0.0,0.9));
		//gl_FragColor = vec4(clamp(disttt/300.0*disttt/300.0,0.0,1.0),clamp(disttt/300.0*disttt/300.0,0.0,1.0),clamp(disttt/300.0*disttt/300.0,0.0,1.0),1.0);
		//return;
		reflColor = mix(tex, sunColor * reflectionTint, reflectionTintStrength);
	#endif
	
	// Specular.
	specular = pow(ndoth, mix(100.0,450.0, v.y*2.0))*sunColor * 1.5;// * sunColor * 1.5 * ww.r;

	losMod = texture2D(losMap, gl_TexCoord[3].st).a;
	losMod = losMod < 0.03 ? 0.0 : losMod;
	
	vec3 colour;
	#if USE_SHADOWS_ON_WATER && USE_SHADOW
		float fresShadow = mix(fresnel, fresnel*shadow, 0.05 + murkiness*0.2);
	/*	#if USE_FOAM
			colour = mix(refrColor, reflColor, fresShadow) + max(ndotl,0.4)*(finalFoam)*(shadow/2.0 + 0.5);
		#else*/
		colour = mix(refrColor, reflColor, fresShadow);
	#else
		/*#if USE_FOAM
			colour = mix(refrColor, reflColor, fresnel) + max(ndotl,0.4)*(finalFoam);
		#else*/
		colour = mix(refrColor, reflColor, fresnel);
	#endif
	
	#if USE_SHADOWS_ON_WATER && USE_SHADOW
		colour += shadow*specular;
	#else
		colour += specular;
	#endif
	
	// TODO: work the foam in somewhere else.
	#if USE_FANCY_EFFECTS
		gl_FragColor.rgb = get_fog(colour) * losMod + fancyeffects.a * losMod;
	#else
		gl_FragColor.rgb = get_fog(colour) * losMod;
	#endif
	
	#if !USE_REFRACTION
		gl_FragColor.a = clamp(depth*2.0,0.0,1.0) * alphaCoeff;
	#else
		gl_FragColor.a = clamp(depth*2.0,0.0,1.0);
	#endif
}
