#version 120

uniform mat4 transform;
uniform vec3 cameraPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec2 losTransform;
uniform mat4 shadowTransform;
uniform mat4 instancingTransform;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;

#if USE_SPECULAR
  varying vec3 v_normal;
  varying vec3 v_half;
#endif

attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_uv0;

void main()
{
  #if USE_INSTANCING
    vec4 position = instancingTransform * vec4(a_vertex, 1.0);
    vec3 normal = mat3(instancingTransform) * a_normal;
  #else
    vec4 position = vec4(a_vertex, 1.0);
    vec3 normal = a_normal;
  #endif

  gl_Position = transform * position;

  #if USE_SPECULAR
    vec3 eyeVec = normalize(cameraPos.xyz - position.xyz);
    vec3 sunVec = -sunDir;
    v_half = normalize(sunVec + eyeVec);
    v_normal = normal;
  #endif
  
  v_lighting = max(0.0, dot(normal, -sunDir)) * sunColor;
  v_tex = a_uv0;
  v_shadow = shadowTransform * position;
  v_los = position.xz * losTransform.x + losTransform.y;
}
