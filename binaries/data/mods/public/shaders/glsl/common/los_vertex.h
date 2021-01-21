#ifndef INCLUDED_LOS_VERTEX
#define INCLUDED_LOS_VERTEX

#if !IGNORE_LOS
	uniform vec2 losTransform;

	varying vec2 v_los;
#endif

void calculateLOSCoordinates(vec2 position)
{
#if !IGNORE_LOS
	v_los = position * losTransform.x + losTransform.y;
#endif
}

#endif // INCLUDED_LOS_VERTEX
