#version 110

#if MINIMAP_BASE || MINIMAP_LOS
uniform sampler2D baseTex;
#endif

#if MINIMAP_BASE || MINIMAP_LOS
varying vec2 v_tex;
#endif

#if MINIMAP_POINT
varying vec3 color;
#endif

void main()
{
#if MINIMAP_BASE
	gl_FragColor = texture2D(baseTex, v_tex);
#endif

#if MINIMAP_LOS
	gl_FragColor = texture2D(baseTex, v_tex).rrrr;
#endif

#if MINIMAP_POINT
	gl_FragColor = vec4(color, 1.0);
#endif
}
