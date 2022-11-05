#version 120

#include "common/fragment.h"

uniform vec4 color;

void main()
{
	OUTPUT_FRAGMENT_SINGLE_COLOR(color);
}
