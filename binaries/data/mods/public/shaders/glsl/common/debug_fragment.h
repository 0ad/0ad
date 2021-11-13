#ifndef INCLUDED_DEBUG_FRAGMENT
#define INCLUDED_DEBUG_FRAGMENT

#define RENDER_DEBUG_MODE_NONE 0
#define RENDER_DEBUG_MODE_AO 1

#ifndef RENDER_DEBUG_MODE
#define RENDER_DEBUG_MODE RENDER_DEBUG_MODE_NONE
#endif

vec3 applyDebugColor(vec3 color, float ao)
{
#if RENDER_DEBUG_MODE == RENDER_DEBUG_MODE_AO
	return vec3(ao);
#else
	return color;
#endif
}

#endif // INCLUDED_DEBUG_FRAGMENT
