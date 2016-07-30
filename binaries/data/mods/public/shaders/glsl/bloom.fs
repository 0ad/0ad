#version 110

varying vec2 v_tex;
uniform sampler2D renderedTex;
uniform vec2 texSize;

void main()
{
  #if BLOOM_NOP
    gl_FragColor = texture2D(renderedTex, v_tex);
    gl_FragColor.a = 1.0;
  #endif

  #if BLOOM_PASS_H
    vec4 color = vec4(0.0);
    vec2 v_tex_offs = vec2(v_tex.x - 0.01, v_tex.y);
    
    for (int i = 0; i < 6; ++i)
    {
      color += texture2D(renderedTex, v_tex_offs);
      v_tex_offs += vec2(0.004, 0.0);
    }
    
    gl_FragColor.rgb = color.rgb / 6.0;
    gl_FragColor.a = 1.0;
  #endif

  #if BLOOM_PASS_V
    vec4 color = vec4(0.0);
    vec2 v_tex_offs = vec2(v_tex.x, v_tex.y - 0.01);
    
    for (int i = 0; i < 6; ++i)
    {
      color += texture2D(renderedTex, v_tex_offs);
      v_tex_offs += vec2(0.0, 0.004);
    }
    
    gl_FragColor.rgb = color.rgb / 6.0;
    gl_FragColor.a = 1.0;
  #endif
}