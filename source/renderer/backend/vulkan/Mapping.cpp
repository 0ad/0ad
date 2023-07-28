/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Mapping.h"

#include "lib/code_annotation.h"
#include "lib/config2.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

namespace Mapping
{

VkCompareOp FromCompareOp(const CompareOp compareOp)
{
	VkCompareOp op = VK_COMPARE_OP_NEVER;
	switch (compareOp)
	{
#define CASE(NAME) case CompareOp::NAME: op = VK_COMPARE_OP_##NAME; break
	CASE(NEVER);
	CASE(LESS);
	CASE(EQUAL);
	CASE(LESS_OR_EQUAL);
	CASE(GREATER);
	CASE(NOT_EQUAL);
	CASE(GREATER_OR_EQUAL);
	CASE(ALWAYS);
#undef CASE
	}
	return op;
}

VkStencilOp FromStencilOp(const StencilOp stencilOp)
{
	VkStencilOp op = VK_STENCIL_OP_KEEP;
	switch (stencilOp)
	{
#define CASE(NAME) case StencilOp::NAME: op = VK_STENCIL_OP_##NAME; break
	CASE(KEEP);
	CASE(ZERO);
	CASE(REPLACE);
	CASE(INCREMENT_AND_CLAMP);
	CASE(DECREMENT_AND_CLAMP);
	CASE(INVERT);
	CASE(INCREMENT_AND_WRAP);
	CASE(DECREMENT_AND_WRAP);
#undef CASE
	}
	return op;
}

VkBlendFactor FromBlendFactor(const BlendFactor blendFactor)
{
	VkBlendFactor factor = VK_BLEND_FACTOR_ZERO;
	switch (blendFactor)
	{
#define CASE(NAME) case BlendFactor::NAME: factor = VK_BLEND_FACTOR_##NAME; break
	CASE(ZERO);
	CASE(ONE);
	CASE(SRC_COLOR);
	CASE(ONE_MINUS_SRC_COLOR);
	CASE(DST_COLOR);
	CASE(ONE_MINUS_DST_COLOR);
	CASE(SRC_ALPHA);
	CASE(ONE_MINUS_SRC_ALPHA);
	CASE(DST_ALPHA);
	CASE(ONE_MINUS_DST_ALPHA);
	CASE(CONSTANT_COLOR);
	CASE(ONE_MINUS_CONSTANT_COLOR);
	CASE(CONSTANT_ALPHA);
	CASE(ONE_MINUS_CONSTANT_ALPHA);
	CASE(SRC_ALPHA_SATURATE);

	CASE(SRC1_COLOR);
	CASE(ONE_MINUS_SRC1_COLOR);
	CASE(SRC1_ALPHA);
	CASE(ONE_MINUS_SRC1_ALPHA);
#undef CASE
	}
	return factor;
}

VkBlendOp FromBlendOp(const BlendOp blendOp)
{
	VkBlendOp mode = VK_BLEND_OP_ADD;
	switch (blendOp)
	{
	case BlendOp::ADD: mode = VK_BLEND_OP_ADD; break;
	case BlendOp::SUBTRACT: mode = VK_BLEND_OP_SUBTRACT; break;
	case BlendOp::REVERSE_SUBTRACT: mode = VK_BLEND_OP_REVERSE_SUBTRACT; break;
	case BlendOp::MIN: mode = VK_BLEND_OP_MIN; break;
	case BlendOp::MAX: mode = VK_BLEND_OP_MAX; break;
	};
	return mode;
}

VkColorComponentFlags FromColorWriteMask(const uint32_t colorWriteMask)
{
	VkColorComponentFlags flags = 0;
	if (colorWriteMask & ColorWriteMask::RED)
		flags |= VK_COLOR_COMPONENT_R_BIT;
	if (colorWriteMask & ColorWriteMask::GREEN)
		flags |= VK_COLOR_COMPONENT_G_BIT;
	if (colorWriteMask & ColorWriteMask::BLUE)
		flags |= VK_COLOR_COMPONENT_B_BIT;
	if (colorWriteMask & ColorWriteMask::ALPHA)
		flags |= VK_COLOR_COMPONENT_A_BIT;
	return flags;
}

VkPolygonMode FromPolygonMode(const PolygonMode polygonMode)
{
	if (polygonMode == PolygonMode::LINE)
		return VK_POLYGON_MODE_LINE;
	return VK_POLYGON_MODE_FILL;
}

VkCullModeFlags FromCullMode(const CullMode cullMode)
{
	VkCullModeFlags flags = VK_CULL_MODE_NONE;
	switch (cullMode)
	{
	case CullMode::NONE:
		break;
	case CullMode::FRONT:
		flags |= VK_CULL_MODE_FRONT_BIT;
		break;
	case CullMode::BACK:
		flags |= VK_CULL_MODE_BACK_BIT;
		break;
	}
	return flags;
}

VkFormat FromFormat(const Format format)
{
	VkFormat resultFormat = VK_FORMAT_UNDEFINED;
	switch (format)
	{
#define CASE(NAME) case Format::NAME: resultFormat = VK_FORMAT_##NAME; break;
#define CASE2(NAME, VK_NAME) case Format::NAME: resultFormat = VK_FORMAT_##VK_NAME; break;

	CASE(UNDEFINED)

	CASE(R8_UNORM)
	CASE(R8G8_UNORM)
	CASE(R8G8_UINT)
	CASE(R8G8B8A8_UNORM)
	CASE(R8G8B8A8_UINT)
	CASE(B8G8R8A8_UNORM)

	CASE(R16_UNORM)
	CASE(R16_UINT)
	CASE(R16_SINT)
	CASE(R16G16_UNORM)
	CASE(R16G16_UINT)
	CASE(R16G16_SINT)

	CASE(R32_SFLOAT)
	CASE(R32G32_SFLOAT)
	CASE(R32G32B32_SFLOAT)
	CASE(R32G32B32A32_SFLOAT)

	CASE(D16_UNORM)
	CASE2(D24_UNORM, X8_D24_UNORM_PACK32)
	CASE(D24_UNORM_S8_UINT)
	CASE(D32_SFLOAT_S8_UINT)
	CASE(D32_SFLOAT)

	CASE2(BC1_RGB_UNORM, BC1_RGB_UNORM_BLOCK)
	CASE2(BC1_RGBA_UNORM, BC1_RGBA_UNORM_BLOCK)
	CASE2(BC2_UNORM, BC2_UNORM_BLOCK)
	CASE2(BC3_UNORM, BC3_UNORM_BLOCK)

#undef CASE
#undef CASE2
	default:
		debug_warn("Unsupported format");
	}
	return resultFormat;
}

VkSampleCountFlagBits FromSampleCount(const uint32_t sampleCount)
{
	VkSampleCountFlagBits flags = VK_SAMPLE_COUNT_1_BIT;
	switch (sampleCount)
	{
	case 1: flags = VK_SAMPLE_COUNT_1_BIT; break;
	case 2: flags = VK_SAMPLE_COUNT_2_BIT; break;
	case 4: flags = VK_SAMPLE_COUNT_4_BIT; break;
	case 8: flags = VK_SAMPLE_COUNT_8_BIT; break;
	case 16: flags = VK_SAMPLE_COUNT_16_BIT; break;
	default:
		debug_warn("Unsupported number of samples");
	}
	return flags;
}

VkSamplerAddressMode FromAddressMode(const Sampler::AddressMode addressMode)
{
	VkSamplerAddressMode resultAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	switch (addressMode)
	{
	case Sampler::AddressMode::REPEAT:
		resultAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case Sampler::AddressMode::MIRRORED_REPEAT:
		resultAddressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		break;
	case Sampler::AddressMode::CLAMP_TO_EDGE:
		resultAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case Sampler::AddressMode::CLAMP_TO_BORDER:
		resultAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	}
	return resultAddressMode;
}

VkAttachmentLoadOp FromAttachmentLoadOp(const AttachmentLoadOp loadOp)
{
	VkAttachmentLoadOp resultLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	switch (loadOp)
	{
	case AttachmentLoadOp::LOAD:
		resultLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		break;
	case AttachmentLoadOp::CLEAR:
		resultLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		break;
	case AttachmentLoadOp::DONT_CARE:
		resultLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		break;
	}
	return resultLoadOp;
}

VkAttachmentStoreOp FromAttachmentStoreOp(const AttachmentStoreOp storeOp)
{
	VkAttachmentStoreOp resultStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	switch (storeOp)
	{
	case AttachmentStoreOp::STORE:
		resultStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		break;
	case AttachmentStoreOp::DONT_CARE:
		resultStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		break;
	}
	return resultStoreOp;
}

} // namespace Mapping

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
