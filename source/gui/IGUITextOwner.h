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

/*
GUI Object Base - Text Owner

--Overview--

	Interface class that enhance the IGUIObject with
	 cached CGUIStrings. This class is not at all needed,
	 and many controls that will use CGUIStrings might
	 not use this, but does help for regular usage such
	 as a text-box, a button, a radio button etc.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_IGUITEXTOWNER
#define INCLUDED_IGUITEXTOWNER

#include "GUI.h"

/**
 * Framework for handling Output text.
 *
 * @see IGUIObject
 */
class IGUITextOwner : virtual public IGUIObject
{
public:
	IGUITextOwner(CGUI* pGUI);
	virtual ~IGUITextOwner();

	/**
	 * Adds a text object.
	 */
	void AddText(SGUIText* text);

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * @see IGUIObject#UpdateCachedSize()
	 */
	virtual void UpdateCachedSize();

	/**
	 * Draws the Text.
	 *
	 * @param index Index value of text. Mostly this will be 0
	 * @param color
	 * @param pos Position
	 * @param z Z value
	 * @param clipping Clipping rectangle, don't even add a parameter
	 *		  to get no clipping.
	 */
	virtual void DrawText(size_t index, const CGUIColor& color, const CPos& pos, float z, const CRect& clipping = CRect());

	/**
	 * Test if mouse position is over an icon
	 */
	virtual bool MouseOverIcon();

protected:

	/**
	 * Setup texts. Functions that sets up all texts when changes have been made.
	 */
	virtual void SetupText() = 0;

	/**
	 * Whether the cached text is currently valid (if not then SetupText will be called by Draw)
	 */
	bool m_GeneratedTextsValid;

	/**
	 * Texts that are generated and ready to be rendered.
	 */
	std::vector<SGUIText*> m_GeneratedTexts;

	/**
	 * Calculate the position for the text, based on the alignment.
	 */
	void CalculateTextPosition(CRect& ObjSize, CPos& TextPos, SGUIText& Text);
};

#endif // INCLUDED_IGUITEXTOWNER
