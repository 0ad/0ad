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

#ifndef INCLUDED_CSLIDER
#define INCLUDED_CSLIDER

#include "GUI.h"


class CSlider : public IGUIObject
{
	GUI_OBJECT(CSlider)

public:
	CSlider();
	virtual ~CSlider();

protected:

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	virtual void Draw();

	/**
	 * Change settings and send the script event
	 */
	void UpdateValue();

	CRect GetButtonRect();

	/**
	 * @return ratio between the value of the slider and its actual size in the GUI
	 */
	float GetSliderRatio() const;

	void IncrementallyChangeValue(const float value);

	float m_MinValue, m_MaxValue, m_Value;

private:
	bool m_IsPressed;

	CPos m_Mouse;

	float m_ButtonSide;
};

#endif // INCLUDED_CSLIDER
