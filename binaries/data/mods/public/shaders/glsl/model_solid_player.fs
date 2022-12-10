#version 110

#include "model_common.h"

#include "common/fog.h"
#include "common/fragment.h"

void main()
{
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(applyFog(playerColor.rgb, fogColor, fogParams), playerColor.a));
}
