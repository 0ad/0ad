#version 110

#include "common/fragment.h"

#if DEBUG_TEXTURED
uniform sampler2D baseTex;

varying vec2 v_tex;
#else
uniform vec4 color;
#endif

void main()
{
#if DEBUG_TEXTURED
	OUTPUT_FRAGMENT_SINGLE_COLOR(texture2D(baseTex, v_tex));
#else
	OUTPUT_FRAGMENT_SINGLE_COLOR(color);
#endif
}
