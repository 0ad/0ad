#version 120

#include "common/los_vertex.h"
#include "common/shadows_vertex.h"

uniform mat4 transform;
uniform vec3 cameraPos;
#ifdef GL_ES
uniform mediump vec3 sunDir;
uniform mediump vec3 sunColor;
#else
uniform vec3 sunDir;
uniform vec3 sunColor;
#endif
uniform vec2 textureTransform;

varying vec3 v_lighting;

varying vec2 v_blend;

#if USE_TRIPLANAR
  varying vec3 v_tex;
#else
  varying vec2 v_tex;
#endif

varying vec3 v_normal;

#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP
  #if USE_NORMAL_MAP
    varying vec4 v_tangent;
    varying vec3 v_bitangent;
  #endif
  #if USE_SPECULAR || USE_SPECULAR_MAP
    varying vec3 v_half;
  #endif
#endif

attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

void main()
{
  vec4 position = vec4(a_vertex, 1.0);

  gl_Position = transform * position;

  v_lighting = clamp(-dot(a_normal, sunDir), 0.0, 1.0) * sunColor;

  #if DECAL
    v_tex.xy = a_uv0;
  #else

    #if USE_TRIPLANAR
      v_tex = a_vertex;
    #else
      // Compute texcoords from position and terrain-texture-dependent transform
      float c = textureTransform.x;
      float s = -textureTransform.y;
      v_tex = vec2(a_vertex.x * c + a_vertex.z * -s, a_vertex.x * -s + a_vertex.z * -c);
    #endif

    #if GL_ES
      // XXX: Ugly hack to hide some precision issues in GLES
      #if USE_TRIPLANAR
        v_tex = mod(v_tex, vec3(9.0, 9.0, 9.0));
      #else
        v_tex = mod(v_tex, vec2(9.0, 9.0));
      #endif
    #endif
  #endif

  #if BLEND
    v_blend = a_uv1;
  #endif

  calculatePositionInShadowSpace(vec4(a_vertex, 1.0));

  v_normal = a_normal;

  #if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_TRIPLANAR
    #if USE_NORMAL_MAP
      vec3 t = vec3(1.0, 0.0, 0.0);
      t = normalize(t - v_normal * dot(v_normal, t));
      v_tangent = vec4(t, -1.0);
      v_bitangent = cross(v_normal, t);
    #endif

    #if USE_SPECULAR || USE_SPECULAR_MAP
      vec3 eyeVec = cameraPos.xyz - position.xyz;
      #if USE_SPECULAR || USE_SPECULAR_MAP
        vec3 sunVec = -sunDir;
        v_half = normalize(sunVec + normalize(eyeVec));
      #endif
    #endif
  #endif

  calculateLOSCoordinates(a_vertex.xz);
}
