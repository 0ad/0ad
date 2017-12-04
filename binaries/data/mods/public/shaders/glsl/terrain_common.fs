#version 120

uniform sampler2D baseTex;
uniform sampler2D blendTex;
uniform sampler2D losTex;
uniform sampler2D normTex;
uniform sampler2D specTex;

#if USE_SHADOW
  uniform float shadowAngle;
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
    #if USE_SHADOW_PCF
      uniform vec4 shadowScale;
    #endif
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec3 sunColor;
uniform vec3 sunDir;

uniform vec3 fogColor;
uniform vec2 fogParams;

uniform vec2 textureTransform;

varying vec3 v_lighting;

#if USE_SHADOW
  varying vec4 v_shadow;
#endif

varying vec2 v_los;
varying vec2 v_blend;

#if USE_TRIPLANAR
  varying vec3 v_tex;
#else
  varying vec2 v_tex;
#endif

#if USE_SPECULAR
  uniform float specularPower;
  uniform vec3 specularColor;
#endif

#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_AO
  uniform vec4 effectSettings;
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

float get_shadow()
{
  float shadowBias = 0.0005;
  #if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
    float biasedShdwZ = v_shadow.z - shadowBias;
    #if USE_SHADOW_SAMPLER
      #if USE_SHADOW_PCF
        vec2 offset = fract(v_shadow.xy - 0.5);
        vec4 size = vec4(offset + 1.0, 2.0 - offset);
        vec4 weight = (vec4(1.0, 1.0, -0.5, -0.5) + (v_shadow.xy - 0.5*offset).xyxy) * shadowScale.zwzw;
        return (1.0/9.0)*dot(size.zxzx*size.wwyy,
          vec4(shadow2D(shadowTex, vec3(weight.zw, biasedShdwZ)).r,
               shadow2D(shadowTex, vec3(weight.xw, biasedShdwZ)).r,
               shadow2D(shadowTex, vec3(weight.zy, biasedShdwZ)).r,
               shadow2D(shadowTex, vec3(weight.xy, biasedShdwZ)).r));
      #else
        return shadow2D(shadowTex, vec3(v_shadow.xy, biasedShdwZ)).r;
      #endif
    #else
      if (biasedShdwZ >= 1.0)
        return 1.0;
      return (biasedShdwZ < texture2D(shadowTex, v_shadow.xy).x ? 1.0 : 0.0);
    #endif
  #else
    return 1.0;
  #endif
}

#if USE_TRIPLANAR
vec4 triplanar(sampler2D sampler, vec3 wpos)
{
	float tighten = 0.4679;

	vec3 blending = abs(normalize(v_normal)) - tighten;
	blending = max(blending, 0.0);

	blending /= vec3(blending.x + blending.y + blending.z);

	vec3 signedBlending = sign(v_normal) * blending;

	vec3 coords = wpos;
	coords.xyz /= 32.0;    // Ugh.

	vec4 col1 = texture2D(sampler, coords.yz);
	vec4 col2 = texture2D(sampler, coords.zx);
	vec4 col3 = texture2D(sampler, coords.yx);

	vec4 colBlended = col1 * blending.x + col2 * blending.y + col3 * blending.z;

	return colBlended;
}

vec4 triplanarNormals(sampler2D sampler, vec3 wpos)
{
	float tighten = 0.4679;

	vec3 blending = abs(normalize(v_normal)) - tighten;
	blending = max(blending, 0.0);

	blending /= vec3(blending.x + blending.y + blending.z);

	vec3 signedBlending = sign(v_normal) * blending;

	vec3 coords = wpos;
	coords.xyz /= 32.0;    // Ugh.

	vec4 col1 = texture2D(sampler, coords.yz).xyzw;
	col1.y = 1.0 - col1.y;
	vec4 col2 = texture2D(sampler, coords.zx).yxzw;
	col2.y = 1.0 - col2.y;
	vec4 col3 = texture2D(sampler, coords.yx).yxzw;
	col3.y = 1.0 - col3.y;

	vec4 colBlended = col1 * blending.x + col2 * blending.y + col3 * blending.z;

	return colBlended;
}
#endif

vec3 get_fog(vec3 color)
{
  float density = fogParams.x;
  float maxFog = fogParams.y;

  const float LOG2 = 1.442695;
  float z = gl_FragCoord.z / gl_FragCoord.w;
  float fogFactor = exp2(-density * density * z * z * LOG2);

  fogFactor = fogFactor * (1.0 - maxFog) + maxFog;

  fogFactor = clamp(fogFactor, 0.0, 1.0);

  return mix(fogColor, color, fogFactor);
}

void main()
{
  #if BLEND
    // Use alpha from blend texture
    gl_FragColor.a = 1.0 - texture2D(blendTex, v_blend).a;

    #if USE_GRASS
      if (gl_FragColor.a < LAYER / 10.0)
        discard;
    #endif
  #else
    gl_FragColor.a = 1.0;
  #endif

  #if USE_TRIPLANAR
    vec4 tex = triplanar(baseTex, v_tex);
  #else
    vec4 tex = texture2D(baseTex, v_tex.xy);
  #endif

  #if USE_GRASS && LAYER
    if (tex.a < 0.05)
      discard;
  #endif

  #if DECAL
    // Use alpha from main texture
    gl_FragColor.a = tex.a;
  #endif

  vec3 texdiffuse = tex.rgb;

  #if USE_SPECULAR || USE_SPECULAR_MAP || USE_NORMAL_MAP
    vec3 normal = v_normal;
  #endif

  #if USE_NORMAL_MAP
    float sign = v_tangent.w;
    mat3 tbn = mat3(v_tangent.xyz, v_bitangent * -sign, v_normal);
    #if USE_TRIPLANAR
      vec3 ntex = triplanarNormals(normTex, v_tex).rgb * 2.0 - 1.0;
    #else
      vec3 ntex = texture2D(normTex, v_tex).rgb * 2.0 - 1.0;
    #endif
    normal = normalize(tbn * ntex);
    vec3 bumplight = max(dot(-sunDir, normal), 0.0) * sunColor;
    vec3 sundiffuse = (bumplight - v_lighting.rgb) * effectSettings.x + v_lighting.rgb;   
  #else
    vec3 sundiffuse = v_lighting;
  #endif

  vec4 specular = vec4(0.0);
  #if USE_SPECULAR || USE_SPECULAR_MAP
    vec3 specCol;
    float specPow;
    #if USE_SPECULAR_MAP
      #if USE_TRIPLANAR
        vec4 s = triplanar(specTex, v_tex);
      #else
        vec4 s = texture2D(specTex, v_tex);
      #endif
      specCol = s.rgb;
      specular.a = s.a;
      specPow = effectSettings.y;
    #else
      specCol = specularColor;
      specPow = specularPower;
    #endif
    specular.rgb = sunColor * specCol * pow(max(0.0, dot(normalize(normal), v_half)), specPow);
  #endif

  vec3 color = (texdiffuse * sundiffuse + specular.rgb) * get_shadow() + texdiffuse * ambient;

  #if USE_SPECULAR_MAP && USE_SELF_LIGHT
    color = mix(texdiffuse, color, specular.a);
  #endif

  #if USE_FOG
    color = get_fog(color);
  #endif

  float los = texture2D(losTex, v_los).a;
  los = los < 0.03 ? 0.0 : los;
  color *= los;

  #if DECAL
    color *= shadingColor;
  #endif

  gl_FragColor.rgb = color;

  #if USE_GRASS
    gl_FragColor.a = tex.a;
  #endif
}
