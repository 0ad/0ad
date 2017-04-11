/* Copyright (C) 2017 Wildfire Games.
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
#include "lib/ogl.h"
#include "ps/CLogger.h"
#include "renderer/Renderer.h"

#include <cmath>

CChart::CChart()
{
	AddSetting(GUIST_CGUIList, "series_color");
	AddSetting(GUIST_CGUISeries, "series");
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
		UpdateSeries();
		break;
	}
	}

}

void CChart::DrawLine(const CShaderProgramPtr& shader, const CColor& color, const std::vector<float>& vertices) const
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

void CChart::Draw()
{
	PROFILE3("render chart");

	if (!GetGUI())
		return;

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

	CVector2D leftBottom, rightTop;
	leftBottom = rightTop = m_Series[0].m_Points[0];
	for (const CChartData& data : m_Series)
		for (const CVector2D& point : data.m_Points)
		{
			if (fabs(point.X) != std::numeric_limits<float>::infinity() && point.X < leftBottom.X)
				leftBottom.X = point.X;
			if (fabs(point.Y) != std::numeric_limits<float>::infinity() && point.Y < leftBottom.Y)
				leftBottom.Y = point.Y;

			if (fabs(point.X) != std::numeric_limits<float>::infinity() && point.X > rightTop.X)
				rightTop.X = point.X;
			if (fabs(point.Y) != std::numeric_limits<float>::infinity() && point.Y > rightTop.Y)
				rightTop.Y = point.Y;
		}

	if (rightTop.Y == leftBottom.Y)
		rightTop.Y += 1;
	if (rightTop.X == leftBottom.X)
		rightTop.X += 1;

	CVector2D scale(width / (rightTop.X - leftBottom.X), height / (rightTop.Y - leftBottom.Y));

	for (const CChartData& data : m_Series)
	{
		if (data.m_Points.empty())
			continue;

		std::vector<float> vertices;
		for (const CVector2D& point : data.m_Points)
		{
			if (fabs(point.Y) != std::numeric_limits<float>::infinity() && fabs(point.Y) != std::numeric_limits<float>::infinity())
			{
				vertices.push_back(rect.left + (point.X - leftBottom.X) * scale.X);
				vertices.push_back(rect.bottom - (point.Y - leftBottom.Y) * scale.Y);
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

	tech->EndPass();

	// Reset depth mask
	glDepthMask(1);
}

CRect CChart::GetChartRect() const
{
	return m_CachedActualSize;
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
}
