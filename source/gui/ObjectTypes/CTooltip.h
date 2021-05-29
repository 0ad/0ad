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

#ifndef INCLUDED_CTOOLTIP
#define INCLUDED_CTOOLTIP

#include "gui/CGUISprite.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectBases/IGUITextOwner.h"
#include "gui/SettingTypes/CGUIString.h"
#include "maths/Vector2D.h"

/**
 * Dynamic tooltips. Similar to CText.
 */
class CTooltip : public IGUIObject, public IGUITextOwner
{
	GUI_OBJECT(CTooltip)

public:
	CTooltip(CGUI& pGUI);
	virtual ~CTooltip();

	const CStr& GetUsedObject() const { return m_UseObject; }
	i32 GetTooltipDelay() const { return m_Delay; }
	bool ShouldHideObject() const { return m_HideObject; }
	void SetMousePos(const CVector2D& vec) { m_MousePos.Set(vec, true); }

protected:
	void SetupText();

	/**
	 * @see IGUIObject#UpdateCachedSize()
	 */
	void UpdateCachedSize();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	virtual void Draw(CCanvas2D& canvas);

	virtual float GetBufferedZ() const;

	CGUISimpleSetting<float> m_BufferZone;
	CGUISimpleSetting<CGUIString> m_Caption;
	CGUISimpleSetting<CStrW> m_Font;
	CGUISimpleSetting<CGUISpriteInstance> m_Sprite;
	CGUISimpleSetting<i32> m_Delay;
	CGUISimpleSetting<CGUIColor> m_TextColor;
	CGUISimpleSetting<float> m_MaxWidth;
	CGUISimpleSetting<CVector2D> m_Offset;
	CGUISimpleSetting<EVAlign> m_Anchor;
	CGUISimpleSetting<bool> m_Independent;
	CGUISimpleSetting<CVector2D> m_MousePos;
	CGUISimpleSetting<CStr> m_UseObject;
	CGUISimpleSetting<bool> m_HideObject;
};

#endif // INCLUDED_CTOOLTIP
