#version 120

uniform sampler2D baseTex;
uniform sampler2D losTex;


#if USE_SHADOW
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
uniform vec3 cameraPos;


uniform float specularPower;
uniform vec3 specularColor;


varying vec4 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
varying vec3 v_half;
varying vec3 v_normal;
varying float v_transp;
varying vec3 v_lighting;

float get_shadow()
{
  #if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
    #if USE_SHADOW_SAMPLER
      #if USE_SHADOW_PCF
        vec2 offset = fract(v_shadow.xy - 0.5);
        vec4 size = vec4(offset + 1.0, 2.0 - offset);
        vec4 weight = (vec4(1.0, 1.0, -0.5, -0.5) + (v_shadow.xy - 0.5*offset).xyxy) * shadowScale.zwzw;
        return (1.0/9.0)*dot(size.zxzx*size.wwyy,
          vec4(shadow2D(shadowTex, vec3(weight.zw, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.xw, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.zy, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.xy, v_shadow.z)).r));
      #else
        return shadow2D(shadowTex, v_shadow.xyz).r;
      #endif
    #else
      if (v_shadow.z >= 1.0)
        return 1.0;
      return (v_shadow.z <= texture2D(shadowTex, v_shadow.xy).x ? 1.0 : 0.0);
    #endif
  #else
    return 1.0;
  #endif
}


void main()
{
	//vec4 texdiffuse = textureGrad(baseTex, vec3(fract(v_tex.xy), v_tex.z), dFdx(v_tex.xy), dFdy(v_tex.xy));
	vec4 texdiffuse = texture2D(baseTex, fract(v_tex.xy));

	if (texdiffuse.a < 0.25)
		discard;

	texdiffuse.a *= v_transp;

	vec3 specular = sunColor * specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);

	vec3 color = (texdiffuse.rgb * v_lighting + specular) * get_shadow();
        color += texdiffuse.rgb * ambient;

	#if !IGNORE_LOS
		float los = texture2D(losTex, v_los).a;
		los = los < 0.03 ? 0.0 : los;
		color *= los;
	#endif

	gl_FragColor.rgb = color;
	gl_FragColor.a = texdiffuse.a;
}

