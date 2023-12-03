/* Copyright (C) 2021 Wildfire Games.
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

#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectBases/IGUITextOwner.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "gui/SettingTypes/CGUIList.h"
#include "gui/SettingTypes/CGUISeries.h"
#include "maths/Size2D.h"
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
class CChart : public IGUIObject, public IGUITextOwner
{
	GUI_OBJECT(CChart)

public:
	CChart(CGUI& pGUI);
	virtual ~CChart();

protected:
	/**
	 * @see IGUIObject#UpdateCachedSize()
	 */
	void UpdateCachedSize();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Draws the Chart
	 */
	virtual void Draw(CCanvas2D& canvas);

	virtual CRect GetChartRect() const;

	void UpdateSeries();

	void SetupText();

	std::vector<CChartData> m_Series;

	CVector2D m_LeftBottom, m_RightTop;

	std::vector<CVector2D> m_TextPositions;

	bool m_EqualX, m_EqualY;

	// Settings
	CGUISimpleSetting<CGUIColor> m_AxisColor;
	CGUISimpleSetting<float> m_AxisWidth;
	CGUISimpleSetting<float> m_BufferZone;
	CGUISimpleSetting<CStrW> m_Font;
	CGUISimpleSetting<CStrW> m_FormatX;
	CGUISimpleSetting<CStrW> m_FormatY;
	CGUISimpleSetting<CGUIList> m_SeriesColor;
	CGUISimpleSetting<CGUISeries> m_SeriesSetting;

private:
	void DrawAxes(CCanvas2D& canvas) const;

	CSize2D AddFormattedValue(const CStrW& format, const float value, const CStrW& font, const float buffer_zone);

	void UpdateBounds();
};

#endif // INCLUDED_CCHART
