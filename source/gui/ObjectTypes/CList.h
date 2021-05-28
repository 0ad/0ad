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

#ifndef INCLUDED_CLIST
#define INCLUDED_CLIST

#include "gui/CGUISprite.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectBases/IGUIScrollBarOwner.h"
#include "gui/ObjectBases/IGUITextOwner.h"
#include "gui/SettingTypes/CGUIList.h"

#include <vector>

/**
 * Create a list of elements, where one can be selected
 * by the user. The control will use a pre-processed
 * text-object for each element, which will be managed
 * by the IGUITextOwner structure.
 *
 * A scroll-bar will appear when needed. This will be
 * achieved with the IGUIScrollBarOwner structure.
 */
class CList : public IGUIObject, public IGUIScrollBarOwner, public IGUITextOwner
{
	GUI_OBJECT(CList)
public:
	CList(CGUI& pGUI);
	virtual ~CList();

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates();

	/**
	 * @see IGUIObject#UpdateCachedSize()
	 */
	virtual void UpdateCachedSize();

	/**
	 * Adds an item last to the list.
	 */
	virtual void AddItem(const CGUIString& str, const CGUIString& data);

	/**
	 * Add an item where both parameters are identical.
	 */
	void AddItem(const CGUIString& strAndData);

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 * @param append - if true, we assume we only need to render the new element at the end of the list.
	 */
	virtual void SetupText();
	virtual void SetupText(bool append);

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Handle events manually to catch keyboard inputting.
	 */
	virtual InReaction ManuallyHandleKeys(const SDL_Event_* ev);

	/**
	 * Draws the List box
	 */
	virtual void Draw(CCanvas2D& canvas);

	virtual void CreateJSObject();

	/**
	 * Easy select elements functions
	 */
	virtual void SelectNextElement();
	virtual void SelectPrevElement();
	virtual void SelectFirstElement();
	virtual void SelectLastElement();

	/**
	 * Handle the \<item\> tag.
	 */
	virtual bool HandleAdditionalChildren(const XMBData& xmb, const XMBElement& child);

	// Called every time the auto-scrolling should be checked.
	void UpdateAutoScroll();

	// Extended drawing interface, this is so that classes built on the this one
	//  can use other sprite names.
	virtual void DrawList(const int& selected, const CGUISpriteInstance& sprite, const CGUISpriteInstance& spriteOverlay,
	                      const CGUISpriteInstance& spriteSelectArea, const CGUISpriteInstance& spriteSelectAreaOverlay, const CGUIColor& textColor);

	// Get the area of the list. This is so that it can easily be changed, like in CDropDown
	//  where the area is not equal to m_CachedActualSize.
	virtual CRect GetListRect() const { return m_CachedActualSize; }

	// Returns whether SetupText() has run since the last message was received
	// (and thus whether list items have possibly changed).
	virtual bool GetModified() const { return m_Modified; }

	/**
	 * List of each element's relative y position. Will be
	 * one larger than m_Items, because it will end with the
	 * bottom of the last element. First element will always
	 * be zero, but still stored for easy handling.
	 */
	std::vector<float> m_ItemsYPositions;

	virtual int GetHoveredItem();

	CGUISimpleSetting<float> m_BufferZone;
	CGUISimpleSetting<CStrW> m_Font;
	CGUISimpleSetting<bool> m_ScrollBar;
	CGUISimpleSetting<CStr> m_ScrollBarStyle;
	CGUISimpleSetting<bool> m_ScrollBottom;
	CGUISimpleSetting<CStrW> m_SoundDisabled;
	CGUISimpleSetting<CStrW> m_SoundSelected;
	CGUISimpleSetting<CGUISpriteInstance> m_Sprite;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteOverlay;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteSelectArea;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteSelectAreaOverlay;
	CGUISimpleSetting<CGUIColor> m_TextColor;
	CGUISimpleSetting<CGUIColor> m_TextColorSelected;
	CGUISimpleSetting<i32> m_Selected;
	CGUISimpleSetting<bool> m_AutoScroll;
	CGUISimpleSetting<i32> m_Hovered;
	CGUISimpleSetting<CGUIList> m_List;
	CGUISimpleSetting<CGUIList> m_ListData;

private:
	static const CStr EventNameSelectionChange;
	static const CStr EventNameHoverChange;
	static const CStr EventNameMouseLeftClickItem;
	static const CStr EventNameMouseLeftDoubleClickItem;

	// Whether the list's items have been modified since last handling a message.
	bool m_Modified;

	// Used for doubleclick registration
	int m_PrevSelectedItem;

	// Last time a click on an item was issued
	double m_LastItemClickTime;
};

#endif // INCLUDED_CLIST
