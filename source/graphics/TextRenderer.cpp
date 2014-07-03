/* Copyright (C) 2012 Wildfire Games.
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
#include "lib/ogl.h"
#include "ps/CStrIntern.h"
#include "renderer/Renderer.h"

extern int g_xres, g_yres;

CTextRenderer::CTextRenderer(const CShaderProgramPtr& shader) :
	m_Shader(shader)
{
	ResetTransform();
	Color(CColor(1.0f, 1.0f, 1.0f, 1.0f));
	Font(str_sans_10);
}

void CTextRenderer::ResetTransform()
{
	m_Transform.SetIdentity();
	m_Transform.Scale(1.0f, -1.f, 1.0f);
	m_Transform.Translate(0.0f, (float)g_yres, -1000.0f);

	CMatrix3D proj;
	proj.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
	m_Transform = proj * m_Transform;
	m_Dirty = true;
}

CMatrix3D CTextRenderer::GetTransform()
{
	return m_Transform;
}

void CTextRenderer::SetTransform(const CMatrix3D& transform)
{
	m_Transform = transform;
	m_Dirty = true;
}

void CTextRenderer::Translate(float x, float y, float z)
{
	CMatrix3D m;
	m.SetTranslation(x, y, z);
	m_Transform = m_Transform * m;
	m_Dirty = true;
}

void CTextRenderer::SetClippingRect(const CRect& rect)
{
	m_Clipping = rect;
}

void CTextRenderer::Color(const CColor& color)
{
	if (m_Color != color)
	{
		m_Color = color;
		m_Dirty = true;
	}
}

void CTextRenderer::Color(float r, float g, float b, float a)
{
	Color(CColor(r, g, b, a));
}

void CTextRenderer::Font(CStrIntern font)
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
		debug_printf(L"CTextRenderer::Printf vswprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);

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
		debug_printf(L"CTextRenderer::PrintfAt vswprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);

	Put(x, y, buf);
}

void CTextRenderer::PutAdvance(const wchar_t* buf)
{
	Put(0.0f, 0.0f, buf);

	int w, h;
	m_Font->CalculateStringSize(buf, w, h);
	Translate((float)w, 0.0f, 0.0f);
}

void CTextRenderer::Put(float x, float y, const wchar_t* buf)
{
	if (buf[0] == 0)
		return; // empty string; don't bother storing

	PutString(x, y, new std::wstring(buf), true);
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
		batch.transform = m_Transform;
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
		if (a.font < b.font)
			return true;
		if (b.font < a.font)
			return false;
		// TODO: is it worth sorting by color/transform too?
		return false;
	}
};

void CTextRenderer::Render()
{
	std::vector<u16> indexes;
	std::vector<t2f_v2i> vertexes;

	// Try to merge non-consecutive batches that share the same font/color/transform:
	// sort the batch list by font, then merge the runs of adjacent compatible batches
	m_Batches.sort(SBatchCompare());
	for (std::list<SBatch>::iterator it = m_Batches.begin(); it != m_Batches.end(); )
	{
		std::list<SBatch>::iterator next = it;
		++next;
		if (next != m_Batches.end() && it->font == next->font && it->color == next->color && it->transform == next->transform)
		{
			it->chars += next->chars;
			it->runs.splice(it->runs.end(), next->runs);
			m_Batches.erase(next);
		}
		else
			++it;
	}

	for (std::list<SBatch>::iterator it = m_Batches.begin(); it != m_Batches.end(); ++it)
	{
		SBatch& batch = *it;

		const CFont::GlyphMap& glyphs = batch.font->GetGlyphs();

		m_Shader->BindTexture(str_tex, batch.font->GetTexture());

		m_Shader->Uniform(str_transform, batch.transform);

		// ALPHA-only textures will have .rgb sampled as 0, so we need to
		// replace it with white (but not affect RGBA textures)
		if (batch.font->HasRGB())
			m_Shader->Uniform(str_colorAdd, CColor(0.0f, 0.0f, 0.0f, 0.0f));
		else
			m_Shader->Uniform(str_colorAdd, CColor(1.0f, 1.0f, 1.0f, 0.0f));

		m_Shader->Uniform(str_colorMul, batch.color);

		vertexes.resize(batch.chars*4);
		indexes.resize(batch.chars*6);

		size_t idx = 0;

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

				indexes[idx*6+0] = idx*4+0;
				indexes[idx*6+1] = idx*4+1;
				indexes[idx*6+2] = idx*4+2;
				indexes[idx*6+3] = idx*4+2;
				indexes[idx*6+4] = idx*4+3;
				indexes[idx*6+5] = idx*4+0;

				x += g->xadvance;

				idx++;
			}
		}

		m_Shader->VertexPointer(2, GL_SHORT, sizeof(t2f_v2i), &vertexes[0].x);
		m_Shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, sizeof(t2f_v2i), &vertexes[0].u);

		glDrawElements(GL_TRIANGLES, indexes.size(), GL_UNSIGNED_SHORT, &indexes[0]);
	}

	m_Batches.clear();
}
