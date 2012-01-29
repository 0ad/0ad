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

extern int g_yres;

CTextRenderer::CTextRenderer()
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
	{
		debug_printf(L"glwprintf failed (buffer size exceeded?) - return value %d, errno %d\n", ret, errno);
	}

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

void CTextRenderer::Render()
{
	for (size_t i = 0; i < m_Batches.size(); ++i)
	{
		SBatch& batch = m_Batches[i];

		batch.font->Bind();

		glPushMatrix();
		glLoadMatrixf(&batch.transform._11);

		glColor4fv(batch.color.FloatArray());
		
		glwprintf(L"%ls", batch.text.c_str());

		glPopMatrix();
	}

	m_Batches.clear();
}
