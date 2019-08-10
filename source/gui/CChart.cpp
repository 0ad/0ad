/* Copyright (C) 2019 Wildfire Games.
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

#include "CChart.h"

#include "gui/CGUIColor.h"
#include "gui/GUIMatrix.h"
#include "graphics/ShaderManager.h"
#include "i18n/L10n.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"
#include "renderer/Renderer.h"
#include "third_party/cppformat/format.h"

#include <cmath>

CChart::CChart(CGUI* pGUI)
	: IGUIObject(pGUI), IGUITextOwner(pGUI)
{
	AddSetting<CGUIColor>("axis_color");
	AddSetting<float>("axis_width");
	AddSetting<float>("buffer_zone");
	AddSetting<CStrW>("font");
	AddSetting<CStrW>("format_x");
	AddSetting<CStrW>("format_y");
	AddSetting<CGUIList>("series_color");
	AddSetting<CGUISeries>("series");
	AddSetting<EAlign>("text_align");

	GUI<float>::GetSetting(this, "axis_width", m_AxisWidth);
	GUI<CStrW>::GetSetting(this, "format_x", m_FormatX);
	GUI<CStrW>::GetSetting(this, "format_y", m_FormatY);
}

CChart::~CChart()
{
}

void CChart::HandleMessage(SGUIMessage& Message)
{
	// TODO: implement zoom
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
	{
		GUI<float>::GetSetting(this, "axis_width", m_AxisWidth);
		GUI<CStrW>::GetSetting(this, "format_x", m_FormatX);
		GUI<CStrW>::GetSetting(this, "format_y", m_FormatY);

		UpdateSeries();
		break;
	}
	}
}

void CChart::DrawLine(const CShaderProgramPtr& shader, const CGUIColor& color, const std::vector<float>& vertices) const
{
	shader->Uniform(str_color, color);
	shader->VertexPointer(3, GL_FLOAT, 0, &vertices[0]);
	shader->AssertPointersBound();

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.1f);
	if (!g_Renderer.m_SkipSubmit)
		glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 3);
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);
}

void CChart::DrawTriangleStrip(const CShaderProgramPtr& shader, const CGUIColor& color, const std::vector<float>& vertices) const
{
	shader->Uniform(str_color, color);
	shader->VertexPointer(3, GL_FLOAT, 0, &vertices[0]);
	shader->AssertPointersBound();

	if (!g_Renderer.m_SkipSubmit)
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size() / 3);
}

void CChart::DrawAxes(const CShaderProgramPtr& shader) const
{
	const float bz = GetBufferedZ();
	CRect rect = GetChartRect();
	std::vector<float> vertices;
	vertices.reserve(30);
#define ADD(x, y) vertices.push_back(x); vertices.push_back(y); vertices.push_back(bz + 0.5f);
	ADD(m_CachedActualSize.right, m_CachedActualSize.bottom);
	ADD(rect.right + m_AxisWidth, rect.bottom);
	ADD(m_CachedActualSize.left, m_CachedActualSize.bottom);
	ADD(rect.left, rect.bottom);
	ADD(m_CachedActualSize.left, m_CachedActualSize.top);
	ADD(rect.left, rect.top - m_AxisWidth);
#undef ADD
	CGUIColor axis_color(0.5f, 0.5f, 0.5f, 1.f);
	GUI<CGUIColor>::GetSetting(this, "axis_color", axis_color);
	DrawTriangleStrip(shader, axis_color, vertices);
}

void CChart::Draw()
{
	PROFILE3("render chart");

	if (m_Series.empty())
		return;

	const float bz = GetBufferedZ();
	CRect rect = GetChartRect();
	const float width = rect.GetWidth();
	const float height = rect.GetHeight();

	// Disable depth updates to prevent apparent z-fighting-related issues
	//  with some drivers causing units to get drawn behind the texture.
	glDepthMask(0);

	// Setup the render state
	CMatrix3D transform = GetDefaultGuiMatrix();
	CShaderDefines lineDefines;
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_gui_solid, g_Renderer.GetSystemShaderDefines(), lineDefines);
	tech->BeginPass();
	CShaderProgramPtr shader = tech->GetShader();
	shader->Uniform(str_transform, transform);

	CVector2D scale(width / (m_RightTop.X - m_LeftBottom.X), height / (m_RightTop.Y - m_LeftBottom.Y));
	for (const CChartData& data : m_Series)
	{
		if (data.m_Points.empty())
			continue;

		std::vector<float> vertices;
		for (const CVector2D& point : data.m_Points)
		{
			if (fabs(point.X) != std::numeric_limits<float>::infinity() && fabs(point.Y) != std::numeric_limits<float>::infinity())
			{
				vertices.push_back(rect.left + (point.X - m_LeftBottom.X) * scale.X);
				vertices.push_back(rect.bottom - (point.Y - m_LeftBottom.Y) * scale.Y);
				vertices.push_back(bz + 0.5f);
			}
			else
			{
				DrawLine(shader, data.m_Color, vertices);
				vertices.clear();
			}
		}
		if (!vertices.empty())
			DrawLine(shader, data.m_Color, vertices);
	}

	if (m_AxisWidth > 0)
		DrawAxes(shader);

	tech->EndPass();

	// Reset depth mask
	glDepthMask(1);

	for (size_t i = 0; i < m_TextPositions.size(); ++i)
		DrawText(i, CGUIColor(1.f, 1.f, 1.f, 1.f), m_TextPositions[i], bz + 0.5f);
}

CRect CChart::GetChartRect() const
{
	return CRect(
		m_CachedActualSize.TopLeft() + CPos(m_AxisWidth, m_AxisWidth),
		m_CachedActualSize.BottomRight() - CPos(m_AxisWidth, m_AxisWidth)
	);
}

void CChart::UpdateSeries()
{
	CGUISeries* pSeries;
	GUI<CGUISeries>::GetSettingPointer(this, "series", pSeries);

	CGUIList* pSeriesColor;
	GUI<CGUIList>::GetSettingPointer(this, "series_color", pSeriesColor);

	m_Series.clear();
	m_Series.resize(pSeries->m_Series.size());
	for (size_t i = 0; i < pSeries->m_Series.size(); ++i)
	{
		CChartData& data = m_Series[i];

		if (i < pSeriesColor->m_Items.size() && !GUI<int>::ParseColor(pSeriesColor->m_Items[i].GetOriginalString(), data.m_Color, 0))
			LOGWARNING("GUI: Error parsing 'series_color' (\"%s\")", utf8_from_wstring(pSeriesColor->m_Items[i].GetOriginalString()));

		data.m_Points = pSeries->m_Series[i];
	}
	UpdateBounds();

	SetupText();
}

void CChart::SetupText()
{
	for (SGUIText* t : m_GeneratedTexts)
		delete t;
	m_GeneratedTexts.clear();
	m_TextPositions.clear();

	if (m_Series.empty())
		return;

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		font = L"default";

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	// Add Y-axis
	GUI<CStrW>::GetSetting(this, "format_y", m_FormatY);
	const float height = GetChartRect().GetHeight();
	// TODO: split values depend on the format;
	if (m_EqualY)
	{
		// We don't need to generate many items for equal values
		AddFormattedValue(m_FormatY, m_RightTop.Y, font, buffer_zone);
		m_TextPositions.emplace_back(GetChartRect().TopLeft());
	}
	else
		for (int i = 0; i < 3; ++i)
		{
			AddFormattedValue(m_FormatY, m_RightTop.Y - (m_RightTop.Y - m_LeftBottom.Y) / 3.f * i, font, buffer_zone);
			m_TextPositions.emplace_back(GetChartRect().TopLeft() + CPos(0.f, height / 3.f * i));
		}

	// Add X-axis
	GUI<CStrW>::GetSetting(this, "format_x", m_FormatX);
	const float width = GetChartRect().GetWidth();
	if (m_EqualX)
	{
		CSize text_size = AddFormattedValue(m_FormatX, m_RightTop.X, font, buffer_zone);
		m_TextPositions.emplace_back(GetChartRect().BottomRight() - text_size);
	}
	else
		for (int i = 0; i < 3; ++i)
		{
			CSize text_size = AddFormattedValue(m_FormatX, m_RightTop.X - (m_RightTop.X - m_LeftBottom.X) / 3 * i, font, buffer_zone);
			m_TextPositions.emplace_back(GetChartRect().BottomRight() - text_size - CPos(width / 3 * i, 0.f));
		}
}

CSize CChart::AddFormattedValue(const CStrW& format, const float value, const CStrW& font, const float buffer_zone)
{
	// TODO: we need to catch cases with equal formatted values.
	CGUIString gui_str;
	if (format == L"DECIMAL2")
	{
		wchar_t buffer[64];
		swprintf(buffer, 64, L"%.2f", value);
		gui_str.SetValue(buffer);
	}
	else if (format == L"INTEGER")
	{
		wchar_t buffer[64];
		swprintf(buffer, 64, L"%d", std::lround(value));
		gui_str.SetValue(buffer);
	}
	else if (format == L"DURATION_SHORT")
	{
		const int seconds = value;
		wchar_t buffer[64];
		swprintf(buffer, 64, L"%d:%02d", seconds / 60, seconds % 60);
		gui_str.SetValue(buffer);
	}
	else if (format == L"PERCENTAGE")
	{
		wchar_t buffer[64];
		swprintf(buffer, 64, L"%d%%", std::lround(value));
		gui_str.SetValue(buffer);
	}
	else
	{
		LOGERROR("Unsupported chart format: " + format.EscapeToPrintableASCII());
		return CSize();
	}
	SGUIText* text = new SGUIText();
	*text = GetGUI()->GenerateText(gui_str, font, 0, buffer_zone, this);
	AddText(text);
	return text->m_Size;
}

void CChart::UpdateBounds()
{
	if (m_Series.empty() || m_Series[0].m_Points.empty())
	{
		m_LeftBottom = m_RightTop = CVector2D(0.f, 0.f);
		return;
	}

	m_LeftBottom = m_RightTop = m_Series[0].m_Points[0];
	for (const CChartData& data : m_Series)
		for (const CVector2D& point : data.m_Points)
		{
			if (fabs(point.X) != std::numeric_limits<float>::infinity() && point.X < m_LeftBottom.X)
				m_LeftBottom.X = point.X;
			if (fabs(point.Y) != std::numeric_limits<float>::infinity() && point.Y < m_LeftBottom.Y)
				m_LeftBottom.Y = point.Y;

			if (fabs(point.X) != std::numeric_limits<float>::infinity() && point.X > m_RightTop.X)
				m_RightTop.X = point.X;
			if (fabs(point.Y) != std::numeric_limits<float>::infinity() && point.Y > m_RightTop.Y)
				m_RightTop.Y = point.Y;
		}

	m_EqualY = m_RightTop.Y == m_LeftBottom.Y;
	if (m_EqualY)
		m_RightTop.Y += 1;
	m_EqualX = m_RightTop.X == m_LeftBottom.X;
	if (m_EqualX)
		m_RightTop.X += 1;
}
