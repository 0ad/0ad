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

#include "DeviceCommandContext.h"

#include "renderer/backend/gl/Texture.h"

namespace Renderer
{

namespace Backend
{

namespace GL
{

// static
std::unique_ptr<CDeviceCommandContext> CDeviceCommandContext::Create()
{
	std::unique_ptr<CDeviceCommandContext> deviceCommandContext(new CDeviceCommandContext());

	return deviceCommandContext;
}

CDeviceCommandContext::CDeviceCommandContext() = default;

CDeviceCommandContext::~CDeviceCommandContext() = default;

void CDeviceCommandContext::UploadTexture(
	CTexture* texture, const Format format, const void* data, const size_t dataSize)
{
	UploadTextureRegion(texture, format, data, dataSize, 0, 0, texture->GetWidth(), texture->GetHeight());
}

void CDeviceCommandContext::UploadTextureRegion(CTexture* texture, const Format format, const void* data, const size_t dataSize,
	const uint32_t xOffset, const uint32_t yOffset, const uint32_t width, const uint32_t height)
{
	if (texture->GetType() == CTexture::Type::TEXTURE_2D)
	{
		if (texture->GetFormat() == Format::R8G8B8A8 || texture->GetFormat() == Format::A8)
		{
			ENSURE(width > 0 && height > 0);
			ENSURE(texture->GetFormat() == format);
			const size_t bpp = format == Format::R8G8B8A8 ? 4 : 1;
			ENSURE(dataSize == width * height * bpp);
			ENSURE(xOffset + width <= texture->GetWidth());
			ENSURE(yOffset + height <= texture->GetHeight());

			glBindTexture(GL_TEXTURE_2D, texture->GetHandle());
			glTexSubImage2D(GL_TEXTURE_2D, 0,
				xOffset, yOffset, width, height,
				format == Format::R8G8B8A8 ? GL_RGBA : GL_ALPHA, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_2D, 0);

			ogl_WarnIfError();
		}
		else
			debug_warn("Unsupported format");
	}
	else
		debug_warn("Unsupported type");
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
