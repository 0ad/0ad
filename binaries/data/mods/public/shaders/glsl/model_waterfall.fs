#version 120

#include "model_waterfall.h"

#include "common/debug_fragment.h"
#include "common/fragment.h"
#include "common/los_fragment.h"
#include "common/shadows_fragment.h"

void main()
{
	//vec4 texdiffuse = textureGrad(baseTex, vec3(fract(v_tex.xy), v_tex.z), dFdx(v_tex.xy), dFdy(v_tex.xy));
	vec4 texdiffuse = SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), fract(v_tex.xy));

	if (texdiffuse.a < 0.25)
		discard;

	texdiffuse.a *= v_transp;

	vec3 specular = sunColor * specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);

	vec3 color = (texdiffuse.rgb * v_lighting + specular) * getShadowOnLandscape();
	color += texdiffuse.rgb * ambient;

#if !IGNORE_LOS
	color *= getLOS(GET_DRAW_TEXTURE_2D(losTex), v_los);
#endif

#if !PASS_SHADOWS
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(applyDebugColor(color, 1.0, texdiffuse.a, 0.0), texdiffuse.a));
#else
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(0.0, 0.0, 0.0, 1.0));
#endif
}
