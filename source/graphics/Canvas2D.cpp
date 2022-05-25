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
#include "renderer/Renderer.h"

#include <array>

namespace
{

// Array of 2D elements unrolled into 1D array.
using PlaneArray2D = std::array<float, 12>;

struct SBindingSlots
{
	int32_t transform;
	int32_t colorAdd;
	int32_t colorMul;
	int32_t grayscaleFactor;
	int32_t tex;
};

inline void DrawTextureImpl(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CTexturePtr& texture, const PlaneArray2D& vertices, PlaneArray2D uvs,
	const CColor& multiply, const CColor& add, const float grayscaleFactor,
	const SBindingSlots& bindingSlots)
{
	texture->UploadBackendTextureIfNeeded(deviceCommandContext);
	deviceCommandContext->SetTexture(
		bindingSlots.tex, texture->GetBackendTexture());
	for (size_t idx = 0; idx < uvs.size(); idx += 2)
	{
		if (texture->GetWidth() > 0.0f)
			uvs[idx + 0] /= texture->GetWidth();
		if (texture->GetHeight() > 0.0f)
			uvs[idx + 1] /= texture->GetHeight();
	}

	deviceCommandContext->SetUniform(bindingSlots.colorAdd, add.AsFloatArray());
	deviceCommandContext->SetUniform(bindingSlots.colorMul, multiply.AsFloatArray());
	deviceCommandContext->SetUniform(bindingSlots.grayscaleFactor, grayscaleFactor);

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, 0,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::UV0,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, 0,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 1);

	deviceCommandContext->SetVertexBufferData(
		0, vertices.data(), vertices.size() * sizeof(vertices[0]));
	deviceCommandContext->SetVertexBufferData(
		1, uvs.data(), uvs.size() * sizeof(uvs[0]));

	deviceCommandContext->Draw(0, vertices.size() / 2);
}

} // anonymous namespace

class CCanvas2D::Impl
{
public:
	Impl(Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
		: DeviceCommandContext(deviceCommandContext)
	{
	}

	void BindTechIfNeeded()
	{
		if (Tech)
			return;

		CShaderDefines defines;
		Tech = g_Renderer.GetShaderManager().LoadEffect(str_canvas2d, defines);
		// The canvas technique must be loaded because we can't render UI without it.
		ENSURE(Tech);
		DeviceCommandContext->SetGraphicsPipelineState(
			Tech->GetGraphicsPipelineStateDesc());
		DeviceCommandContext->BeginPass();
		Renderer::Backend::IShaderProgram* shader = Tech->GetShader();

		BindingSlots.transform = shader->GetBindingSlot(str_transform);
		BindingSlots.colorAdd = shader->GetBindingSlot(str_colorAdd);
		BindingSlots.colorMul = shader->GetBindingSlot(str_colorMul);
		BindingSlots.grayscaleFactor = shader->GetBindingSlot(str_grayscaleFactor);
		BindingSlots.tex = shader->GetBindingSlot(str_tex);

		const CMatrix3D transform = GetDefaultGuiMatrix();
		DeviceCommandContext->SetUniform(
			BindingSlots.transform, transform.AsFloatArray());
	}

	void UnbindTech()
	{
		if (!Tech)
			return;

		DeviceCommandContext->EndPass();
		Tech.reset();
	}

	Renderer::Backend::IDeviceCommandContext* DeviceCommandContext = nullptr;
	CShaderTechniquePtr Tech;

	// We assume that the shader can't be destroyed while it's bound. So these
	// bindings remain valid while the shader is alive.
	SBindingSlots BindingSlots;
};

CCanvas2D::CCanvas2D(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
	: m(std::make_unique<Impl>(deviceCommandContext))
{

}

CCanvas2D::~CCanvas2D()
{
	Flush();
}

void CCanvas2D::DrawLine(const std::vector<CVector2D>& points, const float width, const CColor& color)
{
	if (points.empty())
		return;

	// We could reuse the terrain line building, but it uses 3D space instead of
	// 2D. So it can be less optimal for a canvas.

	// Adding a single pixel line with alpha gradient to reduce the aliasing
	// effect.
	const float halfWidth = width * 0.5f + 1.0f;

	struct PointIndex
	{
		size_t index;
		float length;
		CVector2D normal;
	};
	// Normal for the last index is undefined.
	std::vector<PointIndex> pointsIndices;
	pointsIndices.reserve(points.size());
	pointsIndices.emplace_back(PointIndex{0, 0.0f, CVector2D()});
	for (size_t index = 0; index < points.size();)
	{
		size_t nextIndex = index + 1;
		CVector2D direction;
		float length = 0.0f;
		while (nextIndex < points.size())
		{
			direction = points[nextIndex] - points[pointsIndices.back().index];
			length = direction.Length();
			if (length >= halfWidth * 2.0f)
			{
				direction /= length;
				break;
			}
			++nextIndex;
		}
		if (nextIndex == points.size())
			break;
		pointsIndices.back().length = length;
		pointsIndices.back().normal = CVector2D(-direction.Y, direction.X);
		pointsIndices.emplace_back(PointIndex{nextIndex, 0.0f, CVector2D()});
		index = nextIndex;
	}

	if (pointsIndices.size() <= 1)
		return;

	std::vector<std::array<CVector2D, 3>> vertices;
	std::vector<std::array<CVector2D, 3>> uvs;
	std::vector<u16> indices;
	const size_t reserveSize = 2 * pointsIndices.size() - 1;
	vertices.reserve(reserveSize);
	uvs.reserve(reserveSize);
	indices.reserve(reserveSize * 12);

	auto addVertices = [&vertices, &uvs, &indices, &halfWidth](const CVector2D& p1, const CVector2D& p2)
	{
		if (!vertices.empty())
		{
			const u16 lastVertexIndex = static_cast<u16>(vertices.size() * 3 - 1);
			ENSURE(lastVertexIndex >= 2);
			// First vertical half of the segment.
			indices.emplace_back(lastVertexIndex - 2);
			indices.emplace_back(lastVertexIndex - 1);
			indices.emplace_back(lastVertexIndex + 2);
			indices.emplace_back(lastVertexIndex - 2);
			indices.emplace_back(lastVertexIndex + 2);
			indices.emplace_back(lastVertexIndex + 1);
			// Second vertical half of the segment.
			indices.emplace_back(lastVertexIndex - 1);
			indices.emplace_back(lastVertexIndex);
			indices.emplace_back(lastVertexIndex + 3);
			indices.emplace_back(lastVertexIndex - 1);
			indices.emplace_back(lastVertexIndex + 3);
			indices.emplace_back(lastVertexIndex + 2);
		}
		vertices.emplace_back(std::array<CVector2D, 3>{p1, (p1 + p2) / 2.0f, p2});
		uvs.emplace_back(std::array<CVector2D, 3>{
			CVector2D(0.0f, 0.0f),
			CVector2D(std::max(1.0f, halfWidth - 1.0f), 0.0f),
			CVector2D(0.0f, 0.0f)});
	};

	addVertices(
		points[pointsIndices.front().index] - pointsIndices.front().normal * halfWidth,
		points[pointsIndices.front().index] + pointsIndices.front().normal * halfWidth);
	// For each pair of adjacent segments we need to add smooth transition.
	for (size_t index = 0; index + 2 < pointsIndices.size(); ++index)
	{
		const PointIndex& pointIndex = pointsIndices[index];
		const PointIndex& nextPointIndex = pointsIndices[index + 1];
		// Angle between adjacent segments.
		const float cosAlpha = pointIndex.normal.Dot(nextPointIndex.normal);
		constexpr float EPS = 1e-3f;
		// Use a simple segment if adjacent segments are almost codirectional.
		if (cosAlpha > 1.0f - EPS)
		{
			addVertices(
				points[pointIndex.index] - pointIndex.normal * halfWidth,
				points[pointIndex.index] + pointIndex.normal * halfWidth);
		}
		else
		{
			addVertices(
				points[nextPointIndex.index] - pointIndex.normal * halfWidth,
				points[nextPointIndex.index] + pointIndex.normal * halfWidth);
			// Average normal between adjacent segments. We might want to rotate it but
			// for now we assume that it's enough for current line widths.
			const CVector2D normal = cosAlpha < -1.0f + EPS
				? CVector2D(pointIndex.normal.Y, -pointIndex.normal.X)
				: ((pointIndex.normal + nextPointIndex.normal) / 2.0f).Normalized();
			addVertices(
				points[nextPointIndex.index] - normal * halfWidth,
				points[nextPointIndex.index] + normal * halfWidth);
			addVertices(
				points[nextPointIndex.index] - nextPointIndex.normal * halfWidth,
				points[nextPointIndex.index] + nextPointIndex.normal * halfWidth);
		}
		// We use 16-bit indices, it means that we can't use more than 64K vertices.
		const size_t requiredFreeSpace = 3 * 4;
		if (vertices.size() * 3 + requiredFreeSpace >= 65536)
			break;
	}
	addVertices(
		points[pointsIndices.back().index] - pointsIndices[pointsIndices.size() - 2].normal * halfWidth,
		points[pointsIndices.back().index] + pointsIndices[pointsIndices.size() - 2].normal * halfWidth);

	m->BindTechIfNeeded();

	m->DeviceCommandContext->SetTexture(
		m->BindingSlots.tex,
		g_Renderer.GetTextureManager().GetAlphaGradientTexture()->GetBackendTexture());
	const CColor colorAdd(0.0f, 0.0f, 0.0f, 0.0f);
	m->DeviceCommandContext->SetUniform(
		m->BindingSlots.colorAdd, colorAdd.AsFloatArray());
	m->DeviceCommandContext->SetUniform(
		m->BindingSlots.colorMul, color.AsFloatArray());
	m->DeviceCommandContext->SetUniform(
		m->BindingSlots.grayscaleFactor, 0.0f);

	m->DeviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, 0,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	m->DeviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::UV0,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, 0,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 1);

	m->DeviceCommandContext->SetVertexBufferData(0, vertices.data(), vertices.size() * sizeof(vertices[0]));
	m->DeviceCommandContext->SetVertexBufferData(1, uvs.data(), uvs.size() * sizeof(uvs[0]));

	m->DeviceCommandContext->SetIndexBufferData(indices.data(), indices.size() * sizeof(indices[0]));
	m->DeviceCommandContext->DrawIndexed(0, indices.size(), 0);
}

void CCanvas2D::DrawRect(const CRect& rect, const CColor& color)
{
	const PlaneArray2D uvs
	{
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};
	const PlaneArray2D vertices =
	{
		rect.left, rect.bottom,
		rect.right, rect.bottom,
		rect.right, rect.top,
		rect.left, rect.bottom,
		rect.right, rect.top,
		rect.left, rect.top
	};

	m->BindTechIfNeeded();
	DrawTextureImpl(
		m->DeviceCommandContext,
		g_Renderer.GetTextureManager().GetTransparentTexture(),
		vertices, uvs, CColor(0.0f, 0.0f, 0.0f, 0.0f), color, 0.0f,
		m->BindingSlots);
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
	const PlaneArray2D uvs =
	{
		source.left, source.bottom,
		source.right, source.bottom,
		source.right, source.top,
		source.left, source.bottom,
		source.right, source.top,
		source.left, source.top
	};
	const PlaneArray2D vertices =
	{
		destination.left, destination.bottom,
		destination.right, destination.bottom,
		destination.right, destination.top,
		destination.left, destination.bottom,
		destination.right, destination.top,
		destination.left, destination.top
	};

	m->BindTechIfNeeded();
	DrawTextureImpl(
		m->DeviceCommandContext, texture, vertices, uvs,
		multiply, add, grayscaleFactor, m->BindingSlots);
}

void CCanvas2D::DrawText(CTextRenderer& textRenderer)
{
	m->BindTechIfNeeded();

	m->DeviceCommandContext->SetUniform(
		m->BindingSlots.grayscaleFactor, 0.0f);

	textRenderer.Render(m->DeviceCommandContext, m->Tech->GetShader(), GetDefaultGuiMatrix());
}

void CCanvas2D::Flush()
{
	m->UnbindTech();
}
