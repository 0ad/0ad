#ifndef INCLUDED_COMMON_UNIFORM
#define INCLUDED_COMMON_UNIFORM

#if USE_SPIRV

#if USE_DESCRIPTOR_INDEXING
	#define BEGIN_DRAW_TEXTURES struct DrawTextures {
	#define END_DRAW_TEXTURES };
	#define NO_DRAW_TEXTURES uint padding; // We can't have empty struct in GLSL.

	#define TEXTURE_2D(LOCATION, NAME) uint NAME;
	#define TEXTURE_2D_SHADOW(LOCATION, NAME) uint NAME;
	#define TEXTURE_CUBE(LOCATION, NAME) uint NAME;
	#define GET_DRAW_TEXTURE_2D(NAME) \
		textures2D[drawTextures.NAME]
	#define GET_DRAW_TEXTURE_2D_SHADOW(NAME) \
		texturesShadow[drawTextures.NAME]
	#define GET_DRAW_TEXTURE_CUBE(NAME) \
		texturesCube[drawTextures.NAME]
#else // USE_DESCRIPTOR_INDEXING
	#define BEGIN_DRAW_TEXTURES
	#define END_DRAW_TEXTURES
	#define NO_DRAW_TEXTURES

#if STAGE_FRAGMENT
	#define TEXTURE_2D(LOCATION, NAME) \
		layout (set = 1, binding = LOCATION) uniform sampler2D NAME;
	#define TEXTURE_2D_SHADOW(LOCATION, NAME) \
		layout (set = 1, binding = LOCATION) uniform sampler2DShadow NAME;
	#define TEXTURE_CUBE(LOCATION, NAME) \
		layout (set = 1, binding = LOCATION) uniform samplerCube NAME;
#else
	#define TEXTURE_2D(LOCATION, NAME)
	#define TEXTURE_2D_SHADOW(LOCATION, NAME)
	#define TEXTURE_CUBE(LOCATION, NAME)
#endif
	#define GET_DRAW_TEXTURE_2D(NAME) NAME
	#define GET_DRAW_TEXTURE_2D_SHADOW(NAME) NAME
	#define GET_DRAW_TEXTURE_CUBE(NAME) NAME
#endif // USE_DESCRIPTOR_INDEXING

#if USE_DESCRIPTOR_INDEXING
	#define BEGIN_DRAW_UNIFORMS layout (push_constant) uniform DrawUniforms {
	#define END_DRAW_UNIFORMS DrawTextures drawTextures; };
	#define BEGIN_MATERIAL_UNIFORMS layout (std140, set = 1, binding = 0) uniform MaterialUniforms {
	#define END_MATERIAL_UNIFORMS };
#else
	#define BEGIN_DRAW_UNIFORMS layout (push_constant) uniform DrawUniforms {
	#define END_DRAW_UNIFORMS };
	#define BEGIN_MATERIAL_UNIFORMS layout (std140, set = 0, binding = 0) uniform MaterialUniforms {
	#define END_MATERIAL_UNIFORMS };
#endif

#define UNIFORM(TYPE, NAME) \
	TYPE NAME;

#else // USE_SPIRV

#define BEGIN_DRAW_TEXTURES
#define END_DRAW_TEXTURES
#define NO_DRAW_TEXTURES

#if STAGE_FRAGMENT
	#define TEXTURE_2D(LOCATION, NAME) \
		uniform sampler2D NAME;
	#define TEXTURE_2D_SHADOW(LOCATION, NAME) \
		uniform sampler2DShadow NAME;
	#define TEXTURE_CUBE(LOCATION, NAME) \
		uniform samplerCube NAME;
#else
	#define TEXTURE_2D(LOCATION, NAME)
	#define TEXTURE_2D_SHADOW(LOCATION, NAME)
	#define TEXTURE_CUBE(LOCATION, NAME)
#endif

#define GET_DRAW_TEXTURE_2D(NAME) \
	NAME
#define GET_DRAW_TEXTURE_2D_SHADOW(NAME) \
	NAME
#define GET_DRAW_TEXTURE_CUBE(NAME) \
	NAME

#define BEGIN_DRAW_UNIFORMS
#define END_DRAW_UNIFORMS
#define BEGIN_MATERIAL_UNIFORMS
#define END_MATERIAL_UNIFORMS

#define UNIFORM(TYPE, NAME) \
	uniform TYPE NAME;

#endif // USE_SPIRV

#endif // INCLUDED_COMMON_UNIFORM
