uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec2 losTransform;
uniform mat4 shadowTransform;
uniform mat4 instancingTransform;

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;

void main()
{
  #ifdef USE_INSTANCING
    vec4 position = instancingTransform * gl_Vertex;
    vec3 normal = instancingTransform * vec4(gl_Normal, 0.0);
  #else
    vec4 position = gl_Vertex;
    vec3 normal = gl_Normal;
  #endif

  gl_Position = gl_ModelViewProjectionMatrix * position;

  v_lighting = max(0.0, dot(normal, -sunDir)) * sunColor;
  v_tex = gl_MultiTexCoord0.xy;
  v_shadow = shadowTransform * position;
  v_los = position.xz * losTransform.x + losTransform.y;
}
