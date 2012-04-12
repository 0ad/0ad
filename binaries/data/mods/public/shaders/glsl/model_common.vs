#if USE_GPU_SKINNING
// Skinning requires GLSL 1.30 for ivec4 vertex attributes
#version 130
#else
#version 120
#endif

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

#if USE_GPU_SKINNING
  const int MAX_INFLUENCES = 4;
  const int MAX_BONES = 64;
  uniform mat4 skinBlendMatrices[MAX_BONES];
  attribute ivec4 a_skinJoints;
  attribute vec4 a_skinWeights;
#endif

varying vec4 debugx;

void main()
{
  #if USE_GPU_SKINNING
    vec3 p = vec3(0.0);
    vec3 n = vec3(0.0);
    for (int i = 0; i < MAX_INFLUENCES; ++i) {
      int joint = a_skinJoints[i];
      if (joint != 0xff) {
        mat4 m = skinBlendMatrices[joint];
        p += vec3(m * vec4(a_vertex, 1.0)) * a_skinWeights[i];
        n += vec3(m * vec4(a_normal, 0.0)) * a_skinWeights[i];
      }
    }
    vec4 position = instancingTransform * vec4(p, 1.0);
    vec3 normal = mat3(instancingTransform) * normalize(n);
  #else
  #if USE_INSTANCING
    vec4 position = instancingTransform * vec4(a_vertex, 1.0);
    vec3 normal = mat3(instancingTransform) * a_normal;
  #else
    vec4 position = vec4(a_vertex, 1.0);
    vec3 normal = a_normal;
  #endif
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
