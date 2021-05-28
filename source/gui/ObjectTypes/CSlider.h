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

#ifndef INCLUDED_CSLIDER
#define INCLUDED_CSLIDER

#include "gui/CGUISprite.h"
#include "gui/ObjectBases/IGUIButtonBehavior.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "maths/Vector2D.h"

class CSlider : public IGUIObject, public IGUIButtonBehavior
{
	GUI_OBJECT(CSlider)

public:
	CSlider(CGUI& pGUI);
	virtual ~CSlider();

protected:
	static const CStr EventNameValueChange;

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	virtual void Draw(CCanvas2D& canvas);

	/**
	 * Change settings and send the script event
	 */
	void UpdateValue();

	CRect GetButtonRect() const;

	/**
	 * @return ratio between the value of the slider and its actual size in the GUI
	 */
	float GetSliderRatio() const;

	void IncrementallyChangeValue(const float value);

	// Settings
	CGUISimpleSetting<float> m_ButtonSide;
	CGUISimpleSetting<float> m_MinValue;
	CGUISimpleSetting<float> m_MaxValue;
	CGUISimpleSetting<CGUISpriteInstance> m_Sprite;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteBar;
	CGUISimpleSetting<float> m_Value;

private:
	CVector2D m_Mouse;
};

#endif // INCLUDED_CSLIDER
