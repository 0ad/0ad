#ifndef INCLUDED_COMMON_LOS_FRAGMENT
#define INCLUDED_COMMON_LOS_FRAGMENT

#include "common/texture.h"

#if !IGNORE_LOS
float getLOS(sampler2D losTex, vec2 uv)
{
	float los = SAMPLE_2D(losTex, uv).r;
	float threshold = 0.03;
	return clamp(los - threshold, 0.0, 1.0) / (1.0 - threshold);

	return 1.0;
}
#endif

#endif // INCLUDED_COMMON_LOS_FRAGMENT
