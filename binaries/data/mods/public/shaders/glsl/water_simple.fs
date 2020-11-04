#version 110

uniform sampler2D baseTex;
uniform sampler2D losTex;
uniform vec3 color;

varying vec2 v_coords;
varying vec2 v_losCoords;

void main()
{
	float losMod = texture2D(losTex, v_losCoords.st).a;
	losMod = losMod < 0.03 ? 0.0 : losMod;
	gl_FragColor = vec4(texture2D(baseTex, v_coords).rgb * color * losMod, 1.0);
}
