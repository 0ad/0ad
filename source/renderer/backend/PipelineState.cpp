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

#include "precompiled.h"

#include "PipelineState.h"

#include <limits>

namespace Renderer
{

namespace Backend
{

GraphicsPipelineStateDesc MakeDefaultGraphicsPipelineStateDesc()
{
	GraphicsPipelineStateDesc desc{};

	desc.depthStencilState.depthTestEnabled = true;
	desc.depthStencilState.depthCompareOp = CompareOp::LESS_OR_EQUAL;
	desc.depthStencilState.depthWriteEnabled = true;
	desc.depthStencilState.stencilTestEnabled = false;
	desc.depthStencilState.stencilFrontFace.failOp = StencilOp::KEEP;
	desc.depthStencilState.stencilFrontFace.passOp = StencilOp::KEEP;
	desc.depthStencilState.stencilFrontFace.depthFailOp = StencilOp::KEEP;
	desc.depthStencilState.stencilFrontFace.compareOp = CompareOp::ALWAYS;
	desc.depthStencilState.stencilBackFace = desc.depthStencilState.stencilFrontFace;
	desc.depthStencilState.stencilReadMask = desc.depthStencilState.stencilWriteMask =
		std::numeric_limits<uint32_t>::max();
	desc.depthStencilState.stencilReference = 0;

	desc.blendState.enabled = false;
	desc.blendState.srcColorBlendFactor = desc.blendState.srcAlphaBlendFactor =
		BlendFactor::ONE;
	desc.blendState.dstColorBlendFactor = desc.blendState.dstAlphaBlendFactor =
		BlendFactor::ZERO;
	desc.blendState.colorBlendOp = desc.blendState.alphaBlendOp = BlendOp::ADD;
	desc.blendState.constant = CColor(0.0f, 0.0f, 0.0f, 0.0f);
	desc.blendState.colorWriteMask =
		ColorWriteMask::RED | ColorWriteMask::GREEN | ColorWriteMask::BLUE | ColorWriteMask::ALPHA;

	desc.rasterizationState.cullMode = CullMode::BACK;
	desc.rasterizationState.frontFace = FrontFace::COUNTER_CLOCKWISE;
	return desc;
}

StencilOp ParseStencilOp(const CStr& str)
{
#define CASE(NAME) if (str == #NAME) return StencilOp::NAME
	CASE(KEEP);
	CASE(ZERO);
	CASE(REPLACE);
	CASE(INCREMENT_AND_CLAMP);
	CASE(DECREMENT_AND_CLAMP);
	CASE(INVERT);
	CASE(INCREMENT_AND_WRAP);
	CASE(DECREMENT_AND_WRAP);
#undef CASE
	debug_warn("Invalid stencil op");
	return StencilOp::KEEP;
}

BlendFactor ParseBlendFactor(const CStr& str)
{
	// TODO: it might make sense to use upper case in XML for consistency.
#define CASE(NAME, VALUE) if (str == NAME) return BlendFactor::VALUE
	CASE("zero", ZERO);
	CASE("one", ONE);
	CASE("src_color", SRC_COLOR);
	CASE("one_minus_src_color", ONE_MINUS_SRC_COLOR);
	CASE("dst_color", DST_COLOR);
	CASE("one_minus_dst_color", ONE_MINUS_DST_COLOR);
	CASE("src_alpha", SRC_ALPHA);
	CASE("one_minus_src_alpha", ONE_MINUS_SRC_ALPHA);
	CASE("dst_alpha", DST_ALPHA);
	CASE("one_minus_dst_alpha", ONE_MINUS_DST_ALPHA);
	CASE("constant_color", CONSTANT_COLOR);
	CASE("one_minus_constant_color", ONE_MINUS_CONSTANT_COLOR);
	CASE("constant_alpha", CONSTANT_ALPHA);
	CASE("one_minus_constant_alpha", ONE_MINUS_CONSTANT_ALPHA);
	CASE("src_alpha_saturate", SRC_ALPHA_SATURATE);
	CASE("src1_color", SRC1_COLOR);
	CASE("one_minus_src1_color", ONE_MINUS_SRC1_COLOR);
	CASE("src1_alpha", SRC1_ALPHA);
	CASE("one_minus_src1_alpha", ONE_MINUS_SRC1_ALPHA);
#undef CASE
	debug_warn("Invalid blend factor");
	return BlendFactor::ZERO;
}

BlendOp ParseBlendOp(const CStr& str)
{
#define CASE(NAME) if (str == #NAME) return BlendOp::NAME
	CASE(ADD);
	CASE(SUBTRACT);
	CASE(REVERSE_SUBTRACT);
	CASE(MIN);
	CASE(MAX);
#undef CASE
	debug_warn("Invalid blend op");
	return BlendOp::ADD;
}

CullMode ParseCullMode(const CStr& str)
{
	if (str == "NONE")
		return CullMode::NONE;
	else if (str == "FRONT")
		return CullMode::FRONT;
	else if (str == "BACK")
		return CullMode::BACK;
	debug_warn("Invalid cull mode");
	return CullMode::BACK;
}

FrontFace ParseFrontFace(const CStr& str)
{
	if (str == "CLOCKWISE")
		return FrontFace::CLOCKWISE;
	else if (str == "COUNTER_CLOCKWISE")
		return FrontFace::COUNTER_CLOCKWISE;
	debug_warn("Invalid front face");
	return FrontFace::COUNTER_CLOCKWISE;
}

} // namespace Backend

} // namespace Renderer
