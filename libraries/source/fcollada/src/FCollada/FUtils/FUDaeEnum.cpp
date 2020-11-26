/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUDaeEnum.h"
#include "FUDaeEnumSyntax.h"
#include "FUDaeSyntax.h"

namespace FUDaeAccessor
{
	const char* XY[3] = { "X", "Y", 0 };
	const char* XYZW[5] = { "X", "Y", "Z", "W", 0 };
	const char* RGBA[5] = { "R", "G", "B", "A", 0 };
	const char* STPQ[5] = { "S", "T", "P", "Q", 0 };
};

namespace FUDaeInterpolation
{
	Interpolation FromString(const fm::string& value)
	{
		if (value == DAE_STEP_INTERPOLATION) return STEP;
		else if (value == DAE_LINEAR_INTERPOLATION) return LINEAR;
		else if (value == DAE_BEZIER_INTERPOLATION) return BEZIER;
		else if (value == DAE_TCB_INTERPOLATION) return TCB;
		else if (value.empty()) return BEZIER; // COLLADA 1.4.1, p4.92: application defined

		else return UNKNOWN;
	}

	const char* ToString(const Interpolation& value)
	{
		switch (value)
		{
		case STEP: return DAE_STEP_INTERPOLATION;
		case LINEAR: return DAE_LINEAR_INTERPOLATION;
		case BEZIER: return DAE_BEZIER_INTERPOLATION;
		case TCB: return DAE_TCB_INTERPOLATION;
		case UNKNOWN:
		default: return DAEERR_UNKNOWN_ELEMENT;
		}
	}
};

namespace FUDaeSplineType
{
	Type FromString(const fm::string& value)
	{
		if (IsEquivalent(value, DAE_LINEAR_SPLINE_TYPE)) return LINEAR;
		else if (IsEquivalent(value, DAE_BEZIER_SPLINE_TYPE)) return BEZIER;
		else if (IsEquivalent(value, DAE_NURBS_SPLINE_TYPE)) return NURBS;
		else return UNKNOWN;
	}

	const char* ToString(const Type& value)
	{
		switch (value)
		{
		case LINEAR: return DAE_LINEAR_SPLINE_TYPE;
		case BEZIER: return DAE_BEZIER_SPLINE_TYPE;
		case NURBS: return DAE_NURBS_SPLINE_TYPE;
		default: return DAE_UNKNOWN_SPLINE_TYPE;
		}
	}
};

namespace FUDaeSplineForm
{
	Form FromString(const fm::string& value)
	{
		if (IsEquivalent(value, DAE_OPEN_SPLINE_FORM)) return OPEN;
		else if (IsEquivalent(value, DAE_CLOSED_SPLINE_FORM)) return CLOSED;
		else return UNKNOWN;
	}

	const char* ToString(const Form& value)
	{
		switch (value)
		{
		case OPEN: return DAE_OPEN_SPLINE_FORM;
		case CLOSED: return DAE_CLOSED_SPLINE_FORM;
		default: return DAE_UNKNOWN_SPLINE_FORM;
		}
	}
};


namespace FUDaeTextureChannel
{
	Channel FromString(const fm::string& value)
	{
		if (value == DAE_AMBIENT_TEXTURE_CHANNEL) return AMBIENT;
		else if (value == DAE_BUMP_TEXTURE_CHANNEL) return BUMP;
		else if (value == DAE_DIFFUSE_TEXTURE_CHANNEL) return DIFFUSE;
		else if (value == DAE_DISPLACEMENT_TEXTURE_CHANNEL) return DISPLACEMENT;
		else if (value == DAE_EMISSION_TEXTURE_CHANNEL) return EMISSION;
		else if (value == DAE_FILTER_TEXTURE_CHANNEL) return FILTER;
//		else if (value == DAE_OPACITY_TEXTURE_CHANNEL) return OPACITY;
		else if (value == DAE_REFLECTION_TEXTURE_CHANNEL) return REFLECTION;
		else if (value == DAE_REFRACTION_TEXTURE_CHANNEL) return REFRACTION;
		else if (value == DAE_SHININESS_TEXTURE_CHANNEL) return SHININESS;
		else if (value == DAE_SPECULAR_TEXTURE_CHANNEL) return SPECULAR;
		else if (value == DAE_SPECULARLEVEL_TEXTURE_CHANNEL) return SPECULAR_LEVEL;
		else if (value == DAE_TRANSPARENT_TEXTURE_CHANNEL) return TRANSPARENT;
		else return UNKNOWN;
	}
};

namespace FUDaeTextureWrapMode
{
	WrapMode FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_TEXTURE_WRAP_NONE)) return NONE;
		else if (IsEquivalent(value, DAE_TEXTURE_WRAP_WRAP)) return WRAP;
		else if (IsEquivalent(value, DAE_TEXTURE_WRAP_MIRROR)) return MIRROR;
		else if (IsEquivalent(value, DAE_TEXTURE_WRAP_CLAMP)) return CLAMP;
		else if (IsEquivalent(value, DAE_TEXTURE_WRAP_BORDER)) return BORDER;
		else return UNKNOWN;
	}
	
	const char* ToString(WrapMode wrap)
	{
		switch (wrap)
		{
		case NONE: return DAE_TEXTURE_WRAP_NONE;
		case WRAP: return DAE_TEXTURE_WRAP_WRAP;
		case MIRROR: return DAE_TEXTURE_WRAP_MIRROR;
		case CLAMP: return DAE_TEXTURE_WRAP_CLAMP;
		case BORDER: return DAE_TEXTURE_WRAP_BORDER;
		default: return DAE_TEXTURE_WRAP_UNKNOWN;
		}
	}
};

/** Contains the texture filter functions.*/
namespace FUDaeTextureFilterFunction
{
	FilterFunction FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_TEXTURE_FILTER_NONE)) return NONE;
		else if (IsEquivalent(value, DAE_TEXTURE_FILTER_NEAREST)) return NEAREST;
		else if (IsEquivalent(value, DAE_TEXTURE_FILTER_LINEAR)) return LINEAR;
		else if (IsEquivalent(value, DAE_TEXTURE_FILTER_NEAR_MIP_NEAR)) return NEAREST_MIPMAP_NEAREST;
		else if (IsEquivalent(value, DAE_TEXTURE_FILTER_LIN_MIP_NEAR)) return LINEAR_MIPMAP_NEAREST;
		else if (IsEquivalent(value, DAE_TEXTURE_FILTER_NEAR_MIP_LIN)) return NEAREST_MIPMAP_LINEAR;
		else if (IsEquivalent(value, DAE_TEXTURE_FILTER_LIN_MIP_LIN)) return LINEAR_MIPMAP_LINEAR;
		else return UNKNOWN;
	}

	const char* ToString(FilterFunction function)
	{
		switch (function)
		{
		case NONE: return DAE_TEXTURE_FILTER_NONE;
		case NEAREST: return DAE_TEXTURE_FILTER_NEAREST;
		case NEAREST_MIPMAP_NEAREST: return DAE_TEXTURE_FILTER_NEAR_MIP_NEAR;
		case LINEAR_MIPMAP_NEAREST: return DAE_TEXTURE_FILTER_LIN_MIP_NEAR;
		case NEAREST_MIPMAP_LINEAR: return DAE_TEXTURE_FILTER_NEAR_MIP_LIN;
		case LINEAR_MIPMAP_LINEAR: return DAE_TEXTURE_FILTER_LIN_MIP_LIN;
		default: return DAE_TEXTURE_FILTER_UNKNOWN;
		}
	}
};

namespace FUDaeMorphMethod
{
	Method FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_NORMALIZED_MORPH_METHOD)) return NORMALIZED;
		else if (IsEquivalent(value, DAE_RELATIVE_MORPH_METHOD)) return RELATIVE;
		else return DEFAULT;
	}

	const char* ToString(Method method)
	{
		switch (method)
		{
		case NORMALIZED: return DAE_NORMALIZED_MORPH_METHOD;
		case RELATIVE: return DAE_RELATIVE_MORPH_METHOD;
		case UNKNOWN:
		default: return DAEERR_UNKNOWN_MORPH_METHOD;
		}
	}
};

namespace FUDaeInfinity
{
	Infinity FromString(const char* value)
	{
		if (IsEquivalent(value, DAEMAYA_CONSTANT_INFINITY)) return CONSTANT;
		else if (IsEquivalent(value, DAEMAYA_LINEAR_INFINITY)) return LINEAR;
		else if (IsEquivalent(value, DAEMAYA_CYCLE_INFINITY)) return CYCLE;
		else if (IsEquivalent(value, DAEMAYA_CYCLE_RELATIVE_INFINITY)) return CYCLE_RELATIVE;
		else if (IsEquivalent(value, DAEMAYA_OSCILLATE_INFINITY)) return OSCILLATE;
		else return DEFAULT;
	}

	const char* ToString(Infinity type)
	{
		switch (type)
		{
		case CONSTANT: return DAEMAYA_CONSTANT_INFINITY;
		case LINEAR: return DAEMAYA_LINEAR_INFINITY;
		case CYCLE: return DAEMAYA_CYCLE_INFINITY;
		case CYCLE_RELATIVE: return DAEMAYA_CYCLE_RELATIVE_INFINITY;
		case OSCILLATE: return DAEMAYA_OSCILLATE_INFINITY;
		default: return DAEMAYA_CONSTANT_INFINITY;
		}
	}
};

namespace FUDaeBlendMode
{
	Mode FromString(const char* value)
	{
		if (IsEquivalent(value, DAEMAYA_NONE_BLENDMODE)) return NONE;
		else if (IsEquivalent(value, DAEMAYA_OVER_BLENDMODE)) return OVER;
		else if (IsEquivalent(value, DAEMAYA_IN_BLENDMODE)) return IN;
		else if (IsEquivalent(value, DAEMAYA_OUT_BLENDMODE)) return OUT;
		else if (IsEquivalent(value, DAEMAYA_ADD_BLENDMODE)) return ADD;
		else if (IsEquivalent(value, DAEMAYA_SUBTRACT_BLENDMODE)) return SUBTRACT;
		else if (IsEquivalent(value, DAEMAYA_MULTIPLY_BLENDMODE)) return MULTIPLY;
		else if (IsEquivalent(value, DAEMAYA_DIFFERENCE_BLENDMODE)) return DIFFERENCE;
		else if (IsEquivalent(value, DAEMAYA_LIGHTEN_BLENDMODE)) return LIGHTEN;
		else if (IsEquivalent(value, DAEMAYA_DARKEN_BLENDMODE)) return DARKEN;
		else if (IsEquivalent(value, DAEMAYA_SATURATE_BLENDMODE)) return SATURATE;
		else if (IsEquivalent(value, DAEMAYA_DESATURATE_BLENDMODE)) return DESATURATE;
		else if (IsEquivalent(value, DAEMAYA_ILLUMINATE_BLENDMODE)) return ILLUMINATE;
		else return UNKNOWN;
	}

	const char* ToString(Mode mode)
	{
		switch (mode)
		{
		case NONE: return DAEMAYA_NONE_BLENDMODE;
		case OVER: return DAEMAYA_OVER_BLENDMODE;
		case IN: return DAEMAYA_IN_BLENDMODE;
		case OUT: return DAEMAYA_OUT_BLENDMODE;
		case ADD: return DAEMAYA_ADD_BLENDMODE;
		case SUBTRACT: return DAEMAYA_SUBTRACT_BLENDMODE;
		case MULTIPLY: return DAEMAYA_MULTIPLY_BLENDMODE;
		case DIFFERENCE: return DAEMAYA_DIFFERENCE_BLENDMODE;
		case LIGHTEN: return DAEMAYA_LIGHTEN_BLENDMODE;
		case DARKEN: return DAEMAYA_DARKEN_BLENDMODE;
		case SATURATE: return DAEMAYA_SATURATE_BLENDMODE;
		case DESATURATE: return DAEMAYA_DESATURATE_BLENDMODE;
		case ILLUMINATE: return DAEMAYA_ILLUMINATE_BLENDMODE;
		default: return DAEMAYA_NONE_BLENDMODE;
		}
	}
}

namespace FUDaeGeometryInput
{
	Semantic FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_POSITION_INPUT)) return POSITION;
		else if (IsEquivalent(value, DAE_VERTEX_INPUT)) return VERTEX;
		else if (IsEquivalent(value, DAE_NORMAL_INPUT)) return NORMAL;
		else if (IsEquivalent(value, DAE_GEOTANGENT_INPUT)) return GEOTANGENT;
		else if (IsEquivalent(value, DAE_GEOBINORMAL_INPUT)) return GEOBINORMAL;
		else if (IsEquivalent(value, DAE_TEXCOORD_INPUT)) return TEXCOORD;
		else if (IsEquivalent(value, DAE_TEXTANGENT_INPUT)) return TEXTANGENT;
		else if (IsEquivalent(value, DAE_TEXBINORMAL_INPUT)) return TEXBINORMAL;
		else if (IsEquivalent(value, DAE_MAPPING_INPUT)) return UV;
		else if (IsEquivalent(value, DAE_COLOR_INPUT)) return COLOR;
		else if (IsEquivalent(value, "POINT_SIZE")) return POINT_SIZE;
		else if (IsEquivalent(value, "POINT_ROT")) return POINT_ROTATION;
		else if (IsEquivalent(value, DAEMAYA_EXTRA_INPUT)) return EXTRA;
		else return UNKNOWN;
	}

    const char* ToString(Semantic semantic)
	{
		switch (semantic)
		{
		case POSITION: return DAE_POSITION_INPUT;
		case VERTEX: return DAE_VERTEX_INPUT;
		case NORMAL: return DAE_NORMAL_INPUT;
		case GEOTANGENT: return DAE_GEOTANGENT_INPUT;
		case GEOBINORMAL: return DAE_GEOBINORMAL_INPUT;
		case TEXCOORD: return DAE_TEXCOORD_INPUT;
		case TEXTANGENT: return DAE_TEXTANGENT_INPUT;
		case TEXBINORMAL: return DAE_TEXBINORMAL_INPUT;
		case UV: return DAE_MAPPING_INPUT;
		case COLOR: return DAE_COLOR_INPUT;
		case POINT_SIZE: return "POINT_SIZE";
		case POINT_ROTATION: return "POINT_ROT";
		case EXTRA: return DAEMAYA_EXTRA_INPUT;

		case UNKNOWN:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
}

namespace FUDaeProfileType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_PROFILE_COMMON_ELEMENT)) return COMMON;
		else if (IsEquivalent(value, DAE_FX_PROFILE_CG_ELEMENT)) return CG;
		else if (IsEquivalent(value, DAE_FX_PROFILE_HLSL_ELEMENT)) return HLSL;
		else if (IsEquivalent(value, DAE_FX_PROFILE_GLSL_ELEMENT)) return GLSL;
		else if (IsEquivalent(value, DAE_FX_PROFILE_GLES_ELEMENT)) return GLES;
		else return UNKNOWN;
	}

    const char* ToString(Type type)
	{
		switch (type)
		{
		case COMMON: return DAE_FX_PROFILE_COMMON_ELEMENT;
		case CG: return DAE_FX_PROFILE_CG_ELEMENT;
		case HLSL: return DAE_FX_PROFILE_HLSL_ELEMENT;
		case GLSL: return DAE_FX_PROFILE_GLSL_ELEMENT;
		case GLES: return DAE_FX_PROFILE_GLES_ELEMENT;

		case UNKNOWN:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
}

namespace FUDaePassStateFunction
{
	Function FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_FUNCTION_NEVER)) return NEVER;
		else if (IsEquivalent(value, DAE_FX_FUNCTION_LESS)) return LESS;
		else if (IsEquivalent(value, DAE_FX_FUNCTION_EQUAL)) return EQUAL;
		else if (IsEquivalent(value, DAE_FX_FUNCTION_LEQUAL)) return LESS_EQUAL;
		else if (IsEquivalent(value, DAE_FX_FUNCTION_GREATER)) return GREATER;
		else if (IsEquivalent(value, DAE_FX_FUNCTION_NEQUAL)) return NOT_EQUAL;
		else if (IsEquivalent(value, DAE_FX_FUNCTION_GEQUAL)) return GREATER_EQUAL;
		else if (IsEquivalent(value, DAE_FX_FUNCTION_ALWAYS)) return ALWAYS;
		else return INVALID;
	}

	const char* ToString(Function fn)
	{
		switch (fn)
		{
		case NEVER: return DAE_FX_FUNCTION_NEVER;
		case LESS: return DAE_FX_FUNCTION_LESS;
		case EQUAL: return DAE_FX_FUNCTION_EQUAL;
		case LESS_EQUAL: return DAE_FX_FUNCTION_LEQUAL;
		case GREATER: return DAE_FX_FUNCTION_GREATER;
		case NOT_EQUAL: return DAE_FX_FUNCTION_NEQUAL;
		case GREATER_EQUAL: return DAE_FX_FUNCTION_GEQUAL;
		case ALWAYS: return DAE_FX_FUNCTION_ALWAYS;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassStateStencilOperation
{
	Operation FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_KEEP)) return KEEP;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_ZERO)) return ZERO;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_REPLACE)) return REPLACE;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_INCREMENT)) return INCREMENT;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_DECREMENT)) return DECREMENT;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_INVERT)) return INVERT;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_INCREMENT_WRAP)) return INCREMENT_WRAP;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCILOP_DECREMENT_WRAP)) return DECREMENT_WRAP;
		else return INVALID;
	}

	const char* ToString(Operation op)
	{
		switch (op)
		{
		case KEEP: return DAE_FX_STATE_STENCILOP_KEEP;
		case ZERO: return DAE_FX_STATE_STENCILOP_ZERO;
		case REPLACE: return DAE_FX_STATE_STENCILOP_REPLACE;
		case INCREMENT: return DAE_FX_STATE_STENCILOP_INCREMENT;
		case DECREMENT: return DAE_FX_STATE_STENCILOP_DECREMENT;
		case INVERT: return DAE_FX_STATE_STENCILOP_INVERT;
		case INCREMENT_WRAP: return DAE_FX_STATE_STENCILOP_INCREMENT_WRAP;
		case DECREMENT_WRAP: return DAE_FX_STATE_STENCILOP_DECREMENT_WRAP;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassStateBlendType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ZERO)) return ZERO;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ONE)) return ONE;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_SOURCE_COLOR)) return SOURCE_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ONE_MINUS_SOURCE_COLOR)) return ONE_MINUS_SOURCE_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_DESTINATION_COLOR)) return DESTINATION_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ONE_MINUS_DESTINATION_COLOR)) return ONE_MINUS_DESTINATION_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_SOURCE_ALPHA)) return SOURCE_ALPHA;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ONE_MINUS_SOURCE_ALPHA)) return ONE_MINUS_SOURCE_ALPHA;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_DESTINATION_ALPHA)) return DESTINATION_ALPHA;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ONE_MINUS_DESTINATION_ALPHA)) return ONE_MINUS_DESTINATION_ALPHA;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_CONSTANT_COLOR)) return CONSTANT_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ONE_MINUS_CONSTANT_COLOR)) return ONE_MINUS_CONSTANT_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_CONSTANT_ALPHA)) return CONSTANT_ALPHA;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_ONE_MINUS_CONSTANT_ALPHA)) return ONE_MINUS_CONSTANT_ALPHA;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDTYPE_SOURCE_ALPHA_SATURATE)) return SOURCE_ALPHA_SATURATE;
		else return INVALID;
	}

	const char* ToString(Type type)
	{
		switch (type)
		{
		case ZERO: return DAE_FX_STATE_BLENDTYPE_ZERO;
		case ONE: return DAE_FX_STATE_BLENDTYPE_ONE;
		case SOURCE_COLOR: return DAE_FX_STATE_BLENDTYPE_SOURCE_COLOR;
		case ONE_MINUS_SOURCE_COLOR: return DAE_FX_STATE_BLENDTYPE_ONE_MINUS_SOURCE_COLOR;
		case DESTINATION_COLOR: return DAE_FX_STATE_BLENDTYPE_DESTINATION_COLOR;
		case ONE_MINUS_DESTINATION_COLOR: return DAE_FX_STATE_BLENDTYPE_ONE_MINUS_DESTINATION_COLOR;
		case SOURCE_ALPHA: return DAE_FX_STATE_BLENDTYPE_SOURCE_ALPHA;
		case ONE_MINUS_SOURCE_ALPHA: return DAE_FX_STATE_BLENDTYPE_ONE_MINUS_SOURCE_ALPHA;
		case DESTINATION_ALPHA: return DAE_FX_STATE_BLENDTYPE_DESTINATION_ALPHA;
		case ONE_MINUS_DESTINATION_ALPHA: return DAE_FX_STATE_BLENDTYPE_ONE_MINUS_DESTINATION_ALPHA;
		case CONSTANT_COLOR: return DAE_FX_STATE_BLENDTYPE_CONSTANT_COLOR;
		case ONE_MINUS_CONSTANT_COLOR: return DAE_FX_STATE_BLENDTYPE_ONE_MINUS_CONSTANT_COLOR;
		case CONSTANT_ALPHA: return DAE_FX_STATE_BLENDTYPE_CONSTANT_ALPHA;
		case ONE_MINUS_CONSTANT_ALPHA: return DAE_FX_STATE_BLENDTYPE_ONE_MINUS_CONSTANT_ALPHA;
		case SOURCE_ALPHA_SATURATE: return DAE_FX_STATE_BLENDTYPE_SOURCE_ALPHA_SATURATE;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

/** The render state face types. */
namespace FUDaePassStateFaceType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_FACETYPE_FRONT)) return FRONT;
		else if (IsEquivalent(value, DAE_FX_STATE_FACETYPE_BACK)) return BACK;
		else if (IsEquivalent(value, DAE_FX_STATE_FACETYPE_FRONT_AND_BACK)) return FRONT_AND_BACK;
		else return INVALID;
	}

	const char* ToString(Type type)
	{
		switch (type)
		{
		case FRONT: return DAE_FX_STATE_FACETYPE_FRONT;
		case BACK: return DAE_FX_STATE_FACETYPE_BACK;
		case FRONT_AND_BACK: return DAE_FX_STATE_FACETYPE_FRONT_AND_BACK;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassStateBlendEquation
{
	Equation FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_BLENDEQ_ADD)) return ADD;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDEQ_SUBTRACT)) return SUBTRACT;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDEQ_REVERSE_SUBTRACT)) return REVERSE_SUBTRACT;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDEQ_MIN)) return MIN;
		else if (IsEquivalent(value, DAE_FX_STATE_BLENDEQ_MAX)) return MAX;
		else return INVALID;
	}

	const char* ToString(Equation equation)
	{
		switch (equation)
		{
		case ADD: return DAE_FX_STATE_BLENDEQ_ADD;
		case SUBTRACT: return DAE_FX_STATE_BLENDEQ_SUBTRACT;
		case REVERSE_SUBTRACT: return DAE_FX_STATE_BLENDEQ_REVERSE_SUBTRACT;
		case MIN: return DAE_FX_STATE_BLENDEQ_MIN;
		case MAX: return DAE_FX_STATE_BLENDEQ_MAX;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassStateMaterialType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_MATERIALTYPE_EMISSION)) return EMISSION;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIALTYPE_AMBIENT)) return AMBIENT;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIALTYPE_DIFFUSE)) return DIFFUSE;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIALTYPE_SPECULAR)) return SPECULAR;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIALTYPE_AMBDIFF)) return AMBIENT_AND_DIFFUSE;
		else return INVALID;
	}

	const char* ToString(Type type)
	{
		switch (type)
		{
		case EMISSION: return DAE_FX_STATE_MATERIALTYPE_EMISSION;
		case AMBIENT: return DAE_FX_STATE_MATERIALTYPE_AMBIENT;
		case DIFFUSE: return DAE_FX_STATE_MATERIALTYPE_DIFFUSE;
		case SPECULAR: return DAE_FX_STATE_MATERIALTYPE_SPECULAR;
		case AMBIENT_AND_DIFFUSE: return DAE_FX_STATE_MATERIALTYPE_AMBDIFF;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassStateFogType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_FOGTYPE_LINEAR)) return LINEAR;
		else if (IsEquivalent(value, DAE_FX_STATE_FOGTYPE_EXP)) return EXP;
		else if (IsEquivalent(value, DAE_FX_STATE_FOGTYPE_EXP2)) return EXP2;
		else return INVALID;
	}
	
	const char* ToString(Type type)
	{
		switch (type)
		{
		case LINEAR: return DAE_FX_STATE_FOGTYPE_LINEAR;
		case EXP: return DAE_FX_STATE_FOGTYPE_EXP;
		case EXP2: return DAE_FX_STATE_FOGTYPE_EXP2;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
}

namespace FUDaePassStateFogCoordinateType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_FOGCOORD_FOG_COORDINATE)) return FOG_COORDINATE;
		else if (IsEquivalent(value, DAE_FX_STATE_FOGCOORD_FRAGMENT_DEPTH)) return FRAGMENT_DEPTH;
		else return INVALID;
	}

	const char* ToString(Type type)
	{
		switch (type)
		{
		case FOG_COORDINATE: return DAE_FX_STATE_FOGCOORD_FOG_COORDINATE;
		case FRAGMENT_DEPTH: return DAE_FX_STATE_FOGCOORD_FRAGMENT_DEPTH;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassStateFrontFaceType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_FFACE_CW)) return CLOCKWISE;
		else if (IsEquivalent(value, DAE_FX_STATE_FFACE_CCW)) return COUNTER_CLOCKWISE;
		else return INVALID;
	}

	const char* ToString(Type type)
	{
		switch (type)
		{
		case CLOCKWISE: return DAE_FX_STATE_FFACE_CW;
		case COUNTER_CLOCKWISE: return DAE_FX_STATE_FFACE_CCW;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};


/** The render state logic operations. */
namespace FUDaePassStateLogicOperation
{
	Operation FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_CLEAR)) return CLEAR;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_AND)) return AND;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_AND_REVERSE)) return AND_REVERSE;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_COPY)) return COPY;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_AND_INVERTED)) return AND_INVERTED;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_NOOP)) return NOOP;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_XOR)) return XOR;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_OR)) return OR;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_NOR)) return NOR;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_EQUIV)) return EQUIV;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_INVERT)) return INVERT;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_OR_REVERSE)) return OR_REVERSE;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_COPY_INVERTED)) return COPY_INVERTED;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_NAND)) return NAND;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGICOP_SET)) return SET;
		else return INVALID;
	}

	const char* ToString(Operation op)
	{
		switch (op)
		{
		case CLEAR: return DAE_FX_STATE_LOGICOP_CLEAR;
		case AND: return DAE_FX_STATE_LOGICOP_AND;
		case AND_REVERSE: return DAE_FX_STATE_LOGICOP_AND_REVERSE;
		case COPY: return DAE_FX_STATE_LOGICOP_COPY;
		case AND_INVERTED: return DAE_FX_STATE_LOGICOP_AND_INVERTED;
		case NOOP: return DAE_FX_STATE_LOGICOP_NOOP;
		case XOR: return DAE_FX_STATE_LOGICOP_XOR;
		case OR: return DAE_FX_STATE_LOGICOP_OR;
		case NOR: return DAE_FX_STATE_LOGICOP_NOR;
		case EQUIV: return DAE_FX_STATE_LOGICOP_EQUIV;
		case INVERT: return DAE_FX_STATE_LOGICOP_INVERT;
		case OR_REVERSE: return DAE_FX_STATE_LOGICOP_OR_REVERSE;
		case COPY_INVERTED: return DAE_FX_STATE_LOGICOP_COPY_INVERTED;
		case NAND: return DAE_FX_STATE_LOGICOP_NAND;
		case SET: return DAE_FX_STATE_LOGICOP_SET;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

/** The render state polygon modes. */
namespace FUDaePassStatePolygonMode
{
	Mode FromString(const char* value)
	{
		if (IsEquivalent(value,	DAE_FX_STATE_POLYMODE_POINT)) return POINT;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYMODE_LINE)) return LINE;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYMODE_FILL)) return FILL;
		else return INVALID;
	}

	const char* ToString(Mode mode)
	{
		switch (mode)
		{
		case POINT: return DAE_FX_STATE_POLYMODE_POINT;
		case LINE: return DAE_FX_STATE_POLYMODE_LINE;
		case FILL: return DAE_FX_STATE_POLYMODE_FILL;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

/** The render state shading model. */
namespace FUDaePassStateShadeModel
{
	Model FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_SHADEMODEL_FLAT)) return FLAT;
		else if (IsEquivalent(value, DAE_FX_STATE_SHADEMODEL_SMOOTH)) return SMOOTH;
		else return INVALID;
	}

	const char* ToString(Model model)
	{
		switch (model)
		{
		case FLAT: return DAE_FX_STATE_SHADEMODEL_FLAT;
		case SMOOTH: return DAE_FX_STATE_SHADEMODEL_SMOOTH;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassStateLightModelColorControlType
{
	Type FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_LMCCT_SINGLE_COLOR)) return SINGLE_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_LMCCT_SEPARATE_SPECULAR_COLOR)) return SEPARATE_SPECULAR_COLOR;
		else return INVALID;
	}

	const char* ToString(Type type)
	{
		switch (type)
		{
		case SINGLE_COLOR: return DAE_FX_STATE_LMCCT_SINGLE_COLOR;
		case SEPARATE_SPECULAR_COLOR: return DAE_FX_STATE_LMCCT_SEPARATE_SPECULAR_COLOR;
		case INVALID:
		default: return DAEERR_UNKNOWN_INPUT;
		}
	}
};

namespace FUDaePassState
{
	State FromString(const char* value)
	{
		if (IsEquivalent(value, DAE_FX_STATE_ALPHA_FUNC)) return ALPHA_FUNC;
		else if (IsEquivalent(value, DAE_FX_STATE_BLEND_FUNC)) return BLEND_FUNC;
		else if (IsEquivalent(value, DAE_FX_STATE_BLEND_FUNC_SEPARATE)) return BLEND_FUNC_SEPARATE;
		else if (IsEquivalent(value, DAE_FX_STATE_BLEND_EQUATION)) return BLEND_EQUATION;
		else if (IsEquivalent(value, DAE_FX_STATE_BLEND_EQUATION_SEPARATE)) return BLEND_EQUATION_SEPARATE;
		else if (IsEquivalent(value, DAE_FX_STATE_COLOR_MATERIAL)) return COLOR_MATERIAL;
		else if (IsEquivalent(value, DAE_FX_STATE_CULL_FACE)) return CULL_FACE;
		else if (IsEquivalent(value, DAE_FX_STATE_DEPTH_FUNC)) return DEPTH_FUNC;
		else if (IsEquivalent(value, DAE_FX_STATE_FOG_MODE)) return FOG_MODE;
		else if (IsEquivalent(value, DAE_FX_STATE_FOG_COORD_SRC)) return FOG_COORD_SRC;
		else if (IsEquivalent(value, DAE_FX_STATE_FRONT_FACE)) return FRONT_FACE;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_MODEL_COLOR_CONTROL)) return LIGHT_MODEL_COLOR_CONTROL;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGIC_OP)) return LOGIC_OP;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYGON_MODE)) return POLYGON_MODE;
		else if (IsEquivalent(value, DAE_FX_STATE_SHADE_MODEL)) return SHADE_MODEL;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCIL_FUNC)) return STENCIL_FUNC;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCIL_OP)) return STENCIL_OP;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCIL_FUNC_SEPARATE)) return STENCIL_FUNC_SEPARATE;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCIL_OP_SEPARATE)) return STENCIL_OP_SEPARATE;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCIL_MASK_SEPARATE)) return STENCIL_MASK_SEPARATE;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_ENABLE)) return LIGHT_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_AMBIENT)) return LIGHT_AMBIENT;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_DIFFUSE)) return LIGHT_DIFFUSE;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_SPECULAR)) return LIGHT_SPECULAR;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_POSITION)) return LIGHT_POSITION;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_CONSTANT_ATTENUATION)) return LIGHT_CONSTANT_ATTENUATION;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_LINEAR_ATTENUATION)) return LIGHT_LINEAR_ATTENUATION;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_QUADRATIC_ATTENUATION)) return LIGHT_QUADRATIC_ATTENUATION;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_SPOT_CUTOFF)) return LIGHT_SPOT_CUTOFF;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_SPOT_DIRECTION)) return LIGHT_SPOT_DIRECTION;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_SPOT_EXPONENT)) return LIGHT_SPOT_EXPONENT;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE1D)) return TEXTURE1D;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE2D)) return TEXTURE2D;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE3D)) return TEXTURE3D;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURECUBE)) return TEXTURECUBE;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURERECT)) return TEXTURERECT;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTUREDEPTH)) return TEXTUREDEPTH;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE1D_ENABLE)) return TEXTURE1D_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE2D_ENABLE)) return TEXTURE2D_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE3D_ENABLE)) return TEXTURE3D_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURECUBE_ENABLE)) return TEXTURECUBE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURERECT_ENABLE)) return TEXTURERECT_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTUREDEPTH_ENABLE)) return TEXTUREDEPTH_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE_ENV_COLOR)) return TEXTURE_ENV_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_TEXTURE_ENV_MODE)) return TEXTURE_ENV_MODE;
		else if (IsEquivalent(value, DAE_FX_STATE_CLIP_PLANE)) return CLIP_PLANE;
		else if (IsEquivalent(value, DAE_FX_STATE_CLIP_PLANE_ENABLE)) return CLIP_PLANE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_BLEND_COLOR)) return BLEND_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_CLEAR_COLOR)) return CLEAR_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_CLEAR_STENCIL)) return CLEAR_STENCIL;
		else if (IsEquivalent(value, DAE_FX_STATE_CLEAR_DEPTH)) return CLEAR_DEPTH;
		else if (IsEquivalent(value, DAE_FX_STATE_COLOR_MASK)) return COLOR_MASK;
		else if (IsEquivalent(value, DAE_FX_STATE_DEPTH_BOUNDS)) return DEPTH_BOUNDS;
		else if (IsEquivalent(value, DAE_FX_STATE_DEPTH_MASK)) return DEPTH_MASK;
		else if (IsEquivalent(value, DAE_FX_STATE_DEPTH_RANGE)) return DEPTH_RANGE;
		else if (IsEquivalent(value, DAE_FX_STATE_FOG_DENSITY)) return FOG_DENSITY;
		else if (IsEquivalent(value, DAE_FX_STATE_FOG_START)) return FOG_START;
		else if (IsEquivalent(value, DAE_FX_STATE_FOG_END)) return FOG_END;
		else if (IsEquivalent(value, DAE_FX_STATE_FOG_COLOR)) return FOG_COLOR;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_MODEL_AMBIENT)) return LIGHT_MODEL_AMBIENT;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHTING_ENABLE)) return LIGHTING_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LINE_STIPPLE)) return LINE_STIPPLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LINE_WIDTH)) return LINE_WIDTH;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIAL_AMBIENT)) return MATERIAL_AMBIENT;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIAL_DIFFUSE)) return MATERIAL_DIFFUSE;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIAL_EMISSION)) return MATERIAL_EMISSION;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIAL_SHININESS)) return MATERIAL_SHININESS;
		else if (IsEquivalent(value, DAE_FX_STATE_MATERIAL_SPECULAR)) return MATERIAL_SPECULAR;
		else if (IsEquivalent(value, DAE_FX_STATE_MODEL_VIEW_MATRIX)) return MODEL_VIEW_MATRIX;
		else if (IsEquivalent(value, DAE_FX_STATE_POINT_DISTANCE_ATTENUATION)) return POINT_DISTANCE_ATTENUATION;
		else if (IsEquivalent(value, DAE_FX_STATE_POINT_FADE_THRESHOLD_SIZE)) return POINT_FADE_THRESHOLD_SIZE;
		else if (IsEquivalent(value, DAE_FX_STATE_POINT_SIZE)) return POINT_SIZE;
		else if (IsEquivalent(value, DAE_FX_STATE_POINT_SIZE_MIN)) return POINT_SIZE_MIN;
		else if (IsEquivalent(value, DAE_FX_STATE_POINT_SIZE_MAX)) return POINT_SIZE_MAX;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYGON_OFFSET)) return POLYGON_OFFSET;
		else if (IsEquivalent(value, DAE_FX_STATE_PROJECTION_MATRIX)) return PROJECTION_MATRIX;
		else if (IsEquivalent(value, DAE_FX_STATE_SCISSOR)) return SCISSOR;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCIL_MASK)) return STENCIL_MASK;
		else if (IsEquivalent(value, DAE_FX_STATE_ALPHA_TEST_ENABLE)) return ALPHA_TEST_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_AUTO_NORMAL_ENABLE)) return AUTO_NORMAL_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_BLEND_ENABLE)) return BLEND_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_COLOR_LOGIC_OP_ENABLE)) return COLOR_LOGIC_OP_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_COLOR_MATERIAL_ENABLE)) return COLOR_MATERIAL_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_CULL_FACE_ENABLE)) return CULL_FACE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_DEPTH_BOUNDS_ENABLE)) return DEPTH_BOUNDS_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_DEPTH_CLAMP_ENABLE)) return DEPTH_CLAMP_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_DEPTH_TEST_ENABLE)) return DEPTH_TEST_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_DITHER_ENABLE)) return DITHER_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_FOG_ENABLE)) return FOG_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE)) return LIGHT_MODEL_LOCAL_VIEWER_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE)) return LIGHT_MODEL_TWO_SIDE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LINE_SMOOTH_ENABLE)) return LINE_SMOOTH_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LINE_STIPPLE_ENABLE)) return LINE_STIPPLE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_LOGIC_OP_ENABLE)) return LOGIC_OP_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_MULTISAMPLE_ENABLE)) return MULTISAMPLE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_NORMALIZE_ENABLE)) return NORMALIZE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_POINT_SMOOTH_ENABLE)) return POINT_SMOOTH_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYGON_OFFSET_FILL_ENABLE)) return POLYGON_OFFSET_FILL_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYGON_OFFSET_LINE_ENABLE)) return POLYGON_OFFSET_LINE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYGON_OFFSET_POINT_ENABLE)) return POLYGON_OFFSET_POINT_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYGON_SMOOTH_ENABLE)) return POLYGON_SMOOTH_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_POLYGON_STIPPLE_ENABLE)) return POLYGON_STIPPLE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_RESCALE_NORMAL_ENABLE)) return RESCALE_NORMAL_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE)) return SAMPLE_ALPHA_TO_COVERAGE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE)) return SAMPLE_ALPHA_TO_ONE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_SAMPLE_COVERAGE_ENABLE)) return SAMPLE_COVERAGE_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_SCISSOR_TEST_ENABLE)) return SCISSOR_TEST_ENABLE;
		else if (IsEquivalent(value, DAE_FX_STATE_STENCIL_TEST_ENABLE)) return STENCIL_TEST_ENABLE;
		else return INVALID;
	}

	const char* ToString(State state)
	{
		switch (state)
		{
		case ALPHA_FUNC: return DAE_FX_STATE_ALPHA_FUNC;
		case BLEND_FUNC: return DAE_FX_STATE_BLEND_FUNC;
		case BLEND_FUNC_SEPARATE: return DAE_FX_STATE_BLEND_FUNC_SEPARATE;
		case BLEND_EQUATION: return DAE_FX_STATE_BLEND_EQUATION;
		case BLEND_EQUATION_SEPARATE: return DAE_FX_STATE_BLEND_EQUATION_SEPARATE;
		case COLOR_MATERIAL: return DAE_FX_STATE_COLOR_MATERIAL;
		case CULL_FACE: return DAE_FX_STATE_CULL_FACE;
		case DEPTH_FUNC: return DAE_FX_STATE_DEPTH_FUNC;
		case FOG_MODE: return DAE_FX_STATE_FOG_MODE;
		case FOG_COORD_SRC: return DAE_FX_STATE_FOG_COORD_SRC;
		case FRONT_FACE: return DAE_FX_STATE_FRONT_FACE;
		case LIGHT_MODEL_COLOR_CONTROL: return DAE_FX_STATE_LIGHT_MODEL_COLOR_CONTROL;
		case LOGIC_OP: return DAE_FX_STATE_LOGIC_OP;
		case POLYGON_MODE: return DAE_FX_STATE_POLYGON_MODE;
		case SHADE_MODEL: return DAE_FX_STATE_SHADE_MODEL;
		case STENCIL_FUNC: return DAE_FX_STATE_STENCIL_FUNC;
		case STENCIL_OP: return DAE_FX_STATE_STENCIL_OP;
		case STENCIL_FUNC_SEPARATE: return DAE_FX_STATE_STENCIL_FUNC_SEPARATE;
		case STENCIL_OP_SEPARATE: return DAE_FX_STATE_STENCIL_OP_SEPARATE;
		case STENCIL_MASK_SEPARATE: return DAE_FX_STATE_STENCIL_MASK_SEPARATE;
		case LIGHT_ENABLE: return DAE_FX_STATE_LIGHT_ENABLE;
		case LIGHT_AMBIENT: return DAE_FX_STATE_LIGHT_AMBIENT;
		case LIGHT_DIFFUSE: return DAE_FX_STATE_LIGHT_DIFFUSE;
		case LIGHT_SPECULAR: return DAE_FX_STATE_LIGHT_SPECULAR;
		case LIGHT_POSITION: return DAE_FX_STATE_LIGHT_POSITION;
		case LIGHT_CONSTANT_ATTENUATION: return DAE_FX_STATE_LIGHT_CONSTANT_ATTENUATION;
		case LIGHT_LINEAR_ATTENUATION: return DAE_FX_STATE_LIGHT_LINEAR_ATTENUATION;
		case LIGHT_QUADRATIC_ATTENUATION: return DAE_FX_STATE_LIGHT_QUADRATIC_ATTENUATION;
		case LIGHT_SPOT_CUTOFF: return DAE_FX_STATE_LIGHT_SPOT_CUTOFF;
		case LIGHT_SPOT_DIRECTION: return DAE_FX_STATE_LIGHT_SPOT_DIRECTION;
		case LIGHT_SPOT_EXPONENT: return DAE_FX_STATE_LIGHT_SPOT_EXPONENT;
		case TEXTURE1D: return DAE_FX_STATE_TEXTURE1D;
		case TEXTURE2D: return DAE_FX_STATE_TEXTURE2D;
		case TEXTURE3D: return DAE_FX_STATE_TEXTURE3D;
		case TEXTURECUBE: return DAE_FX_STATE_TEXTURECUBE;
		case TEXTURERECT: return DAE_FX_STATE_TEXTURERECT;
		case TEXTUREDEPTH: return DAE_FX_STATE_TEXTUREDEPTH;
		case TEXTURE1D_ENABLE: return DAE_FX_STATE_TEXTURE1D_ENABLE;
		case TEXTURE2D_ENABLE: return DAE_FX_STATE_TEXTURE2D_ENABLE;
		case TEXTURE3D_ENABLE: return DAE_FX_STATE_TEXTURE3D_ENABLE;
		case TEXTURECUBE_ENABLE: return DAE_FX_STATE_TEXTURECUBE_ENABLE;
		case TEXTURERECT_ENABLE: return DAE_FX_STATE_TEXTURERECT_ENABLE;
		case TEXTUREDEPTH_ENABLE: return DAE_FX_STATE_TEXTUREDEPTH_ENABLE;
		case TEXTURE_ENV_COLOR: return DAE_FX_STATE_TEXTURE_ENV_COLOR;
		case TEXTURE_ENV_MODE: return DAE_FX_STATE_TEXTURE_ENV_MODE;
		case CLIP_PLANE: return DAE_FX_STATE_CLIP_PLANE;
		case CLIP_PLANE_ENABLE: return DAE_FX_STATE_CLIP_PLANE_ENABLE;
		case BLEND_COLOR: return DAE_FX_STATE_BLEND_COLOR;
		case CLEAR_COLOR: return DAE_FX_STATE_CLEAR_COLOR;
		case CLEAR_STENCIL: return DAE_FX_STATE_CLEAR_STENCIL;
		case CLEAR_DEPTH: return DAE_FX_STATE_CLEAR_DEPTH;
		case COLOR_MASK: return DAE_FX_STATE_COLOR_MASK;
		case DEPTH_BOUNDS: return DAE_FX_STATE_DEPTH_BOUNDS;
		case DEPTH_MASK: return DAE_FX_STATE_DEPTH_MASK;
		case DEPTH_RANGE: return DAE_FX_STATE_DEPTH_RANGE;
		case FOG_DENSITY: return DAE_FX_STATE_FOG_DENSITY;
		case FOG_START: return DAE_FX_STATE_FOG_START;
		case FOG_END: return DAE_FX_STATE_FOG_END;
		case FOG_COLOR: return DAE_FX_STATE_FOG_COLOR;
		case LIGHT_MODEL_AMBIENT: return DAE_FX_STATE_LIGHT_MODEL_AMBIENT;
		case LIGHTING_ENABLE: return DAE_FX_STATE_LIGHTING_ENABLE;
		case LINE_STIPPLE: return DAE_FX_STATE_LINE_STIPPLE;
		case LINE_WIDTH: return DAE_FX_STATE_LINE_WIDTH;
		case MATERIAL_AMBIENT: return DAE_FX_STATE_MATERIAL_AMBIENT;
		case MATERIAL_DIFFUSE: return DAE_FX_STATE_MATERIAL_DIFFUSE;
		case MATERIAL_EMISSION: return DAE_FX_STATE_MATERIAL_EMISSION;
		case MATERIAL_SHININESS: return DAE_FX_STATE_MATERIAL_SHININESS;
		case MATERIAL_SPECULAR: return DAE_FX_STATE_MATERIAL_SPECULAR;
		case MODEL_VIEW_MATRIX: return DAE_FX_STATE_MODEL_VIEW_MATRIX;
		case POINT_DISTANCE_ATTENUATION: return DAE_FX_STATE_POINT_DISTANCE_ATTENUATION;
		case POINT_FADE_THRESHOLD_SIZE: return DAE_FX_STATE_POINT_FADE_THRESHOLD_SIZE;
		case POINT_SIZE: return DAE_FX_STATE_POINT_SIZE;
		case POINT_SIZE_MIN: return DAE_FX_STATE_POINT_SIZE_MIN;
		case POINT_SIZE_MAX: return DAE_FX_STATE_POINT_SIZE_MAX;
		case POLYGON_OFFSET: return DAE_FX_STATE_POLYGON_OFFSET;
		case PROJECTION_MATRIX: return DAE_FX_STATE_PROJECTION_MATRIX;
		case SCISSOR: return DAE_FX_STATE_SCISSOR;
		case STENCIL_MASK: return DAE_FX_STATE_STENCIL_MASK;
		case ALPHA_TEST_ENABLE: return DAE_FX_STATE_ALPHA_TEST_ENABLE;
		case AUTO_NORMAL_ENABLE: return DAE_FX_STATE_AUTO_NORMAL_ENABLE;
		case BLEND_ENABLE: return DAE_FX_STATE_BLEND_ENABLE;
		case COLOR_LOGIC_OP_ENABLE: return DAE_FX_STATE_COLOR_LOGIC_OP_ENABLE;
		case COLOR_MATERIAL_ENABLE: return DAE_FX_STATE_COLOR_MATERIAL_ENABLE;
		case CULL_FACE_ENABLE: return DAE_FX_STATE_CULL_FACE_ENABLE;
		case DEPTH_BOUNDS_ENABLE: return DAE_FX_STATE_DEPTH_BOUNDS_ENABLE;
		case DEPTH_CLAMP_ENABLE: return DAE_FX_STATE_DEPTH_CLAMP_ENABLE;
		case DEPTH_TEST_ENABLE: return DAE_FX_STATE_DEPTH_TEST_ENABLE;
		case DITHER_ENABLE: return DAE_FX_STATE_DITHER_ENABLE;
		case FOG_ENABLE: return DAE_FX_STATE_FOG_ENABLE;
		case LIGHT_MODEL_LOCAL_VIEWER_ENABLE: return DAE_FX_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE;
		case LIGHT_MODEL_TWO_SIDE_ENABLE: return DAE_FX_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE;
		case LINE_SMOOTH_ENABLE: return DAE_FX_STATE_LINE_SMOOTH_ENABLE;
		case LINE_STIPPLE_ENABLE: return DAE_FX_STATE_LINE_STIPPLE_ENABLE;
		case LOGIC_OP_ENABLE: return DAE_FX_STATE_LOGIC_OP_ENABLE;
		case MULTISAMPLE_ENABLE: return DAE_FX_STATE_MULTISAMPLE_ENABLE;
		case NORMALIZE_ENABLE: return DAE_FX_STATE_NORMALIZE_ENABLE;
		case POINT_SMOOTH_ENABLE: return DAE_FX_STATE_POINT_SMOOTH_ENABLE;
		case POLYGON_OFFSET_FILL_ENABLE: return DAE_FX_STATE_POLYGON_OFFSET_FILL_ENABLE;
		case POLYGON_OFFSET_LINE_ENABLE: return DAE_FX_STATE_POLYGON_OFFSET_LINE_ENABLE;
		case POLYGON_OFFSET_POINT_ENABLE: return DAE_FX_STATE_POLYGON_OFFSET_POINT_ENABLE;
		case POLYGON_SMOOTH_ENABLE: return DAE_FX_STATE_POLYGON_SMOOTH_ENABLE;
		case POLYGON_STIPPLE_ENABLE: return DAE_FX_STATE_POLYGON_STIPPLE_ENABLE;
		case RESCALE_NORMAL_ENABLE: return DAE_FX_STATE_RESCALE_NORMAL_ENABLE;
		case SAMPLE_ALPHA_TO_COVERAGE_ENABLE: return DAE_FX_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE;
		case SAMPLE_ALPHA_TO_ONE_ENABLE: return DAE_FX_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE;
		case SAMPLE_COVERAGE_ENABLE: return DAE_FX_STATE_SAMPLE_COVERAGE_ENABLE;
		case SCISSOR_TEST_ENABLE: return DAE_FX_STATE_SCISSOR_TEST_ENABLE;
		case STENCIL_TEST_ENABLE: return DAE_FX_STATE_STENCIL_TEST_ENABLE;
		case INVALID:
		default: return DAEERR_UNKNOWN_ELEMENT;
		}
	}
};
