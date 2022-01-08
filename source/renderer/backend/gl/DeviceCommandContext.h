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

#include <memory>

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

	void UploadTexture(CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t level = 0, const uint32_t layer = 0);
	void UploadTextureRegion(CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t xOffset, const uint32_t yOffset,
		const uint32_t width, const uint32_t height,
		const uint32_t level = 0, const uint32_t layer = 0);

private:
	CDeviceCommandContext();
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_GL_DEVICECOMMANDCONTEXT
