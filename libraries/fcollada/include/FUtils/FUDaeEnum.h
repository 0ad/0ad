/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FU_DAE_ENUM_H_
#define _FU_DAE_ENUM_H_

#undef NEVER // 3dsMax defines NEVER in the global namespace.
#undef TRANSPARENT // Win32: GDI defines TRANSPARENT in the global namespace
#undef RELATIVE // Win32: GDI defines RELATIVE in the global namespace
#undef IN
#undef OUT
#undef DIFFERENCE

// Animation curve interpolation function
// Defaults to the STEP interpolation by definition in COLLADA.
// BEZIER is the more common interpolation type
namespace FUDaeInterpolation
{
	enum Interpolation
	{
		STEP = 0, //equivalent to no interpolation
		LINEAR,
		BEZIER,
		TCB,

		UNKNOWN,
		DEFAULT = STEP,
	};

	FCOLLADA_EXPORT Interpolation FromString(const fm::string& value);
	const char* ToString(const Interpolation& value);
};

typedef fm::vector<FUDaeInterpolation::Interpolation> FUDaeInterpolationList;

// Spline type values
namespace FUDaeSplineType
{
	enum Type
	{
		LINEAR = 0,
		BEZIER,
		NURBS,

		UNKNOWN,
		DEFAULT = NURBS,
	};

	FCOLLADA_EXPORT Type FromString(const fm::string& value);
	const char* ToString(const Type& value);
};

// Spline form values
namespace FUDaeSplineForm
{
	enum Form
	{
		OPEN = 0,
		CLOSED,

		UNKNOWN,
		DEFAULT = OPEN,
	};

	FCOLLADA_EXPORT Form FromString(const fm::string& value);
	const char* ToString(const Form& value);
};


// COLLADA generic degree function. Used by lights and the profile_COMMON materials.
namespace FUDaeFunction
{
	enum Function
	{
		CONSTANT = 0,
		LINEAR,
		QUADRATIC,

		UNKNOWN,
		DEFAULT = CONSTANT,
	};

	FCOLLADA_EXPORT Function FromString(const char* value);
	inline Function FromString(const fm::string& value) { return FromString(value.c_str()); }
};

// Material texture channels. Used by profile_common materials to assign textures to channels/slots
// Multi-texturing is done by assigning more than one texture per slot.
// Defaults to diffuse texture slot
namespace FUDaeTextureChannel
{
	enum Channel
	{
		AMBIENT = 0,
		BUMP,
		DIFFUSE,
		DISPLACEMENT,
		EMISSION,
		FILTER,
		OPACITY,
		REFLECTION,
		REFRACTION,
		SHININESS,
		SPECULAR,
		SPECULAR_LEVEL,
		TRANSPARENT,

		COUNT,
		UNKNOWN,
		DEFAULT = DIFFUSE,
	};

	FCOLLADA_EXPORT Channel FromString(const fm::string& value);
};

// Morph controller method
// NORMALIZED implies that the morph targets all have absolute vertex positions
// RELATIVE implies that the morph targets have relative vertex positions
//
// Whether the vertex position is relative or absolute is irrelevant,
// as long as you use the correct weight generation function:
// NORMALIZED: base_weight = 1.0f - SUM(weight[t])
// RELATIVE: base_weight = 1.0f
// and position[k] = SUM(weight[t][k] * position[t][k])
namespace FUDaeMorphMethod
{
	enum Method
	{
		NORMALIZED = 0,
		RELATIVE,

		UNKNOWN,
		DEFAULT = NORMALIZED,
	};

	FCOLLADA_EXPORT Method FromString(const char* value);
	FCOLLADA_EXPORT const char* ToString(Method method);
	inline Method FromString(const fm::string& value) { return FromString(value.c_str()); }
};

// Maya uses the infinity to determine what happens outside an animation curve
// Intentionally matches the MFnAnimCurve::InfinityType enum
namespace FUDaeInfinity
{
	enum Infinity
	{
		CONSTANT = 0,
		LINEAR,
		CYCLE,
		CYCLE_RELATIVE,
		OSCILLATE,

		UNKNOWN,
		DEFAULT = CONSTANT
	};

	FCOLLADA_EXPORT Infinity FromString(const char* value);
	FCOLLADA_EXPORT const char* ToString(Infinity infinity);
	inline Infinity FromString(const fm::string& value) { return FromString(value.c_str()); }
};

// Maya uses the blend mode for texturing purposes
// Intentionally matches the equivalent Maya enum
namespace FUDaeBlendMode
{
	enum Mode
	{
		NONE,
		OVER,
		IN,
		OUT,
		ADD,
		SUBSTRACT,
		MULTIPLY,
		DIFFERENCE,
		LIGHTEN,
		DARKEN,
		SATURATE,
		DESATURATE,
		ILLUMINATE,

		UNKNOWN,
		DEFAULT = NONE,
	};

	FCOLLADA_EXPORT Mode FromString(const char* value);
	FCOLLADA_EXPORT const char* ToString(Mode mode);
	inline Mode FromString(const fm::string& value) { return FromString(value.c_str()); }
}

// Geometry Input Semantics
// Found in the <mesh><vertices>, <mesh><polygons>...
namespace FUDaeGeometryInput
{
	enum Semantic
	{
		POSITION = 0,
		VERTEX,
		NORMAL,
		GEOTANGENT,
		GEOBINORMAL,
		TEXCOORD,
		TEXTANGENT,
		TEXBINORMAL,
		UV,
		COLOR,
		EXTRA, // Maya-specific, used for blind data

		UNKNOWN = -1,
	};
	typedef fm::vector<Semantic> SemanticList;

	FCOLLADA_EXPORT Semantic FromString(const char* value);
    FCOLLADA_EXPORT const char* ToString(Semantic semantic);
	inline Semantic FromString(const fm::string& value) { return FromString(value.c_str()); }
}

/** The types of effect profiles. */
namespace FUDaeProfileType
{
	/** Enumerates the types of effect profiles. */
	enum Type
	{
		CG, /**< The CG profile. */
		HLSL, /**< The HLSL profile for DirectX. */
		GLSL, /**< The GLSL profile for OpenGL. */
		GLES, /**< The GLES profile for OpenGL ES. */
		COMMON, /**< The common profile which encapsulates the standard lighting equations: Lambert, Phong, Blinn. */

		UNKNOWN /**< Not a valid profile type. */
	};

	/** Converts the COLLADA profile name string to a profile type.
		Examples of COLLADA profile name strings are 'profile_CG' and 'profile_COMMON'.
		@param value The profile name string.
		@return The profile type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the profile type to its COLLADA profile name string.
		Examples of COLLADA profile name strings are 'profile_CG' and 'profile_COMMON'.
		@param type The profile type.
		@return The name string for the profile type. */
	FCOLLADA_EXPORT const char* ToString(Type type);
}

/** The function types for the effect pass render states. */
namespace FUDaePassStateFunction
{
	/** Enumerates the COLLADA render state function types. */
	enum Function
	{
		NEVER = 0x0200,
		LESS = 0x0201,
		EQUAL = 0x0202,
		LESS_EQUAL = 0x0203,
		GREATER = 0x0204,
		NOT_EQUAL = 0x0205,
		GREATER_EQUAL = 0x0206,
		ALWAYS = 0x0207,

		INVALID
	};

	/** Converts the COLLADA render state function string to the render state function type.
		@param value The render state function string.
		@return The render state function type. */
	FCOLLADA_EXPORT Function FromString(const char* value);
	inline Function FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the render state function type to its COLLADA render state function string.
		@param fn The render state function type.
		@return The render state function string. */
	FCOLLADA_EXPORT const char* ToString(Function fn);
};

/** The stencil operation types for the effect pass render states. */
namespace FUDaePassStateStencilOperation
{
	/** Enumerates the COLLADA stencil operation types. */
	enum Operation
	{
		KEEP = 0x1E00,
		ZERO = 0x0000,
		REPLACE = 0x1E01,
		INCREMENT = 0x1E02,
		DECREMENT = 0x1E03,
		INVERT = 0x1E0A,
		INCREMENT_WRAP = 0x8507,
		DECREMENT_WRAP = 0x8508,

		INVALID
	};

	/** Converts the COLLADA render state stencil operation string to the stencil operation type.
		@param value The render state stencil operation string.
		@return The stencil operation type. */
	FCOLLADA_EXPORT Operation FromString(const char* value);
	inline Operation FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the stencil operation type to its COLLADA render state function string.
		@param op The stencil operation type.
		@return The render state stencil operation string. */
	FCOLLADA_EXPORT const char* ToString(Operation op);
};

/** The render state blend types for effect passes. */
namespace FUDaePassStateBlendType
{
	/** Enumerates the COLLADA render state blend types. */
	enum Type
	{
		ZERO = 0x0000,
		ONE = 0x0001,
		SOURCE_COLOR = 0x0300,
		ONE_MINUS_SOURCE_COLOR = 0x0301,
		DESTINATION_COLOR = 0x0306,
		ONE_MINUS_DESTINATION_COLOR = 0x0307,
		SOURCE_ALPHA = 0x0302,
		ONE_MINUS_SOURCE_ALPHA = 0x0303,
		DESTINATION_ALPHA = 0x0304,
		ONE_MINUS_DESTINATION_ALPHA = 0x0305,
		CONSTANT_COLOR = 0x8001,
		ONE_MINUS_CONSTANT_COLOR = 0x8002,
		CONSTANT_ALPHA = 0x8003,
		ONE_MINUS_CONSTANT_ALPHA = 0x8004,
		SOURCE_ALPHA_SATURATE = 0x0308,

		INVALID
	};

	/** Converts the COLLADA render state blend type string to the blend type.
		@param value The render state blend type string.
		@return The blend type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the blend type to its COLLADA render state blend type string.
		@param type The blend type.
		@return The render state blend type string. */
	FCOLLADA_EXPORT const char* ToString(Type type);
};

/** The render state face types. */
namespace FUDaePassStateFaceType
{
	enum Type
	{
		FRONT = 0x0404,
		BACK = 0x0405,
		FRONT_AND_BACK = 0x0408,

		INVALID
	};

	/** Converts the COLLADA render state face type string to the blend type.
		@param value The render state face type string.
		@return The face type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the face type to its COLLADA render state face type string.
		@param type The face type.
		@return The render state face type string. */
	FCOLLADA_EXPORT const char* ToString(Type type);
};

/** The render state blend equation types. */
namespace FUDaePassStateBlendEquation
{
	enum Equation
	{
		ADD = 0x8006,
		SUBTRACT = 0x800A,
		REVERSE_SUBTRACT = 0x800B,
		MIN = 0x8007,
		MAX = 0x8008,

		INVALID
	};

	/** Converts the COLLADA render state face type string to the blend equation type.
		@param value The render state blend equation type string.
		@return The face type. */
	FCOLLADA_EXPORT Equation FromString(const char* value);
	inline Equation FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the blend equation type to its COLLADA render state blend equation type string.
		@param equation The blend equation type.
		@return The render state blend equation type string. */
	FCOLLADA_EXPORT const char* ToString(Equation equation);
};

/** The render state material types. */
namespace FUDaePassStateMaterialType
{
	enum Type
	{
		EMISSION = 0x1600,
		AMBIENT = 0x1200,
		DIFFUSE = 0x1201,
		SPECULAR = 0x1202,
		AMBIENT_AND_DIFFUSE = 0x1602,

		INVALID
	};

	/** Converts the COLLADA render state material type string to the material type.
		@param value The render state material type string.
		@return The material type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the material type to its COLLADA render state material type string.
		@param type The material type.
		@return The render state material type string. */
	FCOLLADA_EXPORT const char* ToString(Type type);
};

/** The render state fog types. */
namespace FUDaePassStateFogType
{
	enum Type
	{
		LINEAR = 0x2601,
		EXP = 0x0800,
		EXP2 = 0x0801,

		INVALID
	};

	/** Converts the COLLADA render state fog type string to the fog type.
		@param value The render state fog type string.
		@return The fog type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the fog type to its COLLADA render state fog type string.
		@param type The fog type.
		@return The render state fog type string. */
	FCOLLADA_EXPORT const char* ToString(Type type);
}

/** The render state fog coordinate source types. */
namespace FUDaePassStateFogCoordinateType
{
	enum Type
	{
		FOG_COORDINATE = 0x8451,
		FRAGMENT_DEPTH = 0x8452,

		INVALID
	};

	/** Converts the COLLADA render state fog coordinate source type string to the fog coordinate source type.
		@param value The render state fog coordinate source type string.
		@return The fog coordinate source type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the fog coordinate source type to its COLLADA render state fog coordinate source type string.
		@param type The fog coordinate source type.
		@return The render state fog coordinate source type string. */
	FCOLLADA_EXPORT const char* ToString(Type type);
};

/** The render state front face types. */
namespace FUDaePassStateFrontFaceType
{
	enum Type
	{
		CLOCKWISE = 0x0900,
		COUNTER_CLOCKWISE = 0x0901,

		INVALID
	};

	/** Converts the COLLADA render state front-face type string to the front-face type.
		@param value The render state front-face type string.
		@return The front-face type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the front-face type to its COLLADA render state front-face type string.
		@param type The front-face type.
		@return The render state front-face type string. */
	FCOLLADA_EXPORT const char* ToString(Type type);
};

/** The render state logic operations. */
namespace FUDaePassStateLogicOperation
{
	enum Operation
	{
		CLEAR = 0x1500,
		AND = 0x1501,
		AND_REVERSE = 0x1502,
		COPY = 0x1503,
		AND_INVERTED = 0x1504,
		NOOP = 0x1505,
		XOR = 0x1506,
		OR = 0x1507,
		NOR = 0x1508,
		EQUIV = 0x1509,
		INVERT = 0x150A,
		OR_REVERSE = 0x150B,
		COPY_INVERTED = 0x150C,
		NAND = 0x150E,
		SET = 0x150F,

		INVALID
	};

	/** Converts the COLLADA render state logic operation type string to the logic operation.
		@param value The render state logic operation type string.
		@return The logic operation. */
	FCOLLADA_EXPORT Operation FromString(const char* value);
	inline Operation FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the logic operation to its COLLADA render state logic operation type string.
		@param op The logic operation.
		@return The render state logic operation type string. */
	FCOLLADA_EXPORT const char* ToString(Operation op);
};

/** The render state polygon modes. */
namespace FUDaePassStatePolygonMode
{
	enum Mode
	{
		POINT = 0x1B00,
		LINE = 0x1B01,
		FILL = 0x1B02,

		INVALID
	};

	/** Converts the COLLADA render state polygon mode type string to the polygon mode.
		@param value The render state polygon mode type string.
		@return The polygon mode. */
	FCOLLADA_EXPORT Mode FromString(const char* value);
	inline Mode FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the polygon mode to its COLLADA render state polygon mode type string.
		@param mode The polygon mode.
		@return The render state polygon mode type string. */
	FCOLLADA_EXPORT const char* ToString(Mode mode);
};

/** The render state shading model. */
namespace FUDaePassStateShadeModel
{
	enum Model
	{
		FLAT = 0x1D00, /** Flat shading. */
		SMOOTH = 0x1D01, /** Gouraud shading. */

		INVALID
	};

	/** Converts the COLLADA render state shading model type string to the shading model.
		@param value The render state shading model type string.
		@return The polygon mode. */
	FCOLLADA_EXPORT Model FromString(const char* value);
	inline Model FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the shading model to its COLLADA render state shading model type string.
		@param mode The shading model.
		@return The render state shading model type string. */
	FCOLLADA_EXPORT const char* ToString(Model model);
};

/** The render state light model color control types. */
namespace FUDaePassStateLightModelColorControlType
{
	enum Type
	{
		SINGLE_COLOR = 0x81F9,
		SEPARATE_SPECULAR_COLOR = 0x81FA,

		INVALID
	};

	/** Converts the COLLADA render state light model color control type string to the light model color control type.
		@param value The render state light model color control type string.
		@return The light model color control type. */
	FCOLLADA_EXPORT Type FromString(const char* value);
	inline Type FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the light model color control type to its COLLADA render state light model color control type string.
		@param type The light model color control type.
		@return The render state light model color control type string. */
	FCOLLADA_EXPORT const char* ToString(Type type);
};

/** The render states for effect passes. */
namespace FUDaePassState
{
	/** Enumerates the COLLADA render states for effect passes.
		Each state description states the structure allocated in FCollada for this state.
		For example, ALPHA_FUNC describes: { FUDaePassStateFunction function = ALWAYS, float alphaComparison = 0.0f }.
		This implies a 8-byte structure with
		1) FUDaePassStateFunction::Function at offset 0; it defaults to the ALWAYS enumerated type.
		2) The alpha comparison value at offset 4; it defaults to 0.0f.
		All enumerated types are 4 byte in length. */
	enum State
	{
		ALPHA_FUNC = 0, /**< { FUDaePassStateFunction function = ALWAYS, float alphaComparison = 0.0f } */
		BLEND_FUNC, /**< { FUDaePassStateBlendType sourceBlend = ONE, FUDaePassStateBlendType destinationBlend = ZERO } */
		BLEND_FUNC_SEPARATE, /**< { FUDaePassStateBlendType sourceColorBlend = ONE, FUDaePassStateBlendType destinationColorBlend = ZERO, FUDaePassStateBlendType sourceAlphaBlend = ONE, FUDaePassStateBlendType destinationAlphaBlend = ZERO } */
		BLEND_EQUATION, /**< { FUDaePassStateBlendEquation blendEquation = ADD } */
		BLEND_EQUATION_SEPARATE, /**< { FUDaePassStateBlendEquation colorEquation = ADD, FUDaePassStateBlendEquation alphaEquation = ADD } */
		COLOR_MATERIAL, /**< { FUDaePassStateFaceType whichFaces = FRONT_AND_BACK, FUDaePassStateMaterialType material = AMBIENT_AND_DIFFUSE } */
		CULL_FACE, /**< { FUDaePassStateFaceType culledFaces = BACK } */
		DEPTH_FUNC, /**< { FUDaePassStateFunction depthAcceptFunction = ALWAYS } */
		FOG_MODE, /**< { FUDaePassStateFogType fogType = EXP } */
		FOG_COORD_SRC, /**< { FUDaePassStateFogCoordinateType type = FOG_COORDINATE } */

		FRONT_FACE = 10, /**< { FUDaePassStateFrontFaceType frontFaceSide = COUNTER_CLOCKWISE } */
		LIGHT_MODEL_COLOR_CONTROL, /**< { FUDaePassStateLightModelColorControlType controlType = SINGLE_COLOR } */
		LOGIC_OP, /**< { FUDaePassStateLogicOperation operation = COPY } */
		POLYGON_MODE, /**< { FUDaePassStateFaceType whichFaces = FRONT_AND_BACK, FUDaePassStatePolygonMode renderMode = FILL } */
		SHADE_MODEL, /**< { FUDaePassStateShadeModel model = SMOOTH } */
		STENCIL_FUNC, /**< { FUDaePassStateFunction acceptFunction = ALWAYS, uint8 referenceValue = 0, uint8 mask = 0xFF } */
		STENCIL_OP, /**< { FUDaePassStateStencilOperation failOperation = KEEP, FUDaePassStateStencilOperation depthFailOperation = KEEP, FUDaePassStateStencilOperation depthPassOperation = KEEP } */
		STENCIL_FUNC_SEPARATE, /**< { FUDaePassStateFunction frontFacesAcceptFunction = ALWAYS, FUDaePassStateFunction backFacesAcceptFunction = ALWAYS, uint8 referenceValue = 0, uint8 mask = 0xFF } */
		STENCIL_OP_SEPARATE, /**< { FUDaePassStateFaceType whichFaces = FRONT_AND_BACK, FUDaePassStateStencilOperation failOperation = KEEP, FUDaePassStateStencilOperation depthFailOperation = KEEP, FUDaePassStateStencilOperation depthPassOperation = KEEP } */
		STENCIL_MASK_SEPARATE, /**< { FUDaePassStateFaceType whichFaces = FRONT_AND_BACK, uint8 mask = 0xFF } */

		LIGHT_ENABLE = 20, /**< { uint8 lightIndex = 0, bool enabled = false } */
		LIGHT_AMBIENT, /**< { uint8 lightIndex = 0, FMVector4 ambientColor = [0,0,0,1] } */
		LIGHT_DIFFUSE, /**< { uint8 lightIndex = 0, FMVector4 diffuseColor = [0,0,0,0] } */
		LIGHT_SPECULAR, /**< { uint8 lightIndex = 0, FMVector4 specularColor = [0,0,0,0] } */
		LIGHT_POSITION, /**< { uint8 lightIndex = 0, FMVector4 position = [0,0,1,0] } */
		LIGHT_CONSTANT_ATTENUATION, /**< { uint8 lightIndex = 0, float constantAttenuation = 1.0f } */
		LIGHT_LINEAR_ATTENUATION, /**< { uint8 lightIndex = 0, float linearAttenuation = 0.0f } */
		LIGHT_QUADRATIC_ATTENUATION, /**< { uint8 lightIndex = 0, float quadraticAttenuation = 0.0f } */
		LIGHT_SPOT_CUTOFF, /**< { uint8 lightIndex = 0, float cutoff = 180.0f } */
		LIGHT_SPOT_DIRECTION, /**< { uint8 lightIndex = 0, FMVector4 direction = [0,0,-1] } */
		LIGHT_SPOT_EXPONENT = 30, /**< { uint8 lightIndex = 0, float exponent = 0.0f } */

		TEXTURE1D = 31, /**< { uint8 textureIndex = 0, uint32 textureId = 0 } */ 
		TEXTURE2D, /**< { uint8 textureIndex = 0, uint32 textureId = 0 } */
		TEXTURE3D, /**< { uint8 textureIndex = 0, uint32 textureId = 0 } */
		TEXTURECUBE, /**< { uint8 textureIndex = 0, uint32 textureId = 0 } */
		TEXTURERECT, /**< { uint8 textureIndex = 0, uint32 textureId = 0 } */
		TEXTUREDEPTH, /**< { uint8 textureIndex = 0, uint32 textureId = 0 } */
		TEXTURE1D_ENABLE, /**< { uint8 textureIndex = 0, bool enabled = false } */
		TEXTURE2D_ENABLE, /**< { uint8 textureIndex = 0, bool enabled = false } */
		TEXTURE3D_ENABLE, /**< { uint8 textureIndex = 0, bool enabled = false } */
		TEXTURECUBE_ENABLE, /**< { uint8 textureIndex = 0, bool enabled = false } */
		TEXTURERECT_ENABLE, /**< { uint8 textureIndex = 0, bool enabled = false } */
		TEXTUREDEPTH_ENABLE, /**< { uint8 textureIndex = 0, bool enabled = false } */
		TEXTURE_ENV_COLOR, /**< { uint8 textureIndex = 0, FMVector4 environmentColor = [0,0,0,0] } */
		TEXTURE_ENV_MODE, /**< { uint8 textureIndex = 0, char environmentMode[255] = "" } */

		CLIP_PLANE = 45, /**< { uint8 planeIndex = 0, FMVector4 planeValue = [0,0,0,0] } */
		CLIP_PLANE_ENABLE, /**< { uint8 planeIndex = 0, bool enabled = false } */
		BLEND_COLOR, /**< { FMVector4 blendColor = [0,0,0,0] } */
		CLEAR_COLOR, /**< { FMVector4 clearColor = [0,0,0,0] } */
		CLEAR_STENCIL, /**< { uint32 clearStencilValue = 0 } */
		CLEAR_DEPTH, /**< { float clearDepthValue = 1.0f } */
		COLOR_MASK, /**< { bool redWriteMask = true, bool greenWriteMask = true, bool blueWriteMask = true, bool alphaWriteMask = true } */
		DEPTH_BOUNDS, /**< { FMVector2 depthBounds = [0,1] } */
		DEPTH_MASK, /**< { bool depthWriteMask = true } */
		DEPTH_RANGE, /**< { float minimumDepthValue = 0.0f, float maximumDepthValue = 1.0f } */

		FOG_DENSITY = 55, /**< { float fogDensity = 1.0f } */
		FOG_START, /**< { float fogStartDepthValue = 0.0f } */
		FOG_END, /**< { float fogEndDepthValue = 1.0f } */
		FOG_COLOR, /**< { FMVector4 fogColor = [0,0,0,0] } */
		LIGHT_MODEL_AMBIENT, /**< { FMVector4 ambientColor = [0.2,0.2,0.2,1] } */
		LIGHTING_ENABLE, /**< { bool enabled = false } */
		LINE_STIPPLE, /**< { uint16 lineStippleStart = 1, uint16 lineStippleEnd = 0xFF } */
		LINE_WIDTH, /**< { float lineWidth = 1.0f } */

		MATERIAL_AMBIENT = 63, /**< { FMVector4 ambientColor = [0.2,0.2,0.2,1] } */
		MATERIAL_DIFFUSE, /**< { FMVector4 diffuseColor = [0.8,0.8,0.8,1] } */
		MATERIAL_EMISSION, /**< { FMVector4 emissionColor = [0,0,0,1] } */
		MATERIAL_SHININESS, /**< { float shininessOrSpecularLevel = 0.0f } */
		MATERIAL_SPECULAR, /**< { FMVector4 specularColor = [0,0,0,1] } */
		MODEL_VIEW_MATRIX, /**< { FMMatrix44 modelViewMatrix = FMMatrix44::Identity } */
		POINT_DISTANCE_ATTENUATION, /**< { FMVector3 attenuation = [1,0,0] } */
		POINT_FADE_THRESHOLD_SIZE, /**< { float threshold = 1.0f } */
		POINT_SIZE, /**< { float size = 1.0f } */
		POINT_SIZE_MIN, /**< { float minimum = 0.0f } */
		POINT_SIZE_MAX, /**< { float maximum = 1.0f } */

		POLYGON_OFFSET = 74, /**< { float factor = 0.0f, float units = 0.0f } */
		PROJECTION_MATRIX, /**< { FMMatrix44 projectionMatrix = FMMatrix44::Identity } */
		SCISSOR, /**< { FMVector4 scissor = [0,0,0,0] } */
		STENCIL_MASK, /**< { uint32 mask = 0xFFFFFFFF } */
		ALPHA_TEST_ENABLE, /**< { bool enabled = false } */
		AUTO_NORMAL_ENABLE, /**< { bool enabled  = false } */
		BLEND_ENABLE, /**< { bool enabled  = false } */
		COLOR_LOGIC_OP_ENABLE, /**< { bool enabled  = false } */
		COLOR_MATERIAL_ENABLE, /**< { bool enabled  = true } */
		CULL_FACE_ENABLE, /**< { bool enabled  = false } */

		DEPTH_BOUNDS_ENABLE = 84, /**< { bool enabled  = false } */
		DEPTH_CLAMP_ENABLE, /**< { bool enabled  = false } */
		DEPTH_TEST_ENABLE, /**< { bool enabled  = false } */
		DITHER_ENABLE, /**< { bool enabled  = false } */
		FOG_ENABLE, /**< { bool enabled  = false } */
		LIGHT_MODEL_LOCAL_VIEWER_ENABLE, /**< { bool enabled  = false } */
		LIGHT_MODEL_TWO_SIDE_ENABLE, /**< { bool enabled  = false } */
		LINE_SMOOTH_ENABLE, /**< { bool enabled  = false } */
		LINE_STIPPLE_ENABLE, /**< { bool enabled = false } */
		LOGIC_OP_ENABLE, /**< { bool enabled  = false } */
		MULTISAMPLE_ENABLE, /**< { bool enabled  = false } */
		
		NORMALIZE_ENABLE = 95, /**< { bool enabled  = false } */
		POINT_SMOOTH_ENABLE, /**< { bool enabled  = false } */
		POLYGON_OFFSET_FILL_ENABLE, /**< { bool enabled  = false } */
		POLYGON_OFFSET_LINE_ENABLE, /**< { bool enabled  = false } */
		POLYGON_OFFSET_POINT_ENABLE, /**< { bool enabled  = false } */
		POLYGON_SMOOTH_ENABLE, /**< { bool enabled  = false } */
		POLYGON_STIPPLE_ENABLE, /**< { bool enabled  = false } */
		RESCALE_NORMAL_ENABLE, /**< { bool enabled  = false } */

		SAMPLE_ALPHA_TO_COVERAGE_ENABLE = 103, /**< { bool enabled  = false } */
		SAMPLE_ALPHA_TO_ONE_ENABLE, /**< { bool enabled  = false } */
		SAMPLE_COVERAGE_ENABLE, /**< { bool enabled  = false } */
		SCISSOR_TEST_ENABLE, /**< { bool enabled  = false } */
		STENCIL_TEST_ENABLE, /**< { bool enabled  = false } */

		COUNT = 108,
		INVALID
	};

	/** Converts the COLLADA render state name string to the render state enumeration type.
		@param value The render state name string.
		@return The render state enumeration type. */
	FCOLLADA_EXPORT State FromString(const char* value);
	inline State FromString(const fm::string& value) { return FromString(value.c_str()); } /**< See above. */

	/** Converts the render state enumeration type to its COLLADA render state name string.
		@param state The render state enumeration type.
		@return The render state name string. */
	FCOLLADA_EXPORT const char* ToString(State state);
};

#endif // _FU_DAE_ENUM_H_

