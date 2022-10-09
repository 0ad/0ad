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

#ifndef INCLUDED_RENDERER_BACKEND_SAMPLER
#define INCLUDED_RENDERER_BACKEND_SAMPLER

#include "graphics/Color.h"
#include "renderer/backend/CompareOp.h"

#include <cstdint>

namespace Renderer
{

namespace Backend
{

namespace Sampler
{

enum class Filter
{
	NEAREST,
	LINEAR
};

enum class AddressMode
{
	REPEAT,
	MIRRORED_REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
};

enum class BorderColor
{
	TRANSPARENT_BLACK,
	OPAQUE_BLACK,
	OPAQUE_WHITE
};

struct Desc
{
	Filter magFilter;
	Filter minFilter;
	Filter mipFilter;
	AddressMode addressModeU;
	AddressMode addressModeV;
	AddressMode addressModeW;
	float mipLODBias;
	bool anisotropyEnabled;
	float maxAnisotropy;
	// When some filter is CLAMP_TO_BORDER.
	BorderColor borderColor;
	bool compareEnabled;
	CompareOp compareOp;
};

Desc MakeDefaultSampler(Filter filter, AddressMode addressMode);

} // namespace Sampler

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_SAMPLER
