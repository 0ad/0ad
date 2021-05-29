/* Copyright (C) 2021 Wildfire Games.
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

#include "Canvas2D.h"

#include "graphics/Color.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"
#include "gui/GUIMatrix.h"
#include "maths/Rect.h"
#include "maths/Vector2D.h"
#include "ps/CStrInternStatic.h"
#include "renderer/Renderer.h"

#include <array>

namespace
{

// Array of 2D elements unrolled into 1D array.
using PlaneArray2D = std::array<float, 8>;

void DrawTextureImpl(CTexturePtr texture,
	const PlaneArray2D& vertices, const PlaneArray2D& uvs,
	const CColor& multiply, const CColor& add)
{
	CShaderDefines defines;
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(
		str_canvas2d, g_Renderer.GetSystemShaderDefines(), defines);
	tech->BeginPass();
	CShaderProgramPtr shader = tech->GetShader();

	shader->BindTexture(str_tex, texture);
	shader->Uniform(str_transform, GetDefaultGuiMatrix());
	shader->Uniform(str_colorAdd, add);
	shader->Uniform(str_colorMul, multiply);
	shader->VertexPointer(2, GL_FLOAT, 0, vertices.data());
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, uvs.data());
	shader->AssertPointersBound();

	if (!g_Renderer.DoSkipSubmit())
		glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size() / 2);

	tech->EndPass();
}

} // anonymous namespace

void CCanvas2D::DrawLine(const std::vector<CVector2D>& points, const float width, const CColor& color)
{
	std::vector<float> vertices;
	vertices.reserve(points.size() * 3);
	for (const CVector2D& point : points)
	{
		vertices.emplace_back(point.X);
		vertices.emplace_back(point.Y);
		vertices.emplace_back(0.0f);
	}

	// Setup the render state
	CMatrix3D transform = GetDefaultGuiMatrix();
	CShaderDefines lineDefines;
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_gui_solid, g_Renderer.GetSystemShaderDefines(), lineDefines);
	tech->BeginPass();
	CShaderProgramPtr shader = tech->GetShader();

	shader->Uniform(str_transform, transform);
	shader->Uniform(str_color, color);
	shader->VertexPointer(3, GL_FLOAT, 0, vertices.data());
	shader->AssertPointersBound();

#if !CONFIG2_GLES
	glEnable(GL_LINE_SMOOTH);
#endif
	glLineWidth(width);
	if (!g_Renderer.DoSkipSubmit())
		glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 3);
	glLineWidth(1.0f);
#if !CONFIG2_GLES
	glDisable(GL_LINE_SMOOTH);
#endif

	tech->EndPass();
}

void CCanvas2D::DrawRect(const CRect& rect, const CColor& color)
{
	const PlaneArray2D uvs = {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};
	const PlaneArray2D vertices = {
		rect.left, rect.bottom,
		rect.right, rect.bottom,
		rect.right, rect.top,
		rect.left, rect.top
	};

	DrawTextureImpl(
		g_Renderer.GetTextureManager().GetTransparentTexture(),
		vertices, uvs, CColor(0.0f, 0.0f, 0.0f, 0.0f), color);
}

void CCanvas2D::DrawTexture(CTexturePtr texture, const CRect& destination)
{
	DrawTexture(texture,
		destination, CRect(0, 0, texture->GetWidth(), texture->GetHeight()),
		CColor(1.0f, 1.0f, 1.0f, 1.0f), CColor(0.0f, 0.0f, 0.0f, 0.0f));
}

void CCanvas2D::DrawTexture(
	CTexturePtr texture, const CRect& destination, const CRect& source,
	const CColor& multiply, const CColor& add)
{
	PlaneArray2D uvs = {
		source.left, source.bottom,
		source.right, source.bottom,
		source.right, source.top,
		source.left, source.top
	};
	for (size_t idx = 0; idx < uvs.size() / 2; idx += 2)
	{
		uvs[idx + 0] /= texture->GetWidth();
		uvs[idx + 1] /= texture->GetHeight();
	}
	const PlaneArray2D vertices = {
		destination.left, destination.bottom,
		destination.right, destination.bottom,
		destination.right, destination.top,
		destination.left, destination.top
	};

	DrawTextureImpl(
		g_Renderer.GetTextureManager().GetTransparentTexture(),
		vertices, uvs, multiply, add);
}