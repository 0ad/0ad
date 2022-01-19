/* Copyright (C) 2022 Wildfire Games.
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

class CStr;

namespace Renderer
{

namespace Backend
{

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

struct BlendStateDesc
{
	bool enabled;
	BlendFactor srcColorBlendFactor;
	BlendFactor dstColorBlendFactor;
	BlendOp colorBlendOp;
	BlendFactor srcAlphaBlendFactor;
	BlendFactor dstAlphaBlendFactor;
	BlendOp alphaBlendOp;
	CColor constant;
};

// TODO: Add a shader program to the graphics pipeline state.
struct GraphicsPipelineStateDesc
{
	BlendStateDesc blendState;
};

// We don't provide additional helpers intentionally because all custom states
// should be described with a related shader and should be switched together.
GraphicsPipelineStateDesc MakeDefaultGraphicsPipelineStateDesc();

BlendFactor ParseBlendFactor(const CStr& str);
BlendOp ParseBlendOp(const CStr& str);

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_PIPELINESTATE
