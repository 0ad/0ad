#ifndef INCLUDED_COMMON_TEXTURE
#define INCLUDED_COMMON_TEXTURE

#if USE_SPIRV
	#define SAMPLE_2D texture
	#define SAMPLE_2D_SHADOW texture
	#define SAMPLE_CUBE texture
#else
	#define SAMPLE_2D texture2D
	#define SAMPLE_2D_SHADOW shadow2D
	#define SAMPLE_CUBE textureCube
#endif

#endif // INCLUDED_COMMON_TEXTURE
