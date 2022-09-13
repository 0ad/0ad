#ifndef INCLUDED_LOS_FRAGMENT
#define INCLUDED_LOS_FRAGMENT

#if !IGNORE_LOS
	uniform sampler2D losTex;

	varying vec2 v_los;
#endif

float getLOS()
{
#if !IGNORE_LOS
	float los = texture2D(losTex, v_los).r;
	float threshold = 0.03;
	return clamp(los - threshold, 0.0, 1.0) / (1.0 - threshold);
#else
	return 1.0;
#endif
}

#endif // INCLUDED_LOS_FRAGMENT
