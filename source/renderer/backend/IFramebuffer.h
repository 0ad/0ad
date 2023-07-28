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

#ifndef INCLUDED_RENDERER_BACKEND_IFRAMEBUFFER
#define INCLUDED_RENDERER_BACKEND_IFRAMEBUFFER

#include "graphics/Color.h"
#include "renderer/backend/IDeviceObject.h"

namespace Renderer
{

namespace Backend
{

class ITexture;

/**
 * Load operation is set for each attachment, what should be done with its
 * content on BeginFramebufferPass.
 */
enum class AttachmentLoadOp
{
	// Loads the attachment content.
	LOAD,
	// Clears the attachment content without loading. Prefer to use that
	// operation over manual ClearFramebuffer.
	CLEAR,
	// After BeginFramebufferPass the attachment content is undefined.
	DONT_CARE
};

/**
 * Store operation is set for each attachment, what should be done with its
 * content on EndFramebufferPass.
 */
enum class AttachmentStoreOp
{
	// Stores the attachment content.
	STORE,
	// After EndFramebufferPass the attachment content is undefined.
	DONT_CARE
};

struct SColorAttachment
{
	ITexture* texture = nullptr;
	AttachmentLoadOp loadOp = AttachmentLoadOp::DONT_CARE;
	AttachmentStoreOp storeOp = AttachmentStoreOp::DONT_CARE;
	CColor clearColor;
};

struct SDepthStencilAttachment
{
	ITexture* texture = nullptr;
	AttachmentLoadOp loadOp = AttachmentLoadOp::DONT_CARE;
	AttachmentStoreOp storeOp = AttachmentStoreOp::DONT_CARE;
};

/**
 * IFramebuffer stores attachments which should be used by backend as rendering
 * destinations. That combining allows to set these destinations at once.
 * IFramebuffer doesn't own its attachments so clients must keep them alive.
 * The number of framebuffers ever created for a device during its lifetime
 * should be as small as possible.
 *
 * Framebuffer is a term from OpenGL/Vulkan worlds (D3D synonym is a render
 * target).
 */
class IFramebuffer : public IDeviceObject<IFramebuffer>
{
public:
	/**
	 * Returns a clear color for all color attachments of the framebuffer.
	 * @see IDevice::CreateFramebuffer()
	 */
	virtual const CColor& GetClearColor() const = 0;

	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_IFRAMEBUFFER
