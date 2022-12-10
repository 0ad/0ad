#version 110

#include "waves.h"

#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_basePosition);
VERTEX_INPUT_ATTRIBUTE(1, vec3, a_apexPosition);
VERTEX_INPUT_ATTRIBUTE(2, vec3, a_splashPosition);
VERTEX_INPUT_ATTRIBUTE(3, vec3, a_retreatPosition);
VERTEX_INPUT_ATTRIBUTE(4, vec2, a_normal);
VERTEX_INPUT_ATTRIBUTE(5, vec2, a_uv0);

void main()
{
	normal = a_normal;
	v_tex = a_uv0.xy;

	float tttime = mod(time + translation ,10.0);
	ttime = tttime;

	vec3 pos = mix(a_basePosition,a_apexPosition, clamp(ttime/3.0,0.0,1.0));
	pos = mix (pos, a_splashPosition, clamp(sin((min(ttime,6.1415926536)-3.0)/2.0),0.0,1.0));
	pos = mix (pos, a_retreatPosition, clamp(  1.0 - cos(max(0.0,ttime-6.1415926536)/2.0)  ,0.0,1.0));

	OUTPUT_VERTEX_POSITION(transform * vec4(pos, 1.0));
}
