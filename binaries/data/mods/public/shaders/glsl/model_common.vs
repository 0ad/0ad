#version 110

uniform mat4 transform;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec2 losTransform;
uniform mat4 shadowTransform;
uniform mat4 instancingTransform;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;

attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_uv0;

void main()
{
  #ifdef USE_INSTANCING
    vec4 position = instancingTransform * vec4(a_vertex, 1.0);
    vec3 normal = (instancingTransform * vec4(a_normal, 0.0)).xyz;
  #else
    vec4 position = vec4(a_vertex, 1.0);
    vec3 normal = a_normal;
  #endif

  gl_Position = transform * position;

  v_lighting = max(0.0, dot(normal, -sunDir)) * sunColor;
  v_tex = a_uv0;
  v_shadow = shadowTransform * position;
  v_los = position.xz * losTransform.x + losTransform.y;
}
