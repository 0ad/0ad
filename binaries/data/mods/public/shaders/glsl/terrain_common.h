#include "common/shadows.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, baseTex)
	TEXTURE_2D(1, blendTex)
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
	UNIFORM(vec3, shadingColor)
	UNIFORM(vec2, textureTransform)
#if USE_NORMAL_MAP || USE_SPECULAR_MAP
	UNIFORM(vec4, effectSettings)
#endif
END_DRAW_UNIFORMS

BEGIN_MATERIAL_UNIFORMS
	UNIFORM(vec3, ambient)
	UNIFORM(mat4, transform)
	UNIFORM(vec3, cameraPos)
	UNIFORM(vec3, sunDir)
	UNIFORM(vec3, sunColor)
	UNIFORM(vec3, fogColor)
	UNIFORM(vec2, fogParams)
	UNIFORM(vec2, losTransform)
#if USE_SHADOW
	SHADOWS_UNIFORMS
#endif
END_MATERIAL_UNIFORMS

#if USE_SPECULAR_MAP || USE_NORMAL_MAP || USE_TRIPLANAR
VERTEX_OUTPUT(0, vec3, v_normal);
#endif
VERTEX_OUTPUT(1, vec3, v_lighting);
#if BLEND
VERTEX_OUTPUT(2, vec2, v_blend);
#endif
#if USE_TRIPLANAR
VERTEX_OUTPUT(3, vec3, v_tex);
#else
VERTEX_OUTPUT(3, vec2, v_tex);
#endif
#if USE_NORMAL_MAP
VERTEX_OUTPUT(4, vec4, v_tangent);
VERTEX_OUTPUT(5, vec3, v_bitangent);
#endif
#if USE_SPECULAR_MAP
VERTEX_OUTPUT(6, vec3, v_half);
#endif
#if !IGNORE_LOS
VERTEX_OUTPUT(7, vec2, v_los);
#endif
#if USE_SHADOW
SHADOWS_VERTEX_OUTPUTS(8)
#endif
