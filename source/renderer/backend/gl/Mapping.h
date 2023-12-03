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

#ifndef INCLUDED_RENDERER_BACKEND_GL_MAPPING
#define INCLUDED_RENDERER_BACKEND_GL_MAPPING

#include "lib/ogl.h"
#include "renderer/backend/PipelineState.h"

namespace Renderer
{

namespace Backend
{

namespace GL
{

namespace Mapping
{

GLenum FromCompareOp(const CompareOp compareOp);

GLenum FromStencilOp(const StencilOp stencilOp);

GLenum FromBlendFactor(const BlendFactor blendFactor);

GLenum FromBlendOp(const BlendOp blendOp);

} // namespace Mapping

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_MAPPING
