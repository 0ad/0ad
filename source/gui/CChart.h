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

#ifndef INCLUDED_CCHART
#define INCLUDED_CCHART

#include "GUI.h"
#include "graphics/Color.h"
#include "maths/Vector2D.h"
#include <vector>


struct CChartData
{
	CColor m_Color;
	std::vector<CVector2D> m_Points;
};

/**
 * Chart for a data visualization as lines or points
 *
 * @see IGUIObject
 */
class CChart : public IGUIObject
{
	GUI_OBJECT(CChart)

public:
	CChart();
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

	std::vector<CChartData> m_Series;

private:
	/**
	 * Helper function
	 */
	void DrawLine(const CShaderProgramPtr& shader, const CColor& color, const std::vector<float>& vertices) const;
};

#endif // INCLUDED_CCHART
