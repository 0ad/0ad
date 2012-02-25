#version 110

uniform mat4 transform;
uniform vec3 sunColor;
uniform vec2 textureTransform;
uniform vec2 losTransform;
uniform mat4 shadowTransform;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
varying vec2 v_blend;

attribute vec3 a_vertex;
attribute vec3 a_color;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

void main()
{
  gl_Position = transform * vec4(a_vertex, 1.0);

  v_lighting = a_color * sunColor;
  
#ifdef DECAL
  v_tex = a_uv0;
#else
  // Compute texcoords from position and terrain-texture-dependent transform
  float c = textureTransform.x;
  float s = -textureTransform.y;
  v_tex = vec2(a_vertex.x * c + a_vertex.z * -s, a_vertex.x * -s + a_vertex.z * -c);
#endif

#ifdef BLEND
  v_blend = a_uv1;
#endif

#ifdef USE_SHADOW
  v_shadow = shadowTransform * vec4(a_vertex, 1.0);
#endif

  v_los = a_vertex.xz * losTransform.x + losTransform.yy;
}
