#version 110

// This is a lightened version of water_high.fs that generates only the "n" vector (ie the normal) and the foam coverage.
// It's thus uncommented.

uniform float waviness;
uniform vec2 screenSize;
uniform float time;
varying vec3 worldPos;

uniform float mapSize;

uniform sampler2D normalMap;
uniform sampler2D normalMap2;

/*#if USE_FOAM || USE_WAVES
	uniform sampler2D Foam;
	uniform sampler2D waveTex;
#endif*/

void main()
{
	float wavyFactor = waviness * 0.125;

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
	vec3  n = normalize(ww1 - vec3(0.5, 0.5, 0.5));
	n = mix(vec3(0.0,1.0,0.0),n,waviness/10.0);

	float foamFact1 = texture2D(normalMap, (gl_TexCoord[0].st) * 0.3).a;
	float foamFact2 = texture2D(normalMap2, (gl_TexCoord[0].st) * 0.3).a;
	float foamFact = mix(foamFact1, foamFact2, mod(time * 60.0, 8.0) / 8.0);
	
	float foamUniversal1 = texture2D(normalMap, (gl_TexCoord[0].st) * 0.05).a;
	float foamUniversal2 = texture2D(normalMap2, (gl_TexCoord[0].st) * 0.05).a;
	float foamUniversal = mix(foamUniversal1, foamUniversal2, mod(time * 60.0, 8.0) / 8.0);
	
	gl_FragColor.rgba = vec4(n,foamFact*foamUniversal*30.0);
}
