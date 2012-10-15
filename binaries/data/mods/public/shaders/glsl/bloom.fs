#version 110

varying vec2 v_tex;

uniform sampler2D renderedTex;

uniform vec2 texSize;


void main()
{
  #if BLOOM_NOP
    gl_FragColor = texture2D(renderedTex, v_tex);
  #endif

  #if BLOOM_PASS_H
    vec4 colour;

    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(-2.5, 0.0)) / texSize) * 0.05;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(-1.5, 0.0)) / texSize) * 0.1;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(-0.5, 0.0)) / texSize) * 0.2;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2( 0.0, 0.0)) / texSize) * 0.3;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2( 0.5, 0.0)) / texSize) * 0.2;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2( 1.5, 0.0)) / texSize) * 0.1;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2( 2.5, 0.0)) / texSize) * 0.05;

    gl_FragColor.rgb = colour.rgb;
    gl_FragColor.a = 1.0;
  #endif

  #if BLOOM_PASS_V
    vec4 colour;

    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(0.0, -2.5)) / texSize) * 0.05;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(0.0, -1.5)) / texSize) * 0.1;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(0.0, -0.5)) / texSize) * 0.2;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(0.0,  0.0)) / texSize) * 0.3;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(0.0,  0.5)) / texSize) * 0.2;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(0.0,  1.5)) / texSize) * 0.1;
    colour += texture2D(renderedTex, (gl_FragCoord.xy + vec2(0.0,  2.5)) / texSize) * 0.05;

    gl_FragColor.rgb = colour.rgb;
    gl_FragColor.a = 1.0;
  #endif

}



/*varying vec2 v_tex;

uniform sampler2D bgl_RenderedTexture;
uniform sampler2D bgl_DepthTexture;
uniform sampler2D bgl_LuminanceTexture;
uniform float bgl_RenderedTextureWidth;
uniform float bgl_RenderedTextureHeight;

#define PI    3.14159265

float width = bgl_RenderedTextureWidth; //texture width
float height = bgl_RenderedTextureHeight; //texture height

vec2 texCoord = v_tex;
vec2 texcoord = v_tex;


float BRIGHT_PASS_THRESHOLD = 0.6;
float BRIGHT_PASS_OFFSET = 0.6;

#define blurclamp 0.0015
#define bias 0.01

#define KERNEL_SIZE  3.0



vec4 bright(vec2 coo)
{	
	vec4 color = texture2D(bgl_RenderedTexture, coo);
	
	color = max(color - BRIGHT_PASS_THRESHOLD, 0.0);
	
	return color / (color + BRIGHT_PASS_OFFSET);	
}


void main0(void)
{	
	vec2 blur = vec2(clamp( bias, -blurclamp, blurclamp ));
	
	vec4 col = vec4( 0, 0, 0, 0 );
	for ( float x = -KERNEL_SIZE + 1.0; x < KERNEL_SIZE; x += 1.0 )
	{
	for ( float y = -KERNEL_SIZE + 1.0; y < KERNEL_SIZE; y += 1.0 )
	{
		 col += bright( texcoord + vec2( blur.x * x, blur.y * y ) );
	}
	}
	col /= ((KERNEL_SIZE+KERNEL_SIZE)-1.0)*((KERNEL_SIZE+KERNEL_SIZE)-1.0);

	//gl_FragColor = col + texture2D(bgl_RenderedTexture, texcoord);

	//col *= 0.9;
	gl_FragColor = 1.0 - (1.0 - col) * (1.0 - texture2D(bgl_RenderedTexture, texcoord));

}*/

