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

#include "Mapping.h"

#include "lib/code_annotation.h"
#include "lib/config2.h"

namespace Renderer
{

namespace Backend
{

namespace GL
{

namespace Mapping
{

GLenum FromBlendFactor(BlendFactor blendFactor)
{
	GLenum factor = GL_ZERO;
	switch (blendFactor)
	{
#define CASE(NAME) case BlendFactor::NAME: factor = GL_##NAME; break
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
#undef CASE
	// Dual source blending presents only in GL_VERSION >= 3.3.
	case BlendFactor::SRC1_COLOR: FALLTHROUGH;
	case BlendFactor::ONE_MINUS_SRC1_COLOR: FALLTHROUGH;
	case BlendFactor::SRC1_ALPHA: FALLTHROUGH;
	case BlendFactor::ONE_MINUS_SRC1_ALPHA:
		debug_warn("Unsupported blend factor.");
		break;
	}
	return factor;
}

GLenum FromBlendOp(BlendOp blendOp)
{
	GLenum mode = GL_FUNC_ADD;
	switch (blendOp)
	{
	case BlendOp::ADD: mode = GL_FUNC_ADD; break;
	case BlendOp::SUBTRACT: mode = GL_FUNC_SUBTRACT; break;
	case BlendOp::REVERSE_SUBTRACT: mode = GL_FUNC_REVERSE_SUBTRACT; break;
#if !CONFIG2_GLES
	case BlendOp::MIN: mode = GL_MIN; break;
	case BlendOp::MAX: mode = GL_MAX; break;
#else
	case BlendOp::MIN: FALLTHROUGH;
	case BlendOp::MAX:
		debug_warn("Unsupported blend op.");
		break;
#endif
	};
	return mode;
}

} // namespace Mapping

} // namespace GL

} // namespace Backend

} // namespace Renderer
