#version 110

varying vec3 v_tex;
attribute vec3 a_vertex;
attribute vec3 a_uv0;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(a_vertex, 1.0);
    v_tex = a_uv0.xyz;
}
