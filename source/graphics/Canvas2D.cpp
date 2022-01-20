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

#include "Canvas2D.h"

#include "graphics/Color.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextRenderer.h"
#include "graphics/TextureManager.h"
#include "gui/GUIMatrix.h"
#include "maths/Rect.h"
#include "maths/Vector2D.h"
#include "ps/CStrInternStatic.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/Renderer.h"

#include <array>

namespace
{

// Array of 2D elements unrolled into 1D array.
using PlaneArray2D = std::array<float, 8>;

inline void DrawTextureImpl(const CShaderProgramPtr& shader, CTexturePtr texture,
	const PlaneArray2D& vertices, PlaneArray2D uvs,
	const CColor& multiply, const CColor& add, const float grayscaleFactor)
{
	shader->BindTexture(str_tex, texture);
	for (size_t idx = 0; idx < uvs.size(); idx += 2)
	{
		if (texture->GetWidth() > 0.0f)
			uvs[idx + 0] /= texture->GetWidth();
		if (texture->GetHeight() > 0.0f)
			uvs[idx + 1] /= texture->GetHeight();
	}

	shader->Uniform(str_transform, GetDefaultGuiMatrix());
	shader->Uniform(str_colorAdd, add);
	shader->Uniform(str_colorMul, multiply);
	shader->Uniform(str_grayscaleFactor, grayscaleFactor);
	shader->VertexPointer(2, GL_FLOAT, 0, vertices.data());
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, uvs.data());
	shader->AssertPointersBound();

	glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size() / 2);
}

} // anonymous namespace

class CCanvas2D::Impl
{
public:
	void BindTechIfNeeded()
	{
		if (Tech)
			return;

		CShaderDefines defines;
		Tech = g_Renderer.GetShaderManager().LoadEffect(str_canvas2d, defines);
		ENSURE(Tech);
		Tech->BeginPass();
		// TODO: remove global renderer access.
		g_Renderer.GetDeviceCommandContext()->SetGraphicsPipelineState(
			Tech->GetGraphicsPipelineStateDesc());
	}

	void UnbindTech()
	{
		if (!Tech)
			return;

		Tech->EndPass();
		Tech.reset();
	}

	CShaderTechniquePtr Tech;
};

CCanvas2D::CCanvas2D() : m(std::make_unique<Impl>())
{

}

CCanvas2D::~CCanvas2D()
{
	Flush();
}

void CCanvas2D::DrawLine(const std::vector<CVector2D>& points, const float width, const CColor& color)
{
	m->BindTechIfNeeded();

	std::vector<float> vertices;
	std::vector<float> uvs(points.size() * 2, 0.0f);
	vertices.reserve(points.size() * 2);
	for (const CVector2D& point : points)
	{
		vertices.emplace_back(point.X);
		vertices.emplace_back(point.Y);
	}

	CShaderProgramPtr shader = m->Tech->GetShader();
	shader->BindTexture(str_tex, g_Renderer.GetTextureManager().GetTransparentTexture());
	shader->Uniform(str_transform, GetDefaultGuiMatrix());
	shader->Uniform(str_colorAdd, color);
	shader->Uniform(str_colorMul, CColor(0.0f, 0.0f, 0.0f, 0.0f));
	shader->Uniform(str_grayscaleFactor, 0.0f);
	shader->VertexPointer(2, GL_FLOAT, 0, vertices.data());
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, uvs.data());
	shader->AssertPointersBound();

#if !CONFIG2_GLES
	glEnable(GL_LINE_SMOOTH);
#endif
	glLineWidth(width);
	glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 2);
	glLineWidth(1.0f);
#if !CONFIG2_GLES
	glDisable(GL_LINE_SMOOTH);
#endif
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

	m->BindTechIfNeeded();
	DrawTextureImpl(
		m->Tech->GetShader(), g_Renderer.GetTextureManager().GetTransparentTexture(),
		vertices, uvs, CColor(0.0f, 0.0f, 0.0f, 0.0f), color, 0.0f);
}

void CCanvas2D::DrawTexture(CTexturePtr texture, const CRect& destination)
{
	DrawTexture(texture,
		destination, CRect(0, 0, texture->GetWidth(), texture->GetHeight()),
		CColor(1.0f, 1.0f, 1.0f, 1.0f), CColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
}

void CCanvas2D::DrawTexture(
	CTexturePtr texture, const CRect& destination, const CRect& source,
	const CColor& multiply, const CColor& add, const float grayscaleFactor)
{
	const PlaneArray2D uvs = {
		source.left, source.bottom,
		source.right, source.bottom,
		source.right, source.top,
		source.left, source.top
	};
	const PlaneArray2D vertices = {
		destination.left, destination.bottom,
		destination.right, destination.bottom,
		destination.right, destination.top,
		destination.left, destination.top
	};

	m->BindTechIfNeeded();
	DrawTextureImpl(m->Tech->GetShader(), texture, vertices, uvs, multiply, add, grayscaleFactor);
}

void CCanvas2D::DrawText(CTextRenderer& textRenderer)
{
	m->BindTechIfNeeded();

	CShaderProgramPtr shader = m->Tech->GetShader();
	shader->Uniform(str_grayscaleFactor, 0.0f);

	textRenderer.Render(shader, GetDefaultGuiMatrix());
}

void CCanvas2D::Flush()
{
	m->UnbindTech();
}
