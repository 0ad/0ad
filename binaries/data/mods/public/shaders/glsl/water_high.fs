#version 110

#include "common/debug_fragment.h"
#include "common/fog.h"
#include "common/fragment.h"
#include "common/los_fragment.h"
#include "common/shadows_fragment.h"

// Environment settings
uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform mat4 skyBoxRot;

uniform float waviness;			// "Wildness" of the reflections and refractions; choose based on texture
uniform vec3 color;				// color of the water
uniform vec3 tint;				// Tint for refraction (used to simulate particles in water)
uniform float murkiness;		// Amount of tint to blend in with the refracted color

uniform float windAngle;
varying vec2 WindCosSin;

uniform vec2 screenSize;
varying float moddedTime;

varying vec3 worldPos;
varying float waterDepth;
varying vec2 waterInfo;

varying vec3 v_eyeVec;

varying vec4 normalCoords;
#if USE_REFLECTION
varying vec3 reflectionCoords;
#endif
#if USE_REFRACTION
varying vec3 refractionCoords;
#endif

varying float fwaviness;

uniform samplerCube skyCube;

uniform sampler2D normalMap;
uniform sampler2D normalMap2;

#if USE_FANCY_EFFECTS
	uniform sampler2D waterEffectsTex;
#endif

uniform vec4 waveParams1; // wavyEffect, BaseScale, Flattenism, Basebump
uniform vec4 waveParams2; // Smallintensity, Smallbase, Bigmovement, Smallmovement

#if USE_REFLECTION
	uniform sampler2D reflectionMap;
#endif

#if USE_REFRACTION
	uniform sampler2D refractionMap;
#if USE_REAL_DEPTH
	uniform sampler2D depthTex;
	uniform mat4 projInvTransform;
	uniform mat4 viewInvTransform;
#endif
#endif

vec3 getNormal(vec4 fancyeffects)
{
	float wavyEffect = waveParams1.r;
	float baseScale = waveParams1.g;
	float flattenism = waveParams1.b;
	float baseBump = waveParams1.a;
	float BigMovement = waveParams2.b;

	// This method uses 60 animated water frames. We're blending between each two frames
	// Scale the normal textures by waviness so that big waviness means bigger waves.
	vec3 ww1 = texture2D(normalMap, (normalCoords.st + normalCoords.zw * BigMovement * waviness / 10.0) * (baseScale - waviness / wavyEffect)).xzy;
	vec3 ww2 = texture2D(normalMap2, (normalCoords.st + normalCoords.zw * BigMovement * waviness / 10.0) * (baseScale - waviness / wavyEffect)).xzy;
	vec3 wwInterp = mix(ww1, ww2, moddedTime) - vec3(0.5, 0.0, 0.5);

	ww1.x = wwInterp.x * WindCosSin.x - wwInterp.z * WindCosSin.y;
	ww1.z = wwInterp.x * WindCosSin.y + wwInterp.z * WindCosSin.x;
	ww1.y = wwInterp.y;

	// Flatten them based on waviness.
	vec3 normal = normalize(mix(vec3(0.0, 1.0, 0.0), ww1, clamp(baseBump + fwaviness / flattenism, 0.0, 1.0)));

#if USE_FANCY_EFFECTS
	normal = mix(vec3(0.0, 1.0, 0.0), normal, 0.5 + waterInfo.r / 2.0);
	normal.xz = mix(normal.xz, fancyeffects.rb, fancyeffects.a / 2.0);
#else
	normal = mix(vec3(0.0, 1.0, 0.0), normal, 0.5 + waterInfo.r / 2.0);
#endif

	return vec3(-normal.x, normal.y, -normal.z);
}

vec3 getSpecular(vec3 normal, vec3 eyeVec)
{
	// Specular lighting vectors
	vec3 specularVector = reflect(sunDir, normal);
	// pow is undefined for null or negative values, except on intel it seems.
	float specularIntensity = pow(max(dot(specularVector, eyeVec), 0.0), 100.0);
	// Workaround to fix too flattened water.
	specularIntensity = smoothstep(0.6, 1.0, specularIntensity) * 1.2;
	return clamp(specularIntensity * sunColor, 0.0, 1.0);
}

vec4 getReflection(vec3 normal, vec3 eyeVec)
{
	// Reflections
	// 3 level of settings:
	// -If a player has refraction and reflection disabled, we return a gradient of blue based on the Y component.
	// -If a player has refraction OR reflection, we return a reflection of the actual skybox used.
	// -If a player has reflection enabled, we also return a reflection of actual entities where applicable.

	// reflMod reduces the intensity of reflections somewhat since they kind of wash refractions out otherwise.
	float reflMod = 0.75;
	vec3 eye = reflect(eyeVec, normal);

#if USE_REFLECTION
	float refVY = clamp(eyeVec.y * 2.0, 0.05, 1.0);

	// Distort the reflection coords based on waves.
	vec2 reflCoords = (0.5 * reflectionCoords.xy - 15.0 * normal.zx / refVY) / reflectionCoords.z + 0.5;
	vec4 refTex = texture2D(reflectionMap, reflCoords);

	vec3 reflColor = refTex.rgb;

	// Interpolate between the sky color and nearby objects.
	// Only do this when alpha is rather low, or transparent leaves show up as extremely white.
	if (refTex.a < 0.4)
		reflColor = mix(textureCube(skyCube, (vec4(eye, 0.0) * skyBoxRot).xyz).rgb, refTex.rgb, refTex.a);

	// Let actual objects be reflected fully.
	reflMod = max(refTex.a, 0.75);
#else
	vec3 reflColor = textureCube(skyCube, (vec4(eye, 0.0) * skyBoxRot).xyz).rgb;
#endif

	return vec4(reflColor, reflMod);
}

#if USE_REFRACTION && USE_REAL_DEPTH
vec3 getWorldPositionFromRefractionDepth(vec2 uv)
{
	float depth = texture2D(depthTex, uv).x;
	vec4 viewPosition = projInvTransform * (vec4((uv - vec2(0.5)) * 2.0, depth * 2.0 - 1.0, 1.0));
	viewPosition /= viewPosition.w;
	vec3 refrWorldPos = (viewInvTransform * viewPosition).xyz;
	// Depth buffer precision errors can give heights above the water.
	refrWorldPos.y = min(refrWorldPos.y, worldPos.y);
	return refrWorldPos;
}
#endif

vec4 getRefraction(vec3 normal, vec3 eyeVec, float depthLimit)
{
#if USE_REFRACTION && USE_REAL_DEPTH
	// Compute real depth at the target point.
	vec2 coords = (0.5 * refractionCoords.xy) / refractionCoords.z + 0.5;
	vec3 refrWorldPos = getWorldPositionFromRefractionDepth(coords);

	// Set depth to the depth at the undistorted point.
	float depth = distance(refrWorldPos, worldPos);
#else
	// fake depth computation: take the value at the vertex, add some if we are looking at a more oblique angle.
	float depth = waterDepth / (min(0.5, eyeVec.y) * 1.5 * min(0.5, eyeVec.y) * 2.0);
#endif

#if USE_REFRACTION
	// for refraction we want to distort more as depth goes down.
	// 1) compute a distortion based on depth at the pixel.
	// 2) Re-sample the depth at the target point
	// 3) Sample refraction texture

	// distoFactor controls the amount of distortion relative to wave normals.
	float distoFactor = 0.5 + clamp(depth / 2.0, 0.0, 7.0);

#if USE_REAL_DEPTH
	// Distort the texture coords under where the water is to simulate refraction.
	vec2 shiftedCoords = (0.5 * refractionCoords.xy - normal.xz * distoFactor) / refractionCoords.z + 0.5;
	vec3 refrWorldPos2 = getWorldPositionFromRefractionDepth(shiftedCoords);
	float newDepth = distance(refrWorldPos2, worldPos);

	// try to correct for fish. In general they'd look weirder without this fix.
	if (depth > newDepth + 3.0)
		distoFactor /= 2.0; // this in general will not fall on the fish but still look distorted.
	else
		depth = newDepth;
#endif

#if USE_FANCY_EFFECTS
	depth = max(depth, depthLimit);
	if (waterDepth < 0.0)
		depth = 0.0;
#endif

	// Distort the texture coords under where the water is to simulate refraction.
	vec2 refrCoords = (0.5 * refractionCoords.xy - normal.xz * distoFactor) / refractionCoords.z + 0.5;
	vec3 refColor = texture2D(refractionMap, refrCoords).rgb;

	// Note, the refraction map is cleared using (255, 0, 0), so pixels outside of the water plane are pure red.
	// If we get a pure red fragment, use an undistorted/less distorted coord instead.
	// blur the refraction map, distoring using normal so that it looks more random than it really is
	// and thus looks much better.
	float blur = (0.1 + clamp(normal.x, -0.1, 0.1)) / refractionCoords.z;

	vec4 blurColor = vec4(refColor, 1.0);

	vec4 tex = texture2D(refractionMap, refrCoords + vec2(blur + normal.x, blur + normal.z));
	blurColor += vec4(tex.rgb * tex.a, tex.a);
	tex = texture2D(refractionMap, refrCoords + vec2(-blur, blur + normal.z));
	blurColor += vec4(tex.rgb * tex.a, tex.a);
	tex = texture2D(refractionMap, refrCoords + vec2(-blur, -blur + normal.x));
	blurColor += vec4(tex.rgb * tex.a, tex.a);
	tex = texture2D(refractionMap, refrCoords + vec2(blur + normal.z, -blur));
	blurColor += vec4(tex.rgb * tex.a, tex.a);
	blurColor /= blurColor.a;
	float blurFactor = (distoFactor / 7.0);
	refColor = (refColor + blurColor.rgb * blurFactor) / (1.0 + blurFactor);

#else // !USE_REFRACTION

#if USE_FANCY_EFFECTS
	depth = max(depth, depthLimit);
#endif
	vec3 refColor = color;
#endif

	// for refraction, we want to adjust the value by v.y slightly otherwise it gets too different between "from above" and "from the sides".
	// And it looks weird (again, we are not used to seeing water from above).
	float fixedVy = max(eyeVec.y, 0.01);

	float murky = mix(200.0, 0.1, pow(murkiness, 0.25));

	// Apply water tint and murk color.
	float extFact = max(0.0, 1.0 - (depth * fixedVy / murky));
	float ColextFact = max(0.0, 1.0 - (depth * fixedVy / murky));
	vec3 colll = mix(refColor * tint, refColor, ColextFact);
	vec3 refrColor = mix(color, colll, extFact);

	float alpha = clamp(depth, 0.0, 1.0);

#if !USE_REFRACTION
	alpha = (1.4 - extFact) * alpha;
#endif
	return vec4(refrColor, alpha);
}

vec4 getFoam(vec4 fancyeffects, float shadow)
{
#if USE_FANCY_EFFECTS
	float wavyEffect = waveParams1.r;
	float baseScale = waveParams1.g;
	float BigMovement = waveParams2.b;
	vec3 foam1 = texture2D(normalMap, (normalCoords.st + normalCoords.zw * BigMovement * waviness / 10.0) * (baseScale - waviness / wavyEffect)).aaa;
	vec3 foam2 = texture2D(normalMap2, (normalCoords.st + normalCoords.zw * BigMovement * waviness / 10.0) * (baseScale - waviness / wavyEffect)).aaa;
	vec3 foam3 = texture2D(normalMap, normalCoords.st / 6.0 - normalCoords.zw * 0.02).aaa;
	vec3 foam4 = texture2D(normalMap2, normalCoords.st / 6.0 - normalCoords.zw * 0.02).aaa;
	vec3 foaminterp = mix(foam1, foam2, moddedTime);
	foaminterp *= mix(foam3, foam4, moddedTime);

	foam1.x = abs(foaminterp.x * WindCosSin.x) + abs(foaminterp.z * WindCosSin.y);

	float alpha = (fancyeffects.g + pow(foam1.x * (3.0 + waviness), 2.6 - waviness / 5.5)) * 2.0;
	return vec4(sunColor * shadow + ambient, clamp(alpha, 0.0, 1.0));
#else
	return vec4(0.0);
#endif
}

void main()
{
	float los = getLOS();
	// We don't need to render a water fragment if it's invisible.
	if (los < 0.001)
	{
		OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(0.0, 0.0, 0.0, 1.0));
		return;
	}

#if USE_FANCY_EFFECTS
	vec4 fancyeffects = texture2D(waterEffectsTex, gl_FragCoord.xy / screenSize);
#else
	vec4 fancyeffects = vec4(0.0);
#endif

	vec3 eyeVec = normalize(v_eyeVec);
	vec3 normal = getNormal(fancyeffects);

	vec4 refrColor = getRefraction(normal, eyeVec, fancyeffects.a);
	vec4 reflColor = getReflection(normal, eyeVec);

	// How perpendicular to the normal our view is. Used for fresnel.
	float ndotv = clamp(dot(normal, eyeVec), 0.0, 1.0);

	// Fresnel for "how much reflection vs how much refraction".
	float fresnel = clamp(((pow(1.1 - ndotv, 2.0)) * 1.5), 0.1, 0.75); // Approximation. I'm using 1.1 and not 1.0 because it causes artifacts, see #1714

	vec3 specular = getSpecular(normal, eyeVec);

#if USE_SHADOW
	float shadow = getShadowOnLandscape();
	fresnel = mix(fresnel, fresnel * shadow, 0.05 + murkiness * 0.2);
#else
	float shadow = 1.0;
#endif

	vec3 color = mix(refrColor.rgb, reflColor.rgb, fresnel * reflColor.a);
	color += shadow * specular;

	vec4 foam = getFoam(fancyeffects, shadow);
	color = clamp(mix(color, foam.rgb, foam.a), 0.0, 1.0);

	color = applyFog(color);

	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(applyDebugColor(color * los, 1.0, refrColor.a, 0.0), refrColor.a));
}
