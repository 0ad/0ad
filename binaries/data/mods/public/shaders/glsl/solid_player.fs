#version 110

#include "common/fog.h"

uniform vec4 playerColor;

void main()
{
	gl_FragColor = vec4(applyFog(playerColor.rgb), playerColor.a);
}
