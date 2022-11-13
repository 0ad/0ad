#version 110

#include "common/fog.h"
#include "common/fragment.h"

uniform vec4 playerColor;

void main()
{
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(applyFog(playerColor.rgb), playerColor.a));
}
