/* Copyright (C) 2024 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_RENDERER_BACKEND_PIPELINESTATE
#define INCLUDED_RENDERER_BACKEND_PIPELINESTATE

#include "graphics/Color.h"
#include "renderer/backend/CompareOp.h"
#include "renderer/backend/IDeviceObject.h"
#include "renderer/backend/IShaderProgram.h"

class CStr;

namespace Renderer
{

namespace Backend
{

enum class StencilOp
{
	// Keeps the current value.
	KEEP,
	// Sets the value to zero.
	ZERO,
	// Sets the value to reference.
	REPLACE,
	// Increments the value and clamps to the maximum representable unsigned
	// value.
	INCREMENT_AND_CLAMP,
	// Decrements the value and clamps to zero.
	DECREMENT_AND_CLAMP,
	// Bitwise inverts the value.
	INVERT,
	// Increments the value and wraps it to zero when incrementing the maximum
	// representable unsigned value.
	INCREMENT_AND_WRAP,
	// Decrements the value and wraps it to the maximum representable unsigned
	// value when decrementing zero.
	DECREMENT_AND_WRAP
};

struct SStencilOpState
{
	StencilOp failOp;
	StencilOp passOp;
	StencilOp depthFailOp;
	CompareOp compareOp;
};

struct SDepthStencilStateDesc
{
	bool depthTestEnabled;
	CompareOp depthCompareOp;
	bool depthWriteEnabled;
	bool stencilTestEnabled;
	uint32_t stencilReadMask;
	uint32_t stencilWriteMask;
	uint32_t stencilReference;
	SStencilOpState stencilFrontFace;
	SStencilOpState stencilBackFace;
};

// TODO: add per constant description.

enum class BlendFactor
{
	ZERO,
	ONE,
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR,
	DST_COLOR,
	ONE_MINUS_DST_COLOR,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA,
	DST_ALPHA,
	ONE_MINUS_DST_ALPHA,
	CONSTANT_COLOR,
	ONE_MINUS_CONSTANT_COLOR,
	CONSTANT_ALPHA,
	ONE_MINUS_CONSTANT_ALPHA,
	SRC_ALPHA_SATURATE,
	SRC1_COLOR,
	ONE_MINUS_SRC1_COLOR,
	SRC1_ALPHA,
	ONE_MINUS_SRC1_ALPHA,
};

enum class BlendOp
{
	ADD,
	SUBTRACT,
	REVERSE_SUBTRACT,
	MIN,
	MAX
};

// Using a namespace instead of a enum allows using the same syntax while
// avoiding adding operator overrides and additional checks on casts.
namespace ColorWriteMask
{
constexpr uint8_t RED = 0x01;
constexpr uint8_t GREEN = 0x02;
constexpr uint8_t BLUE = 0x04;
constexpr uint8_t ALPHA = 0x08;
} // namespace ColorWriteMask

struct SBlendStateDesc
{
	bool enabled;
	BlendFactor srcColorBlendFactor;
	BlendFactor dstColorBlendFactor;
	BlendOp colorBlendOp;
	BlendFactor srcAlphaBlendFactor;
	BlendFactor dstAlphaBlendFactor;
	BlendOp alphaBlendOp;
	CColor constant;
	uint8_t colorWriteMask;
};

enum class PolygonMode
{
	FILL,
	LINE
};

enum class CullMode
{
	NONE,
	FRONT,
	BACK
};

enum class FrontFace
{
	COUNTER_CLOCKWISE,
	CLOCKWISE
};

struct SRasterizationStateDesc
{
	PolygonMode polygonMode;
	CullMode cullMode;
	FrontFace frontFace;
	bool depthBiasEnabled;
	float depthBiasConstantFactor;
	float depthBiasSlopeFactor;
};

struct SGraphicsPipelineStateDesc
{
	// It's a backend client reponsibility to keep the shader program alive
	// while it's bound.
	IShaderProgram* shaderProgram;
	SDepthStencilStateDesc depthStencilState;
	SBlendStateDesc blendState;
	SRasterizationStateDesc rasterizationState;
};

struct SComputePipelineStateDesc
{
	// It's a backend client reponsibility to keep the shader program alive
	// while it's bound.
	IShaderProgram* shaderProgram;
};

// We don't provide additional helpers intentionally because all custom states
// should be described with a related shader and should be switched together.
SGraphicsPipelineStateDesc MakeDefaultGraphicsPipelineStateDesc();

StencilOp ParseStencilOp(const CStr& str);

BlendFactor ParseBlendFactor(const CStr& str);
BlendOp ParseBlendOp(const CStr& str);

PolygonMode ParsePolygonMode(const CStr& str);
CullMode ParseCullMode(const CStr& str);
FrontFace ParseFrontFace(const CStr& str);

/**
 * A holder for precompiled graphics pipeline description.
 */
class IGraphicsPipelineState : public IDeviceObject<IGraphicsPipelineState>
{
public:
	virtual IShaderProgram* GetShaderProgram() const = 0;
};

/**
 * A holder for precompiled compute pipeline description.
 */
class IComputePipelineState : public IDeviceObject<IComputePipelineState>
{
public:
	virtual IShaderProgram* GetShaderProgram() const = 0;
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_PIPELINESTATE
