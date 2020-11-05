#version 110

attribute vec3 a_vertex;

uniform mat4 transform;
uniform mat4 losMatrix;
uniform float time;

varying vec2 v_coords;
varying vec2 v_losCoords;

void main()
{
	// Shift the texture coordinates by these amounts to make the water "flow"
	float tx = -mod(time, 81.0) / 81.0;
	float tz = -mod(time, 34.0) / 34.0;
	float repeatPeriod = 16.0;

	v_coords = a_vertex.xz / repeatPeriod + vec2(tx, tz);
	v_losCoords = (losMatrix * vec4(a_vertex, 1.0)).rg;
	gl_Position = transform * vec4(a_vertex, 1.0);
}
