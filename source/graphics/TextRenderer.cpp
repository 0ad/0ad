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

	CMatrix3D translate;
	translate.SetTranslation(x, y, 0.0f);

	SBatch batch;
	batch.transform = m_Transform * translate;
	batch.color = m_Color;
	batch.font = m_Font;
	batch.text = buf;
	m_Batches.push_back(batch);
}

void CTextRenderer::Render()
{
	for (size_t i = 0; i < m_Batches.size(); ++i)
	{
		SBatch& batch = m_Batches[i];

		int unit = m_Shader->GetTextureUnit("tex");
		if (unit == -1) // just in case the shader doesn't use the sampler
			continue;

		batch.font->Bind(unit);

		m_Shader->Uniform("transform", batch.transform);

		// ALPHA-only textures will have .rgb sampled as 0, so we need to
		// replace it with white (but not affect RGBA textures)
		if (batch.font->HasRGB())
			m_Shader->Uniform("colorAdd", CColor(0.0f, 0.0f, 0.0f, 0.0f));
		else
			m_Shader->Uniform("colorAdd", CColor(1.0f, 1.0f, 1.0f, 0.0f));

		m_Shader->Uniform("colorMul", batch.color);

		unifont_render(batch.text.c_str());
	}

	m_Batches.clear();
}
