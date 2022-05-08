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

#ifndef INCLUDED_RENDERER_BACKEND_ITEXTURE
#define INCLUDED_RENDERER_BACKEND_ITEXTURE

#include "renderer/backend/Format.h"
#include "renderer/backend/IDeviceObject.h"
#include "renderer/backend/Sampler.h"

#include <cstdint>

namespace Renderer
{

namespace Backend
{

class ITexture : public IDeviceObject<ITexture>
{
public:
	enum class Type
	{
		TEXTURE_2D,
		TEXTURE_2D_MULTISAMPLE,
		TEXTURE_CUBE
	};

	virtual Type GetType() const = 0;
	virtual Format GetFormat() const = 0;

	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual uint32_t GetMIPLevelCount() const = 0;
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_ITEXTURE
