/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_RENDERER_BACKEND_GL_FRAMEBUFFER
#define INCLUDED_RENDERER_BACKEND_GL_FRAMEBUFFER

#include "graphics/Color.h"
#include "lib/ogl.h"
#include "renderer/backend/IFramebuffer.h"

#include <cstdint>
#include <memory>

namespace Renderer
{

namespace Backend
{

namespace GL
{

class CDevice;
class CTexture;

class CFramebuffer final : public IFramebuffer
{
public:
	~CFramebuffer() override;

	IDevice* GetDevice() override;

	const CColor& GetClearColor() const override { return m_ClearColor; }

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }

	GLuint GetHandle() const { return m_Handle; }
	GLbitfield GetAttachmentMask() const { return m_AttachmentMask; }

	AttachmentLoadOp GetColorAttachmentLoadOp() const { return m_ColorAttachmentLoadOp; }
	AttachmentStoreOp GetColorAttachmentStoreOp() const { return m_ColorAttachmentStoreOp; }
	AttachmentLoadOp GetDepthStencilAttachmentLoadOp() const { return m_DepthStencilAttachmentLoadOp; }
	AttachmentStoreOp GetDepthStencilAttachmentStoreOp() const { return m_DepthStencilAttachmentStoreOp; }

private:
	friend class CDevice;

	static std::unique_ptr<CFramebuffer> Create(
		CDevice* device, const char* name, SColorAttachment* colorAttachment,
		SDepthStencilAttachment* depthStencilAttachment);
	static std::unique_ptr<CFramebuffer> CreateBackbuffer(
		CDevice* device,
		const int surfaceDrawableWidth, const int surfaceDrawableHeight,
		const AttachmentLoadOp colorAttachmentLoadOp,
		const AttachmentStoreOp colorAttachmentStoreOp,
		const AttachmentLoadOp depthStencilAttachmentLoadOp,
		const AttachmentStoreOp depthStencilAttachmentStoreOp);

	CFramebuffer();

	CDevice* m_Device = nullptr;

	GLuint m_Handle = 0;
	uint32_t m_Width = 0, m_Height = 0;
	GLbitfield m_AttachmentMask = 0;

	CColor m_ClearColor;

	AttachmentLoadOp m_ColorAttachmentLoadOp = AttachmentLoadOp::DONT_CARE;
	AttachmentStoreOp m_ColorAttachmentStoreOp = AttachmentStoreOp::DONT_CARE;
	AttachmentLoadOp m_DepthStencilAttachmentLoadOp = AttachmentLoadOp::DONT_CARE;
	AttachmentStoreOp m_DepthStencilAttachmentStoreOp = AttachmentStoreOp::DONT_CARE;
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_FRAMEBUFFER
