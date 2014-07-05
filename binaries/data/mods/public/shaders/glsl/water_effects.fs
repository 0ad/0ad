#version 110

// This is a lightened version of water_high.fs that generates only the "n" vector (ie the normal) and the foam coverage.
// It's thus uncommented.

uniform float waviness;
uniform vec2 screenSize;
uniform float time;

varying vec3 worldPos;
varying vec4 waterInfo;

uniform float mapSize;

uniform sampler2D normalMap;
uniform sampler2D normalMap2;

/*#if USE_FOAM || USE_WAVES
	uniform sampler2D Foam;
	uniform sampler2D waveTex;
#endif*/

uniform vec4 waveParams1; // wavyEffect, BaseScale, Flattenism, Basebump
uniform vec4 waveParams2; // Smallintensity, Smallbase, Bigmovement, Smallmovement

void main()
{
	// Fix the waviness for local wind strength
	float fwaviness = waviness * ((0.15+waterInfo.r/1.15));

	float wavyEffect = waveParams1.r;
	float baseScale = waveParams1.g;
	float flattenism = waveParams1.b;
	float baseBump = waveParams1.a;
	
	float smallIntensity = waveParams2.r;
	float smallBase = waveParams2.g;
	float BigMovement = waveParams2.b;
	float SmallMovement = waveParams2.a;
	
	// This method uses 60 animated water frames. We're blending between each two frames
	// TODO: could probably have fewer frames thanks to this blending.
	// Scale the normal textures by waviness so that big waviness means bigger waves.
	vec3 ww1 = texture2D(normalMap, (gl_TexCoord[0].st + gl_TexCoord[0].zw * BigMovement*waviness/10.0) * (baseScale - waviness/wavyEffect)).xzy;
	vec3 ww2 = texture2D(normalMap2, (gl_TexCoord[0].st + gl_TexCoord[0].zw * BigMovement*waviness/10.0) * (baseScale - waviness/wavyEffect)).xzy;
	
	vec3 smallWW = texture2D(normalMap, (gl_TexCoord[0].st + gl_TexCoord[0].zw * SmallMovement*waviness/10.0) * baseScale*3.0).xzy;
	vec3 smallWW2 = texture2D(normalMap2, (gl_TexCoord[0].st + gl_TexCoord[0].zw * SmallMovement*waviness/10.0) * baseScale*3.0).xzy;
	
	ww1 = mix(ww1, ww2, mod(time * 60.0, 8.0) / 8.0);
	smallWW = mix(smallWW, smallWW2, mod(time * 60.0, 8.0) / 8.0) - vec3(0.5);
	ww1 += vec3(smallWW.x,0.0,smallWW.z)*(fwaviness/10.0*smallIntensity + smallBase);
	
	ww1 = mix(smallWW + vec3(0.5,0.0,0.5), ww1, waterInfo.r);
	
	// Flatten them based on waviness.
	vec3 n = normalize(mix(vec3(0.0,1.0,0.0),ww1 - vec3(0.5,0.0,0.5), clamp(baseBump + fwaviness/flattenism,0.0,1.0)));

	float foamFact1 = texture2D(normalMap, (gl_TexCoord[0].st) * 0.3).a;
	float foamFact2 = texture2D(normalMap2, (gl_TexCoord[0].st) * 0.3).a;
	float foamFact = mix(foamFact1, foamFact2, mod(time * 60.0, 8.0) / 8.0);
	
	float foamUniversal1 = texture2D(normalMap, (gl_TexCoord[0].st) * 0.05).a;
	float foamUniversal2 = texture2D(normalMap2, (gl_TexCoord[0].st) * 0.05).a;
	float foamUniversal = mix(foamUniversal1, foamUniversal2, mod(time * 60.0, 8.0) / 8.0);
	
	gl_FragColor.rgba = vec4((n + 1.0)/2.0,foamFact*foamUniversal*30.0);
}
