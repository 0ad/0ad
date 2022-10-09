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

#include "TextRenderer.h"

#include "graphics/Font.h"
#include "graphics/FontManager.h"
#include "graphics/ShaderProgram.h"
#include "graphics/TextureManager.h"
#include "maths/Matrix3D.h"
#include "ps/CStrIntern.h"
#include "ps/CStrInternStatic.h"
#include "renderer/Renderer.h"

#include <errno.h>

namespace
{

// We can't draw chars more than vertices, currently we use 4 vertices per char.
constexpr size_t MAX_CHAR_COUNT_PER_BATCH = 65536 / 4;

} // anonymous namespace

CTextRenderer::CTextRenderer()
{
	ResetTranslate();
	SetCurrentColor(CColor(1.0f, 1.0f, 1.0f, 1.0f));
	SetCurrentFont(str_sans_10);
}

void CTextRenderer::ResetTranslate(const CVector2D& translate)
{
	m_Translate = translate;
	m_Dirty = true;
}

void CTextRenderer::Translate(float x, float y)
{
	m_Translate += CVector2D{x, y};
	m_Dirty = true;
}

void CTextRenderer::SetClippingRect(const CRect& rect)
{
	m_Clipping = rect;
}

void CTextRenderer::SetCurrentColor(const CColor& color)
{
	if (m_Color != color)
	{
		m_Color = color;
		m_Dirty = true;
	}
}

void CTextRenderer::SetCurrentFont(CStrIntern font)
{
	if (font != m_FontName)
	{
		m_FontName = font;
		m_Font = g_Renderer.GetFontManager().LoadFont(font);
		m_Dirty = true;
	}
}

void CTextRenderer::PrintfAdvance(const wchar_t* fmt, ...)
{
	wchar_t buf[1024] = {0};

	va_list args;
	va_start(args, fmt);
	int ret = vswprintf(buf, ARRAY_SIZE(buf)-1, fmt, args);
	va_end(args);

	if (ret < 0)
		debug_printf("CTextRenderer::Printf vswprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);

	PutAdvance(buf);
}


void CTextRenderer::PrintfAt(float x, float y, const wchar_t* fmt, ...)
{
	wchar_t buf[1024] = {0};

	va_list args;
	va_start(args, fmt);
	int ret = vswprintf(buf, ARRAY_SIZE(buf)-1, fmt, args);
	va_end(args);

	if (ret < 0)
		debug_printf("CTextRenderer::PrintfAt vswprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);

	Put(x, y, buf);
}

void CTextRenderer::PutAdvance(const wchar_t* buf)
{
	Put(0.0f, 0.0f, buf);

	int w, h;
	m_Font->CalculateStringSize(buf, w, h);
	Translate((float)w, 0.0f);
}

void CTextRenderer::Put(float x, float y, const wchar_t* buf)
{
	if (buf[0] == 0)
		return; // empty string; don't bother storing

	PutString(x, y, new std::wstring(buf), true);
}

void CTextRenderer::Put(float x, float y, const char* buf)
{
	if (buf[0] == 0)
		return; // empty string; don't bother storing

	PutString(x, y, new std::wstring(wstring_from_utf8(buf)), true);
}

void CTextRenderer::Put(float x, float y, const std::wstring* buf)
{
	if (buf->empty())
		return; // empty string; don't bother storing

	PutString(x, y, buf, false);
}

void CTextRenderer::PutString(float x, float y, const std::wstring* buf, bool owned)
{
	if (!m_Font)
		return; // invalid font; can't render

	if (m_Clipping != CRect())
	{
		float x0, y0, x1, y1;
		m_Font->GetGlyphBounds(x0, y0, x1, y1);
		if (y + y1 < m_Clipping.top)
			return;
		if (y + y0 > m_Clipping.bottom)
			return;
	}

	// If any state has changed since the last batch, start a new batch
	if (m_Dirty)
	{
		SBatch batch;
		batch.chars = 0;
		batch.translate = m_Translate;
		batch.color = m_Color;
		batch.font = m_Font;
		m_Batches.push_back(batch);
		m_Dirty = false;
	}

	// Push a new run onto the latest batch
	SBatchRun run;
	run.x = x;
	run.y = y;
	m_Batches.back().runs.push_back(run);
	m_Batches.back().runs.back().text = buf;
	m_Batches.back().runs.back().owned = owned;
	m_Batches.back().chars += buf->size();
}


struct t2f_v2i
{
	t2f_v2i() : u(0), v(0), x(0), y(0) { }
	float u, v;
	i16 x, y;
};

struct SBatchCompare
{
	bool operator()(const CTextRenderer::SBatch& a, const CTextRenderer::SBatch& b)
	{
		if (a.font != b.font)
			return a.font < b.font;
		// TODO: is it worth sorting by color/translate too?
		return false;
	}
};

void CTextRenderer::Render(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::IShaderProgram* shader,
	const CVector2D& transformScale, const CVector2D& translation)
{
	std::vector<u16> indexes;
	std::vector<t2f_v2i> vertexes;

	// Try to merge non-consecutive batches that share the same font/color/translate:
	// sort the batch list by font, then merge the runs of adjacent compatible batches
	m_Batches.sort(SBatchCompare());
	for (std::list<SBatch>::iterator it = m_Batches.begin(); it != m_Batches.end(); )
	{
		std::list<SBatch>::iterator next = std::next(it);
		if (next != m_Batches.end() && it->chars + next->chars <= MAX_CHAR_COUNT_PER_BATCH && it->font == next->font && it->color == next->color && it->translate == next->translate)
		{
			it->chars += next->chars;
			it->runs.splice(it->runs.end(), next->runs);
			m_Batches.erase(next);
		}
		else
			++it;
	}

	const int32_t texBindingSlot = shader->GetBindingSlot(str_tex);
	const int32_t translationBindingSlot = shader->GetBindingSlot(str_translation);
	const int32_t colorAddBindingSlot = shader->GetBindingSlot(str_colorAdd);
	const int32_t colorMulBindingSlot = shader->GetBindingSlot(str_colorMul);

	bool translationChanged = false;

	CTexture* lastTexture = nullptr;
	for (std::list<SBatch>::iterator it = m_Batches.begin(); it != m_Batches.end(); ++it)
	{
		SBatch& batch = *it;

		const CFont::GlyphMap& glyphs = batch.font->GetGlyphs();

		if (lastTexture != batch.font->GetTexture().get())
		{
			lastTexture = batch.font->GetTexture().get();
			lastTexture->UploadBackendTextureIfNeeded(deviceCommandContext);
			deviceCommandContext->SetTexture(texBindingSlot, lastTexture->GetBackendTexture());
		}

		if (batch.translate.X != 0.0f || batch.translate.Y != 0.0f)
		{
			const CVector2D localTranslation =
				translation + CVector2D(batch.translate.X * transformScale.X, batch.translate.Y * transformScale.Y);
			deviceCommandContext->SetUniform(translationBindingSlot, localTranslation.AsFloatArray());
			translationChanged = true;
		}

		// ALPHA-only textures will have .rgb sampled as 0, so we need to
		// replace it with white (but not affect RGBA textures)
		if (batch.font->HasRGB())
			deviceCommandContext->SetUniform(colorAddBindingSlot, 0.0f, 0.0f, 0.0f, 0.0f);
		else
			deviceCommandContext->SetUniform(colorAddBindingSlot, batch.color.r, batch.color.g, batch.color.b, 0.0f);

		deviceCommandContext->SetUniform(colorMulBindingSlot, batch.color.AsFloatArray());

		vertexes.resize(std::min(MAX_CHAR_COUNT_PER_BATCH, batch.chars) * 4);
		indexes.resize(std::min(MAX_CHAR_COUNT_PER_BATCH, batch.chars) * 6);

		size_t idx = 0;

		auto flush = [deviceCommandContext, &idx, &vertexes, &indexes]() -> void
		{
			if (idx == 0)
				return;

			const uint32_t stride = sizeof(t2f_v2i);

			deviceCommandContext->SetVertexAttributeFormat(
				Renderer::Backend::VertexAttributeStream::POSITION,
				Renderer::Backend::Format::R16G16_SINT, offsetof(t2f_v2i, x), stride,
				Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
			deviceCommandContext->SetVertexAttributeFormat(
				Renderer::Backend::VertexAttributeStream::UV0,
				Renderer::Backend::Format::R32G32_SFLOAT, offsetof(t2f_v2i, u), stride,
				Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);

			deviceCommandContext->SetVertexBufferData(
				0, vertexes.data(), vertexes.size() * sizeof(vertexes[0]));
			deviceCommandContext->SetIndexBufferData(indexes.data(), indexes.size() * sizeof(indexes[0]));

			deviceCommandContext->DrawIndexed(0, idx * 6, 0);
			idx = 0;
		};

		for (std::list<SBatchRun>::iterator runit = batch.runs.begin(); runit != batch.runs.end(); ++runit)
		{
			SBatchRun& run = *runit;
			i16 x = run.x;
			i16 y = run.y;
			for (size_t i = 0; i < run.text->size(); ++i)
			{
				const CFont::GlyphData* g = glyphs.get((*run.text)[i]);

				if (!g)
					g = glyphs.get(0xFFFD); // Use the missing glyph symbol
				if (!g) // Missing the missing glyph symbol - give up
					continue;

				vertexes[idx*4].u = g->u1;
				vertexes[idx*4].v = g->v0;
				vertexes[idx*4].x = g->x1 + x;
				vertexes[idx*4].y = g->y0 + y;

				vertexes[idx*4+1].u = g->u0;
				vertexes[idx*4+1].v = g->v0;
				vertexes[idx*4+1].x = g->x0 + x;
				vertexes[idx*4+1].y = g->y0 + y;

				vertexes[idx*4+2].u = g->u0;
				vertexes[idx*4+2].v = g->v1;
				vertexes[idx*4+2].x = g->x0 + x;
				vertexes[idx*4+2].y = g->y1 + y;

				vertexes[idx*4+3].u = g->u1;
				vertexes[idx*4+3].v = g->v1;
				vertexes[idx*4+3].x = g->x1 + x;
				vertexes[idx*4+3].y = g->y1 + y;

				indexes[idx*6+0] = static_cast<u16>(idx*4+0);
				indexes[idx*6+1] = static_cast<u16>(idx*4+1);
				indexes[idx*6+2] = static_cast<u16>(idx*4+2);
				indexes[idx*6+3] = static_cast<u16>(idx*4+2);
				indexes[idx*6+4] = static_cast<u16>(idx*4+3);
				indexes[idx*6+5] = static_cast<u16>(idx*4+0);

				x += g->xadvance;

				++idx;
				if (idx == MAX_CHAR_COUNT_PER_BATCH)
					flush();
			}
		}

		flush();
	}

	m_Batches.clear();

	if (translationChanged)
		deviceCommandContext->SetUniform(translationBindingSlot, translation.AsFloatArray());
}
