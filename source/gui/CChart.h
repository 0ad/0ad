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

#ifndef INCLUDED_CCHART
#define INCLUDED_CCHART

#include "graphics/ShaderProgramPtr.h"
#include "gui/CGUIColor.h"
#include "gui/IGUITextOwner.h"
#include "maths/Vector2D.h"

#include <vector>

struct CChartData
{
	// Avoid copying the container.
	NONCOPYABLE(CChartData);
	MOVABLE(CChartData);
	CChartData() = default;

	CGUIColor m_Color;
	std::vector<CVector2D> m_Points;
};

/**
 * Chart for a data visualization as lines or points
 */
class CChart : public IGUITextOwner
{
	GUI_OBJECT(CChart)

public:
	CChart(CGUI& pGUI);
	virtual ~CChart();

protected:
	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Draws the Chart
	 */
	virtual void Draw();

	virtual CRect GetChartRect() const;

	void UpdateSeries();

	void SetupText();

	std::vector<CChartData> m_Series;

	CVector2D m_LeftBottom, m_RightTop;

	CStrW m_FormatX, m_FormatY;

	std::vector<CPos> m_TextPositions;

	float m_AxisWidth;

	bool m_EqualX, m_EqualY;

private:
	/**
	 * Helper functions
	 */
	void DrawLine(const CShaderProgramPtr& shader, const CGUIColor& color, const std::vector<float>& vertices) const;

	// Draws the triangle sequence so that the each next triangle has a common edge with the previous one.
	// If we need to draw n triangles, we need only n + 2 points.
	void DrawTriangleStrip(const CShaderProgramPtr& shader, const CGUIColor& color, const std::vector<float>& vertices) const;

	// Represents axes as triangles and draws them with DrawTriangleStrip.
	void DrawAxes(const CShaderProgramPtr& shader) const;

	CSize AddFormattedValue(const CStrW& format, const float value, const CStrW& font, const float buffer_zone);

	void UpdateBounds();
};

#endif // INCLUDED_CCHART
