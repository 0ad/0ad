#version 110

#include "particle.h"

#include "common/fog.h"
#include "common/fragment.h"
#include "common/los_fragment.h"

void main()
{
	vec4 color = SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex) * vec4((v_color.rgb + sunColor) / 2.0, v_color.a);

	color.rgb = applyFog(color.rgb, fogColor, fogParams);
#if !IGNORE_LOS
	color.rgb *= getLOS(GET_DRAW_TEXTURE_2D(losTex), v_los);
#endif

	OUTPUT_FRAGMENT_SINGLE_COLOR(color);
}
