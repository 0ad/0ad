#version 120

#include "model_common.h"

#include "common/debug_fragment.h"
#include "common/fog.h"
#include "common/fragment.h"
#include "common/los_fragment.h"
#include "common/shading.h"
#include "common/shadows_fragment.h"

void main()
{
  vec2 coord = v_tex;

  #if (USE_INSTANCING || USE_GPU_SKINNING) && (USE_PARALLAX || USE_NORMAL_MAP)
    vec3 bitangent = vec3(v_normal.w, v_tangent.w, v_lighting.w);
    mat3 tbn = mat3(v_tangent.xyz, bitangent, v_normal.xyz);
  #endif

  #if (USE_INSTANCING || USE_GPU_SKINNING) && USE_PARALLAX
  {
    float h = SAMPLE_2D(GET_DRAW_TEXTURE_2D(normTex), coord).a;

    vec3 eyeDir = normalize(v_eyeVec * tbn);
    float dist = length(v_eyeVec);

    vec2 move;
    float height = 1.0;
    float scale = effectSettings.z;

    int iter = int(min(20.0, 25.0 - dist/10.0));

	if (iter > 0)
	{
		float s = 1.0/float(iter);
		float t = s;
		move = vec2(-eyeDir.x, eyeDir.y) * scale / (eyeDir.z * float(iter));
		vec2 nil = vec2(0.0);

		for (int i = 0; i < iter; ++i) {
		  height -= t;
		  t = (h < height) ? s : 0.0;
		  vec2 temp = (h < height) ? move : nil;
		  coord += temp;
		  h = SAMPLE_2D(GET_DRAW_TEXTURE_2D(normTex), coord).a;
		}

		// Move back to where we collided with the surface.
		// This assumes the surface is linear between the sample point before we
		// intersect the surface and after we intersect the surface
		float hp = SAMPLE_2D(GET_DRAW_TEXTURE_2D(normTex), coord - move).a;
		coord -= move * ((h - height) / (s + h - hp));
	}
  }
  #endif

  vec4 tex = SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), coord);

  // Alpha-test as early as possible
  #ifdef REQUIRE_ALPHA_GEQUAL
    if (tex.a < REQUIRE_ALPHA_GEQUAL)
      discard;
  #endif

  #if USE_TRANSPARENT
    float alpha = tex.a;
  #else
    float alpha = 1.0;
  #endif

  vec3 texdiffuse = tex.rgb;

  // Apply-coloring based on texture alpha
  #if USE_PLAYERCOLOR
    texdiffuse *= mix(playerColor.rgb, vec3(1.0, 1.0, 1.0), tex.a);
  #elif USE_OBJECTCOLOR
    texdiffuse *= mix(objectColor, vec3(1.0, 1.0, 1.0), tex.a);
  #endif

  #if USE_SPECULAR_MAP || USE_NORMAL_MAP
    vec3 normal = normalize(v_normal.xyz);
  #endif

  #if (USE_INSTANCING || USE_GPU_SKINNING) && USE_NORMAL_MAP
    normal = calculateNormal(normal, SAMPLE_2D(GET_DRAW_TEXTURE_2D(normTex), coord).rgb, tbn, effectSettings.x);
    vec3 sundiffuse = max(dot(-sunDir, normal), 0.0) * sunColor;
  #else
    vec3 sundiffuse = v_lighting.rgb;
  #endif

  vec3 specular = vec3(0.0);
  float emissionWeight = 0.0;
  #if USE_SPECULAR_MAP
    vec4 s = SAMPLE_2D(GET_DRAW_TEXTURE_2D(specTex), coord);
    vec3 specCol = s.rgb;
    float specPow = effectSettings.y;
    specular = calculateSpecular(normal, normalize(v_half), sunColor, specCol, specPow);
    emissionWeight = s.a;
  #endif

  #if (USE_INSTANCING || USE_GPU_SKINNING) && USE_AO
    float ao = SAMPLE_2D(GET_DRAW_TEXTURE_2D(aoTex), v_tex2).r;
  #else
    float ao = 1.0;
  #endif

  vec3 color = calculateShading(texdiffuse, sundiffuse, specular, ambient, getShadow(), ao);

  #if USE_SPECULAR_MAP && USE_SELF_LIGHT
    color = mix(texdiffuse, color, emissionWeight);
  #endif

  color = applyFog(color, fogColor, fogParams);

#if !IGNORE_LOS
  color *= getLOS(GET_DRAW_TEXTURE_2D(losTex), v_los);
#endif

  color *= shadingColor;

  color = applyDebugColor(color, ao, alpha, 0.0);

  OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(color, alpha));
}
