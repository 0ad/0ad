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

#include "lib/ogl.h"
#include "lib/res/graphics/unifont.h"
#include "ps/Font.h"

extern int g_xres, g_yres;

CTextRenderer::CTextRenderer(const CShaderProgramPtr& shader) :
	m_Shader(shader)
{
	ResetTransform();
	Color(CColor(1.0f, 1.0f, 1.0f, 1.0f));
	Font(L"sans-10");
}

void CTextRenderer::ResetTransform()
{
	m_Transform.SetIdentity();
	m_Transform.Scale(1.0f, -1.f, 1.0f);
	m_Transform.Translate(0.0f, (float)g_yres, -1000.0f);

	CMatrix3D proj;
	proj.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
	m_Transform = proj * m_Transform;
}

CMatrix3D CTextRenderer::GetTransform()
{
	return m_Transform;
}

void CTextRenderer::SetTransform(const CMatrix3D& transform)
{
	m_Transform = transform;
}

void CTextRenderer::Translate(float x, float y, float z)
{
	CMatrix3D m;
	m.SetTranslation(x, y, z);
	m_Transform = m_Transform * m;
}

void CTextRenderer::Color(const CColor& color)
{
	m_Color = color;
}

void CTextRenderer::Color(float r, float g, float b, float a)
{
	m_Color = CColor(r, g, b, a);
}

void CTextRenderer::Font(const CStrW& font)
{
	if (!m_Fonts[font])
		m_Fonts[font] = shared_ptr<CFont>(new CFont(font));

	m_Font = m_Fonts[font];
}

void CTextRenderer::Printf(const wchar_t* fmt, ...)
{
	wchar_t buf[1024] = {0};

	va_list args;
	va_start(args, fmt);
	int ret = vswprintf(buf, ARRAY_SIZE(buf)-1, fmt, args);
	va_end(args);

	if (ret < 0)
		debug_printf(L"CTextRenderer::Printf vswprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);

	if (ret == 0)
		return; // empty string; don't bother storing

	SBatch batch;
	batch.transform = m_Transform;
	batch.color = m_Color;
	batch.font = m_Font;
	batch.text = buf;
	m_Batches.push_back(batch);

	int w, h;
	batch.font->CalculateStringSize(batch.text, w, h);
	Translate((float)w, 0.0f, 0.0f);
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

	if (ret == 0)
		return; // empty string; don't bother storing

	CMatrix3D translate;
	translate.SetTranslation(x, y, 0.0f);

	SBatch batch;
	batch.transform = m_Transform * translate;
	batch.color = m_Color;
	batch.font = m_Font;
	batch.text = buf;
	m_Batches.push_back(batch);
}

struct t2f_v2i
{
	t2f_v2i() : u(0), v(0), x(0), y(0) { }
	float u, v;
	i16 x, y;
};

void CTextRenderer::Render()
{
	std::vector<u16> indexes;
	std::vector<t2f_v2i> vertexes;

	for (size_t i = 0; i < m_Batches.size(); ++i)
	{
		SBatch& batch = m_Batches[i];

		if (batch.text.empty()) // avoid zero-length arrays
			continue;

		const std::map<u16, UnifontGlyphData>& glyphs = batch.font->GetGlyphs();

		m_Shader->BindTexture("tex", batch.font->GetTexture());

		m_Shader->Uniform("transform", batch.transform);

		// ALPHA-only textures will have .rgb sampled as 0, so we need to
		// replace it with white (but not affect RGBA textures)
		if (batch.font->HasRGB())
			m_Shader->Uniform("colorAdd", CColor(0.0f, 0.0f, 0.0f, 0.0f));
		else
			m_Shader->Uniform("colorAdd", CColor(1.0f, 1.0f, 1.0f, 0.0f));

		m_Shader->Uniform("colorMul", batch.color);

		vertexes.clear();
		vertexes.resize(batch.text.size()*4);

		indexes.clear();
		indexes.resize(batch.text.size()*6);

		i16 x = 0;
		for (size_t i = 0; i < batch.text.size(); ++i)
		{
			std::map<u16, UnifontGlyphData>::const_iterator it = glyphs.find(batch.text[i]);

			if (it == glyphs.end())
				it = glyphs.find(0xFFFD); // Use the missing glyph symbol

			if (it == glyphs.end()) // Missing the missing glyph symbol - give up
				continue;

			const UnifontGlyphData& g = it->second;

			vertexes[i*4].u = g.u1;
			vertexes[i*4].v = g.v0;
			vertexes[i*4].x = g.x1 + x;
			vertexes[i*4].y = g.y0;

			vertexes[i*4+1].u = g.u0;
			vertexes[i*4+1].v = g.v0;
			vertexes[i*4+1].x = g.x0 + x;
			vertexes[i*4+1].y = g.y0;

			vertexes[i*4+2].u = g.u0;
			vertexes[i*4+2].v = g.v1;
			vertexes[i*4+2].x = g.x0 + x;
			vertexes[i*4+2].y = g.y1;

			vertexes[i*4+3].u = g.u1;
			vertexes[i*4+3].v = g.v1;
			vertexes[i*4+3].x = g.x1 + x;
			vertexes[i*4+3].y = g.y1;

			indexes[i*6+0] = i*4+0;
			indexes[i*6+1] = i*4+1;
			indexes[i*6+2] = i*4+2;
			indexes[i*6+3] = i*4+2;
			indexes[i*6+4] = i*4+3;
			indexes[i*6+5] = i*4+0;

			x += g.xadvance;
		}

		m_Shader->VertexPointer(2, GL_SHORT, sizeof(t2f_v2i), &vertexes[0].x);
		m_Shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, sizeof(t2f_v2i), &vertexes[0].u);

		glDrawElements(GL_TRIANGLES, indexes.size(), GL_UNSIGNED_SHORT, &indexes[0]);
	}

	m_Batches.clear();
}
