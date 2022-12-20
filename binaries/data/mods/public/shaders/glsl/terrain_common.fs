#version 120

#include "terrain_common.h"

#include "common/debug_fragment.h"
#include "common/fog.h"
#include "common/fragment.h"
#include "common/los_fragment.h"
#include "common/shadows_fragment.h"

#if USE_TRIPLANAR
vec4 triplanar(sampler2D tex, vec3 wpos)
{
	float tighten = 0.4679;

	vec3 blending = abs(normalize(v_normal)) - tighten;
	blending = max(blending, 0.0);

	blending /= vec3(blending.x + blending.y + blending.z);

	vec3 signedBlending = sign(v_normal) * blending;

	vec3 coords = wpos;
	coords.xyz /= 32.0;    // Ugh.

	vec4 col1 = SAMPLE_2D(tex, coords.yz);
	vec4 col2 = SAMPLE_2D(tex, coords.zx);
	vec4 col3 = SAMPLE_2D(tex, coords.yx);

	vec4 colBlended = col1 * blending.x + col2 * blending.y + col3 * blending.z;

	return colBlended;
}

vec4 triplanarNormals(sampler2D tex, vec3 wpos)
{
	float tighten = 0.4679;

	vec3 blending = abs(normalize(v_normal)) - tighten;
	blending = max(blending, 0.0);

	blending /= vec3(blending.x + blending.y + blending.z);

	vec3 signedBlending = sign(v_normal) * blending;

	vec3 coords = wpos;
	coords.xyz /= 32.0;    // Ugh.

	vec4 col1 = SAMPLE_2D(tex, coords.yz).xyzw;
	col1.y = 1.0 - col1.y;
	vec4 col2 = SAMPLE_2D(tex, coords.zx).yxzw;
	col2.y = 1.0 - col2.y;
	vec4 col3 = SAMPLE_2D(tex, coords.yx).yxzw;
	col3.y = 1.0 - col3.y;

	vec4 colBlended = col1 * blending.x + col2 * blending.y + col3 * blending.z;

	return colBlended;
}
#endif

void main()
{
  float alpha = 0.0;

  #if BLEND
    // Use alpha from blend texture
    alpha = 1.0 - SAMPLE_2D(GET_DRAW_TEXTURE_2D(blendTex), v_blend).a;
  #else
    alpha = 1.0;
  #endif

  #if USE_TRIPLANAR
    vec4 tex = triplanar(GET_DRAW_TEXTURE_2D(baseTex), v_tex);
  #else
    vec4 tex = SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex.xy);
  #endif

  #if DECAL
    // Use alpha from main texture
    alpha = tex.a;
  #endif

  vec3 texdiffuse = tex.rgb;

  #if USE_SPECULAR_MAP || USE_NORMAL_MAP
    vec3 normal = v_normal;
  #endif

  #if USE_NORMAL_MAP
    float sign = v_tangent.w;
    mat3 tbn = mat3(v_tangent.xyz, v_bitangent * -sign, v_normal);
    #if USE_TRIPLANAR
      vec3 ntex = triplanarNormals(GET_DRAW_TEXTURE_2D(normTex), v_tex).rgb * 2.0 - 1.0;
    #else
      vec3 ntex = SAMPLE_2D(GET_DRAW_TEXTURE_2D(normTex), v_tex).rgb * 2.0 - 1.0;
    #endif
    normal = normalize(tbn * ntex);
    vec3 bumplight = max(dot(-sunDir, normal), 0.0) * sunColor;
    vec3 sundiffuse = (bumplight - v_lighting.rgb) * effectSettings.x + v_lighting.rgb;
  #else
    vec3 sundiffuse = v_lighting;
  #endif

  vec4 specular = vec4(0.0);
  #if USE_SPECULAR_MAP
    vec3 specCol;
    float specPow;
    #if USE_TRIPLANAR
      vec4 s = triplanar(GET_DRAW_TEXTURE_2D(specTex), v_tex);
    #else
      vec4 s = SAMPLE_2D(GET_DRAW_TEXTURE_2D(specTex), v_tex);
    #endif
    specCol = s.rgb;
    specular.a = s.a;
    specPow = effectSettings.y;
    specular.rgb = sunColor * specCol * pow(max(0.0, dot(normalize(normal), v_half)), specPow);
  #endif

  vec3 color = (texdiffuse * sundiffuse + specular.rgb) * getShadowOnLandscape() + texdiffuse * ambient;

  #if USE_SPECULAR_MAP && USE_SELF_LIGHT
    color = mix(texdiffuse, color, specular.a);
  #endif

  color = applyFog(color, fogColor, fogParams);

#if !IGNORE_LOS
  color *= getLOS(GET_DRAW_TEXTURE_2D(losTex), v_los);
#endif

  #if DECAL
    color *= shadingColor;
  #endif

  OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(applyDebugColor(color, 1.0, 1.0, 0.0), alpha));
}
