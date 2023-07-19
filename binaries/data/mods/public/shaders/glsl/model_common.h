#include "common/shadows.h"
#include "common/stage.h"

#if USE_GPU_SKINNING
const int MAX_INFLUENCES = 4;
const int MAX_BONES = 64;
#endif

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, baseTex)
	TEXTURE_2D(1, aoTex)
	TEXTURE_2D(2, normTex)
	TEXTURE_2D(3, specTex)
#if !IGNORE_LOS
	TEXTURE_2D(4, losTex)
#endif
#if USE_SHADOW
	SHADOWS_TEXTURES(5)
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, instancingTransform)
#if USE_PLAYERCOLOR
	UNIFORM(vec4, playerColor)
#elif USE_OBJECTCOLOR
	UNIFORM(vec3, objectColor)
#endif
	UNIFORM(vec3, shadingColor)
END_DRAW_UNIFORMS

BEGIN_MATERIAL_UNIFORMS
#if USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX || USE_AO
	UNIFORM(vec4, effectSettings)
#endif
	UNIFORM(vec3, ambient)
	UNIFORM(vec3, sunColor)
	UNIFORM(vec3, sunDir)
	UNIFORM(mat4, transform)
	UNIFORM(vec3, cameraPos)
#if USE_WIND
	UNIFORM(float, sim_time)
	UNIFORM(vec4, windData)
#endif
	UNIFORM(vec3, fogColor)
	UNIFORM(vec2, fogParams)
	UNIFORM(vec2, losTransform)
#if USE_SHADOW
	SHADOWS_UNIFORMS
#endif
#if USE_GPU_SKINNING
	UNIFORM(mat4, skinBlendMatrices[MAX_BONES]);
#endif
END_MATERIAL_UNIFORMS

VERTEX_OUTPUT(0, vec4, v_lighting);
VERTEX_OUTPUT(1, vec2, v_tex);
#if (USE_INSTANCING || USE_GPU_SKINNING) && USE_AO
VERTEX_OUTPUT(2, vec2, v_tex2);
#endif
#if USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX
VERTEX_OUTPUT(3, vec4, v_normal);
#if (USE_INSTANCING || USE_GPU_SKINNING) && (USE_NORMAL_MAP || USE_PARALLAX)
VERTEX_OUTPUT(4, vec4, v_tangent);
//VERTEX_OUTPUT(5, vec3, v_bitangent);
#endif
#if USE_SPECULAR_MAP
VERTEX_OUTPUT(6, vec3, v_half);
#endif
#if (USE_INSTANCING || USE_GPU_SKINNING) && USE_PARALLAX
VERTEX_OUTPUT(7, vec3, v_eyeVec);
#endif
#endif
#if !IGNORE_LOS
VERTEX_OUTPUT(8, vec2, v_los);
#endif
#if USE_SHADOW
SHADOWS_VERTEX_OUTPUTS(9)
#endif
