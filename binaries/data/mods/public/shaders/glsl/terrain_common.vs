#version 120

uniform mat4 transform;
uniform vec3 cameraPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec2 textureTransform;
uniform vec2 losTransform;
uniform mat4 shadowTransform;

#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
  uniform vec4 shadowScale;
#endif

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
varying vec2 v_blend;

#if USE_SPECULAR
  varying vec3 v_normal;
  varying vec3 v_half;
#endif

attribute vec3 a_vertex;
attribute vec3 a_color;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

void main()
{
  vec4 position = vec4(a_vertex, 1.0);

  gl_Position = transform * position;

  v_lighting = a_color * sunColor;
  
  #if DECAL
    v_tex = a_uv0;
  #else
    // Compute texcoords from position and terrain-texture-dependent transform
    float c = textureTransform.x;
    float s = -textureTransform.y;
    v_tex = vec2(a_vertex.x * c + a_vertex.z * -s, a_vertex.x * -s + a_vertex.z * -c);

    #if GL_ES
      // XXX: Ugly hack to hide some precision issues in GLES
      v_tex = mod(v_tex, vec2(9.0, 9.0));
    #endif
  #endif

  #if BLEND
    v_blend = a_uv1;
  #endif

  #if USE_SHADOW
    v_shadow = shadowTransform * vec4(a_vertex, 1.0);
    #if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
      v_shadow.xy *= shadowScale.xy;
    #endif  
  #endif

  #if USE_SPECULAR
    // TODO: for proper specular terrain, we need to provide vertex normals.
    // But we don't have that yet, so do something wrong instead.
    vec3 normal = vec3(0, 1, 0);

    vec3 eyeVec = normalize(cameraPos.xyz - position.xyz);
    vec3 sunVec = -sunDir;
    v_half = normalize(sunVec + eyeVec);
    v_normal = normal;
  #endif

  v_los = a_vertex.xz * losTransform.x + losTransform.yy;
}
