/* Copyright (C) 2020 Wildfire Games.
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

#include "graphics/ShaderManager.h"
#include "gui/GUIMatrix.h"
#include "gui/SettingTypes/CGUIList.h"
#include "gui/SettingTypes/CGUISeries.h"
#include "gui/SettingTypes/CGUIString.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"

#include <cmath>

CChart::CChart(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  m_AxisColor(),
	  m_AxisWidth(),
	  m_BufferZone(),
	  m_Font(),
	  m_FormatX(),
	  m_FormatY(),
	  m_SeriesColor(),
	  m_SeriesSetting(),
	  m_TextAlign()
{
	RegisterSetting("axis_color", m_AxisColor);
	RegisterSetting("axis_width", m_AxisWidth);
	RegisterSetting("buffer_zone", m_BufferZone);
	RegisterSetting("font", m_Font);
	RegisterSetting("format_x", m_FormatX);
	RegisterSetting("format_y", m_FormatY);
	RegisterSetting("series_color", m_SeriesColor);
	RegisterSetting("series", m_SeriesSetting);
	RegisterSetting("text_align", m_TextAlign);
}

CChart::~CChart()
{
}

void CChart::UpdateCachedSize()
{
	IGUIObject::UpdateCachedSize();
	IGUITextOwner::UpdateCachedSize();
}

void CChart::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	// IGUITextOwner::HandleMessage(Message); performed in UpdateSeries

	// TODO: implement zoom
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
	{
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

#if !CONFIG2_GLES
	glEnable(GL_LINE_SMOOTH);
#endif
	glLineWidth(1.1f);
	if (!g_Renderer.m_SkipSubmit)
		glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 3);
	glLineWidth(1.0f);
#if !CONFIG2_GLES
	glDisable(GL_LINE_SMOOTH);
#endif
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
	DrawTriangleStrip(shader, m_AxisColor, vertices);
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
	m_Series.clear();
	m_Series.resize(m_SeriesSetting.m_Series.size());

	for (size_t i = 0; i < m_SeriesSetting.m_Series.size(); ++i)
	{
		CChartData& data = m_Series[i];

		if (i < m_SeriesColor.m_Items.size() && !data.m_Color.ParseString(m_pGUI, m_SeriesColor.m_Items[i].GetOriginalString().ToUTF8(), 0))
			LOGWARNING("GUI: Error parsing 'series_color' (\"%s\")", utf8_from_wstring(m_SeriesColor.m_Items[i].GetOriginalString()));

		data.m_Points = m_SeriesSetting.m_Series[i];
	}
	UpdateBounds();

	SetupText();
}

void CChart::SetupText()
{
	m_GeneratedTexts.clear();
	m_TextPositions.clear();

	if (m_Series.empty())
		return;

	// Add Y-axis
	const float height = GetChartRect().GetHeight();
	// TODO: split values depend on the format;
	if (m_EqualY)
	{
		// We don't need to generate many items for equal values
		AddFormattedValue(m_FormatY, m_RightTop.Y, m_Font, m_BufferZone);
		m_TextPositions.emplace_back(GetChartRect().TopLeft());
	}
	else
		for (int i = 0; i < 3; ++i)
		{
			AddFormattedValue(m_FormatY, m_RightTop.Y - (m_RightTop.Y - m_LeftBottom.Y) / 3.f * i, m_Font, m_BufferZone);
			m_TextPositions.emplace_back(GetChartRect().TopLeft() + CPos(0.f, height / 3.f * i));
		}

	// Add X-axis
	const float width = GetChartRect().GetWidth();
	if (m_EqualX)
	{
		CSize text_size = AddFormattedValue(m_FormatX, m_RightTop.X, m_Font, m_BufferZone);
		m_TextPositions.emplace_back(GetChartRect().BottomRight() - text_size);
	}
	else
		for (int i = 0; i < 3; ++i)
		{
			CSize text_size = AddFormattedValue(m_FormatX, m_RightTop.X - (m_RightTop.X - m_LeftBottom.X) / 3 * i, m_Font, m_BufferZone);
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

	return AddText(gui_str, font, 0, buffer_zone).GetSize();
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
