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

#ifndef INCLUDED_RENDERER_GL_DEVICECOMMANDCONTEXT
#define INCLUDED_RENDERER_GL_DEVICECOMMANDCONTEXT

#include "renderer/backend/Format.h"
#include "renderer/backend/PipelineState.h"

#include <array>
#include <memory>
#include <optional>

namespace Renderer
{

namespace Backend
{

namespace GL
{

class CTexture;

class CDeviceCommandContext
{
public:
	~CDeviceCommandContext();

	static std::unique_ptr<CDeviceCommandContext> Create();

	void SetGraphicsPipelineState(const GraphicsPipelineStateDesc& pipelineStateDesc);

	void UploadTexture(CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t level = 0, const uint32_t layer = 0);
	void UploadTextureRegion(CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t xOffset, const uint32_t yOffset,
		const uint32_t width, const uint32_t height,
		const uint32_t level = 0, const uint32_t layer = 0);

	// TODO: maybe we should add a more common type, like CRectI.
	struct ScissorRect
	{
		int32_t x, y;
		int32_t width, height;
	};
	void SetScissors(const uint32_t scissorCount, const ScissorRect* scissors);

	void Flush();

private:
	CDeviceCommandContext();

	void ResetStates();

	void SetGraphicsPipelineStateImpl(
		const GraphicsPipelineStateDesc& pipelineStateDesc, const bool force);

	GraphicsPipelineStateDesc m_GraphicsPipelineStateDesc{};
	uint32_t m_ScissorCount = 0;
	// GL2.1 doesn't support more than 1 scissor.
	std::array<ScissorRect, 1> m_Scissors;
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_GL_DEVICECOMMANDCONTEXT
