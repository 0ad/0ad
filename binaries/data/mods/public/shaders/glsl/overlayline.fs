#version 120

uniform sampler2D baseTex;
uniform sampler2D maskTex;
uniform sampler2D losTex;

#if USE_OBJECTCOLOR
uniform vec4 objectColor;
#else
varying vec4 v_color;
#endif

varying vec2 v_tex;

#if !IGNORE_LOS
varying vec2 v_los;
#endif

void main()
{
#if USE_OBJECTCOLOR
    vec3 color = objectColor.rgb;
    float alpha = objectColor.a;
#else
    vec3 color = v_color.rgb;
    float alpha = v_color.a;
#endif

    vec4 base = texture2D(baseTex, v_tex);
    vec4 mask = texture2D(maskTex, v_tex);
    color = mix(base.rgb, color, mask.r);

#if !IGNORE_LOS
    float los = texture2D(losTex, v_los).a;
    los = los < 0.03 ? 0.0 : los;
    color *= los;
#endif

    gl_FragColor = vec4(color, alpha * base.a);
}
