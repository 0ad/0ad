#ifndef INCLUDED_DEBUG_FRAGMENT
#define INCLUDED_DEBUG_FRAGMENT

#define RENDER_DEBUG_MODE_NONE 0
#define RENDER_DEBUG_MODE_AO 1
#define RENDER_DEBUG_MODE_ALPHA 2
#define RENDER_DEBUG_MODE_CUSTOM 3

#ifndef RENDER_DEBUG_MODE
#define RENDER_DEBUG_MODE RENDER_DEBUG_MODE_NONE
#endif

vec3 applyDebugColor(vec3 color, float ao, float alpha, float custom)
{
#if RENDER_DEBUG_MODE == RENDER_DEBUG_MODE_AO
	return vec3(ao);
#elif RENDER_DEBUG_MODE == RENDER_DEBUG_MODE_ALPHA
	return vec3(alpha);
#elif RENDER_DEBUG_MODE == RENDER_DEBUG_MODE_CUSTOM
	return vec3(custom);
#else
	return color;
#endif
}

#endif // INCLUDED_DEBUG_FRAGMENT
