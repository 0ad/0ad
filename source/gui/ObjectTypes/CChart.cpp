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

#include "CChart.h"

#include "graphics/Canvas2D.h"
#include "gui/GUIMatrix.h"
#include "gui/SettingTypes/CGUIList.h"
#include "gui/SettingTypes/CGUISeries.h"
#include "gui/SettingTypes/CGUIString.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Profile.h"

#include <cmath>

CChart::CChart(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  m_AxisColor(this, "axis_color"),
	  m_AxisWidth(this, "axis_width"),
	  m_BufferZone(this, "buffer_zone"),
	  m_Font(this, "font"),
	  m_FormatX(this, "format_x"),
	  m_FormatY(this, "format_y"),
	  m_SeriesColor(this, "series_color"),
	  m_SeriesSetting(this, "series")
{
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
	if(Message.type == GUIM_SETTINGS_UPDATED)
		UpdateSeries();
}

void CChart::DrawAxes(CCanvas2D& canvas) const
{
	canvas.DrawRect(CRect(
		m_CachedActualSize.TopLeft(),
		m_CachedActualSize.BottomLeft() + CVector2D(m_AxisWidth, 0.0f)), m_AxisColor);
	canvas.DrawRect(CRect(
		m_CachedActualSize.BottomLeft() - CVector2D(0.0f, m_AxisWidth),
		m_CachedActualSize.BottomRight()), m_AxisColor);
}

void CChart::Draw(CCanvas2D& canvas)
{
	PROFILE3("render chart");

	if (m_Series.empty())
		return;

	CRect rect = GetChartRect();
	const float width = rect.GetWidth();
	const float height = rect.GetHeight();

	CVector2D scale(width / (m_RightTop.X - m_LeftBottom.X), height / (m_RightTop.Y - m_LeftBottom.Y));
	std::vector<CVector2D> linePoints;
	for (const CChartData& data : m_Series)
	{
		if (data.m_Points.empty())
			continue;
		
		linePoints.clear();
		for (const CVector2D& point : data.m_Points)
		{
			if (fabs(point.X) != std::numeric_limits<float>::infinity() && fabs(point.Y) != std::numeric_limits<float>::infinity())
			{
				linePoints.emplace_back(
					rect.left + (point.X - m_LeftBottom.X) * scale.X,
					rect.bottom - (point.Y - m_LeftBottom.Y) * scale.Y);
			}
			else
			{
				canvas.DrawLine(linePoints, 2.0f, data.m_Color);
				linePoints.clear();
			}
		}
		if (!linePoints.empty())
			canvas.DrawLine(linePoints, 2.0f, data.m_Color);
	}

	if (m_AxisWidth > 0)
		DrawAxes(canvas);

	for (size_t i = 0; i < m_TextPositions.size(); ++i)
		DrawText(canvas, i, CGUIColor(1.f, 1.f, 1.f, 1.f), m_TextPositions[i]);
}

CRect CChart::GetChartRect() const
{
	return CRect(
		m_CachedActualSize.TopLeft() + CVector2D(m_AxisWidth, m_AxisWidth),
		m_CachedActualSize.BottomRight() - CVector2D(m_AxisWidth, m_AxisWidth)
	);
}

void CChart::UpdateSeries()
{
	m_Series.clear();
	m_Series.resize(m_SeriesSetting->m_Series.size());

	for (size_t i = 0; i < m_SeriesSetting->m_Series.size(); ++i)
	{
		CChartData& data = m_Series[i];

		if (i < m_SeriesColor->m_Items.size() && !data.m_Color.ParseString(m_pGUI, m_SeriesColor->m_Items[i].GetOriginalString().ToUTF8(), 0))
			LOGWARNING("GUI: Error parsing 'series_color' (\"%s\")", utf8_from_wstring(m_SeriesColor->m_Items[i].GetOriginalString()));

		data.m_Points = m_SeriesSetting->m_Series[i];
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
			m_TextPositions.emplace_back(GetChartRect().TopLeft() + CVector2D(0.f, height / 3.f * i));
		}

	// Add X-axis
	const float width = GetChartRect().GetWidth();
	if (m_EqualX)
	{
		CSize2D text_size = AddFormattedValue(m_FormatX, m_RightTop.X, m_Font, m_BufferZone);
		m_TextPositions.emplace_back(GetChartRect().BottomRight() - text_size);
	}
	else
		for (int i = 0; i < 3; ++i)
		{
			CSize2D text_size = AddFormattedValue(m_FormatX, m_RightTop.X - (m_RightTop.X - m_LeftBottom.X) / 3 * i, m_Font, m_BufferZone);
			m_TextPositions.emplace_back(GetChartRect().BottomRight() - text_size - CVector2D(width / 3 * i, 0.f));
		}
}

CSize2D CChart::AddFormattedValue(const CStrW& format, const float value, const CStrW& font, const float buffer_zone)
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
		return CSize2D();
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
