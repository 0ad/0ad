/* Copyright (C) 2023 Wildfire Games.
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
#ifndef INCLUDED_COLIST
#define INCLUDED_COLIST

#include "gui/ObjectTypes/CList.h"
#include "gui/SettingTypes/CGUIColor.h"

#include <vector>

/**
 * Represents a column.
 */
class COListColumn
{
public:
	COListColumn(IGUIObject* owner, const CStr& cid)
		: m_Id(cid), m_Width(0), m_Heading(owner, "heading_" + cid), m_List(owner, "list_" + cid),
		m_Hidden(owner, "hidden_" + cid, false)
	{}
	// Avoid copying the strings.
	NONCOPYABLE(COListColumn);
	MOVABLE(COListColumn);
	CGUIColor m_TextColor;
	CStr m_Id;
	float m_Width;
	CGUISimpleSetting<CStrW> m_Heading; // CGUIString??
	CGUISimpleSetting<CGUIList> m_List;
	CGUISimpleSetting<bool> m_Hidden;
};

/**
 * Multi-column list. One row can be selected by the user.
 * Individual cells are clipped if the contained text is too long.
 *
 * The list can be sorted dynamically by JS code when a
 * heading is clicked.
 * A scroll-bar will appear when needed.
 */
class COList : public CList
{
	GUI_OBJECT(COList)

public:
	COList(CGUI& pGUI);

protected:
	void SetupText();
	void HandleMessage(SGUIMessage& Message);

	/**
	 * Handle the \<item\> tag.
	 */
	virtual bool HandleAdditionalChildren(const XMBData& xmb, const XMBElement& child);
	virtual void AdditionalChildrenHandled();

	virtual void DrawList(
		CCanvas2D& canvas, const int& selected, const CGUISpriteInstance& sprite, const CGUISpriteInstance& spriteOverlay,
		const CGUISpriteInstance& spriteSelectarea, const CGUISpriteInstance& spriteSelectAreaOverlay, const CGUIColor& textColor);

	virtual CRect GetListRect() const;

	/**
	 * Available columns.
	 */
	std::vector<COListColumn> m_Columns;

	CGUISimpleSetting<CGUISpriteInstance> m_SpriteHeading;
	CGUISimpleSetting<bool> m_Sortable;
	CGUISimpleSetting<CStr> m_SelectedColumn;
	CGUISimpleSetting<i32> m_SelectedColumnOrder;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteAsc;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteDesc;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteNotSorted;

private:
	static const CStr EventNameSelectionColumnChange;

	// Width of space available for columns
	float m_TotalAvailableColumnWidth;
	float m_HeadingHeight;
};

#endif // INCLUDED_COLIST
