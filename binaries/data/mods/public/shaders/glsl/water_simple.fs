#version 110

#include "water_simple.h"

#include "common/fog.h"
#include "common/fragment.h"
#include "common/los_fragment.h"

void main()
{
	vec3 waterColor = SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_coords).rgb;
	waterColor *= color;
	waterColor = applyFog(waterColor, fogColor, fogParams);
#if !IGNORE_LOS
	waterColor *= getLOS(GET_DRAW_TEXTURE_2D(losTex), v_los);
#endif
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(waterColor, 1.0));
}
