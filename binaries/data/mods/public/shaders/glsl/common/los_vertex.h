#ifndef INCLUDED_COMMON_LOS_VERTEX
#define INCLUDED_COMMON_LOS_VERTEX

#if !IGNORE_LOS
vec2 calculateLOSCoordinates(vec2 position, vec2 losTransform)
{
	return position * losTransform.x + losTransform.y;
}
#endif

#endif // INCLUDED_COMMON_LOS_VERTEX
