/* Copyright (C) 2024 Wildfire Games.
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

#include "COList.h"

#include "gui/CGUI.h"
#include "gui/IGUIScrollBar.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "gui/SettingTypes/CGUIList.h"
#include "i18n/L10n.h"
#include "ps/CLogger.h"

const float SORT_SPRITE_DIM = 16.0f;
const CVector2D COLUMN_SHIFT = CVector2D(0, 4);

const CStr COList::EventNameSelectionColumnChange = "SelectionColumnChange";

COList::COList(CGUI& pGUI)
	: CList(pGUI),
	  m_SpriteHeading(this, "sprite_heading"),
	  m_Sortable(this, "sortable"), // The actual sorting is done in JS for more versatility
	  m_SelectedColumn(this, "selected_column"),
	  m_SelectedColumnOrder(this, "selected_column_order"),
	  m_SpriteAsc(this, "sprite_asc"), // Show the order of sorting
	  m_SpriteDesc(this, "sprite_desc"),
	  m_SpriteNotSorted(this, "sprite_not_sorted")
{
}

void COList::SetupText()
{
	m_ItemsYPositions.resize(m_List->m_Items.size() + 1);

	// Delete all generated texts. Some could probably be saved,
	//  but this is easier, and this function will never be called
	//  continuously, or even often, so it'll probably be okay.
	m_GeneratedTexts.clear();

	m_TotalAvailableColumnWidth = GetListRect().GetWidth();
	// remove scrollbar if applicable
	if (m_ScrollBar && GetScrollBar(0).GetStyle())
		m_TotalAvailableColumnWidth -= GetScrollBar(0).GetStyle()->m_Width;

	m_HeadingHeight = SORT_SPRITE_DIM; // At least the size of the sorting sprite

	for (const COListColumn& column : m_Columns)
	{
		float width = column.m_Width;
		if (column.m_Width > 0 && column.m_Width < 1)
			width *= m_TotalAvailableColumnWidth;

		CGUIString gui_string;
		gui_string.SetValue(column.m_Heading);

		const CGUIText& text = AddText(gui_string, m_Font, width, m_BufferZone);
		m_HeadingHeight = std::max(m_HeadingHeight, text.GetSize().Height + COLUMN_SHIFT.Y);
	}

	// Generate texts
	float buffered_y = 0.f;

	for (size_t i = 0; i < m_List->m_Items.size(); ++i)
	{
		m_ItemsYPositions[i] = buffered_y;
		float shift = 0.0f;
		for (const COListColumn& column : m_Columns)
		{
			float width = column.m_Width;
			if (column.m_Width > 0 && column.m_Width < 1)
				width *= m_TotalAvailableColumnWidth;

			CGUIText* text;
			if (!column.m_List->m_Items[i].GetOriginalString().empty())
				text = &AddText(column.m_List->m_Items[i], m_Font, width, m_BufferZone);
			else
			{
				// Minimum height of a space character of the current font size
				CGUIString align_string;
				align_string.SetValue(L" ");
				text = &AddText(align_string, m_Font, width, m_BufferZone);
			}
			shift = std::max(shift, text->GetSize().Height);
		}
		buffered_y += shift;
	}

	m_ItemsYPositions[m_List->m_Items.size()] = buffered_y;

	if (m_ScrollBar)
	{
		CRect rect = GetListRect();
		GetScrollBar(0).SetScrollRange(m_ItemsYPositions.back());
		GetScrollBar(0).SetScrollSpace(rect.GetHeight());

		GetScrollBar(0).SetX(rect.right);
		GetScrollBar(0).SetY(rect.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(rect.bottom - rect.top);
	}
}

CRect COList::GetListRect() const
{
	return m_CachedActualSize + CRect(0, m_HeadingHeight, 0, 0);
}

void COList::HandleMessage(SGUIMessage& Message)
{
	CList::HandleMessage(Message);

	switch (Message.type)
	{

	case GUIM_SETTINGS_UPDATED:
	{
		if (Message.value.find("heading_") == 0)
			SetupText();
		break;
	}

	// If somebody clicks on the column heading
	case GUIM_MOUSE_PRESS_LEFT:
	{
		if (!m_Sortable)
			return;

		const CVector2D& mouse = m_pGUI.GetMousePos();
		if (!m_CachedActualSize.PointInside(mouse))
			return;

		float xpos = 0;
		for (const COListColumn& column : m_Columns)
		{
			if (column.m_Hidden)
				continue;

			float width = column.m_Width;
			// Check if it's a decimal value, and if so, assume relative positioning.
			if (column.m_Width < 1 && column.m_Width > 0)
				width *= m_TotalAvailableColumnWidth;
			CVector2D leftTopCorner = m_CachedActualSize.TopLeft() + CVector2D(xpos, 0);
			if (mouse.X >= leftTopCorner.X &&
				mouse.X < leftTopCorner.X + width &&
				mouse.Y < leftTopCorner.Y + m_HeadingHeight)
			{
				if (column.m_Id != static_cast<CStr>(m_SelectedColumn))
				{
					m_SelectedColumnOrder.Set(column.m_SortOrder, true);
					CStr selected_column = column.m_Id;
					m_SelectedColumn.Set(selected_column, true);
				}
				else
					m_SelectedColumnOrder.Set(-m_SelectedColumnOrder, true);

				ScriptEvent(EventNameSelectionColumnChange);
				PlaySound(m_SoundSelected);
				return;
			}
			xpos += width;
		}
		return;
	}
	default:
		return;
	}
}

bool COList::HandleAdditionalChildren(const XMBData& xmb, const XMBElement& child)
{
	#define ELMT(x) int elmt_##x = xmb.GetElementID(#x)
	#define ATTR(x) int attr_##x = xmb.GetAttributeID(#x)
	ELMT(item);
	ELMT(column);
	ELMT(translatableAttribute);
	ATTR(id);
	ATTR(context);

	if (child.GetNodeName() == elmt_item)
	{
		CGUIString vlist;
		vlist.SetValue(child.GetText().FromUTF8());
		AddItem(vlist, vlist);
		return true;
	}
	else if (child.GetNodeName() == elmt_column)
	{
		CStr id;
		XERO_ITER_ATTR(child, attr)
		{
			if (attr.Name == attr_id)
				id = attr.Value;
		}

		COListColumn column(this, id);

		for (XMBAttribute attr : child.GetAttributes())
		{
			std::string_view attr_name(xmb.GetAttributeStringView(attr.Name));
			CStr attr_value(attr.Value);

			if (attr_name == "textcolor")
			{
				if (!CGUI::ParseString<CGUIColor>(&m_pGUI, attr_value.FromUTF8(), column.m_TextColor))
					LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name.data(), attr_value.c_str());
			}
			else if (attr_name == "textcolor_selected")
			{
				if (!CGUI::ParseString<CGUIColor>(&m_pGUI, attr_value.FromUTF8(), column.m_TextColorSelected))
					LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name.data(), attr_value.c_str());
			}
			else if (attr_name == "hidden")
			{
				bool hidden = false;
				if (!CGUI::ParseString<bool>(&m_pGUI, attr_value.FromUTF8(), hidden))
					LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name.data(), attr_value.c_str());
				else
					column.m_Hidden.Set(hidden, false);
			}
			else if (attr_name == "width")
			{
				float width;
				if (!CGUI::ParseString<float>(&m_pGUI, attr_value.FromUTF8(), width))
					LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name.data(), attr_value.c_str());
				else
				{
					// Check if it's a relative value, and save as decimal if so.
					if (attr_value.find("%") != std::string::npos)
						width = width / 100.f;
					column.m_Width = width;
				}
			}
			else if (attr_name == "heading")
			{
				column.m_Heading.Set(attr_value.FromUTF8(), false);
			}
			else if (attr_name == "sort_order")
			{
				column.m_SortOrder.Set(attr_value == "desc" ? -1 : 1, false);
			}
		}

		for (XMBElement grandchild : child.GetChildNodes())
		{
			if (grandchild.GetNodeName() != elmt_translatableAttribute)
				continue;

			CStr attributeName(grandchild.GetAttributes().GetNamedItem(attr_id));
			// only the heading is translatable for list column
			if (attributeName.empty() || attributeName != "heading")
			{
				LOGERROR("GUI: translatable attribute in olist column that isn't a heading. (object: %s)", this->GetPresentableName().c_str());
				continue;
			}

			CStr value(grandchild.GetText());
			if (value.empty())
				continue;

			CStr context(grandchild.GetAttributes().GetNamedItem(attr_context)); // Read the context if any.
			if (!context.empty())
			{
				CStr translatedValue(g_L10n.TranslateWithContext(context, value));
				column.m_Heading.Set(translatedValue.FromUTF8(), false);
			}
			else
			{
				CStr translatedValue(g_L10n.Translate(value));
				column.m_Heading.Set(translatedValue.FromUTF8(), false);
			}
		}

		m_Columns.emplace_back(std::move(column));
		return true;
	}

	return false;
}

void COList::AdditionalChildrenHandled()
{
	SetupText();
}

void COList::DrawList(CCanvas2D& canvas, const int& selected, const CGUISpriteInstance& sprite, const CGUISpriteInstance& spriteOverlay,
                      const CGUISpriteInstance& spriteSelectArea, const CGUISpriteInstance& spriteSelectAreaOverlay, const CGUIColor& textColor)
{
	CRect rect = GetListRect();

	m_pGUI.DrawSprite(sprite, canvas, rect);

	float scroll = 0.f;
	if (m_ScrollBar)
		scroll = GetScrollBar(0).GetPos();

	bool drawSelected = false;
	CRect rectSel;
	// Draw item selection
	if (selected != -1)
	{
		ENSURE(selected >= 0 && selected+1 < (int)m_ItemsYPositions.size());

		// Get rectangle of selection:
		rectSel = CRect(
			rect.left, rect.top + m_ItemsYPositions[selected] - scroll,
			rect.right, rect.top + m_ItemsYPositions[selected+1] - scroll);

		if (rectSel.top <= rect.bottom &&
			rectSel.bottom >= rect.top)
		{
			if (rectSel.bottom > rect.bottom)
				rectSel.bottom = rect.bottom;
			if (rectSel.top < rect.top)
				rectSel.top = rect.top;

			if (m_ScrollBar)
			{
				// Remove any overlapping area of the scrollbar.
				if (rectSel.right > GetScrollBar(0).GetOuterRect().left &&
					rectSel.right <= GetScrollBar(0).GetOuterRect().right)
					rectSel.right = GetScrollBar(0).GetOuterRect().left;

				if (rectSel.left >= GetScrollBar(0).GetOuterRect().left &&
					rectSel.left < GetScrollBar(0).GetOuterRect().right)
					rectSel.left = GetScrollBar(0).GetOuterRect().right;
			}

			// Draw item selection
			m_pGUI.DrawSprite(spriteSelectArea, canvas, rectSel);
			drawSelected = true;
		}
	}

	// Draw line above column header
	CRect rect_head(m_CachedActualSize.left, m_CachedActualSize.top, m_CachedActualSize.right,
									m_CachedActualSize.top + m_HeadingHeight);
	m_pGUI.DrawSprite(m_SpriteHeading, canvas, rect_head);

	// Draw column headers
	float xpos = 0;
	size_t col = 0;
	for (const COListColumn& column : m_Columns)
	{
		if (column.m_Hidden)
		{
			++col;
			continue;
		}

		// Check if it's a decimal value, and if so, assume relative positioning.
		float width = column.m_Width;
		if (column.m_Width < 1 && column.m_Width > 0)
			width *= m_TotalAvailableColumnWidth;

		CVector2D leftTopCorner = m_CachedActualSize.TopLeft() + CVector2D(xpos, 0);

		// Draw sort arrows in colum header
		if (m_Sortable)
		{
			const CGUISpriteInstance* pSprite;
			if (*m_SelectedColumn == column.m_Id)
			{
				if (m_SelectedColumnOrder == 0)
					LOGERROR("selected_column_order must not be 0");

				if (m_SelectedColumnOrder != -1)
					pSprite = &*m_SpriteAsc;
				else
					pSprite = &*m_SpriteDesc;
			}
			else
				pSprite = &*m_SpriteNotSorted;

			m_pGUI.DrawSprite(*pSprite, canvas, CRect(leftTopCorner + CVector2D(width - SORT_SPRITE_DIM, 0), leftTopCorner + CVector2D(width, SORT_SPRITE_DIM)));
		}

		// Draw column header text
		DrawText(canvas, col, textColor, leftTopCorner + COLUMN_SHIFT, rect_head);
		xpos += width;
		++col;
	}

	// Draw list items for each column
	const size_t objectsCount = m_Columns.size();
	for (size_t i = 0; i < m_List->m_Items.size(); ++i)
	{
		if (m_ItemsYPositions[i+1] - scroll < 0 ||
			m_ItemsYPositions[i] - scroll > rect.GetHeight())
			continue;

		const float rowHeight = m_ItemsYPositions[i+1] - m_ItemsYPositions[i];

		// Clipping area (we'll have to substract the scrollbar)
		CRect cliparea = GetListRect();

		if (m_ScrollBar)
		{
			if (cliparea.right > GetScrollBar(0).GetOuterRect().left &&
				cliparea.right <= GetScrollBar(0).GetOuterRect().right)
				cliparea.right = GetScrollBar(0).GetOuterRect().left;

			if (cliparea.left >= GetScrollBar(0).GetOuterRect().left &&
				cliparea.left < GetScrollBar(0).GetOuterRect().right)
				cliparea.left = GetScrollBar(0).GetOuterRect().right;
		}

		// Draw all items for that column
		xpos = 0;
		for (size_t colIdx = 0; colIdx < m_Columns.size(); ++colIdx)
		{
			const COListColumn& column = m_Columns[colIdx];
			if (column.m_Hidden)
				continue;

			// Determine text position and width
			const CVector2D textPos = rect.TopLeft() + CVector2D(xpos, -scroll + m_ItemsYPositions[i]);

			float width = column.m_Width;
			// Check if it's a decimal value, and if so, assume relative positioning.
			if (column.m_Width < 1 && column.m_Width > 0)
				width *= m_TotalAvailableColumnWidth;

			// Clip text to the column (to prevent drawing text into the neighboring column)
			CRect cliparea2 = cliparea;
			cliparea2.right = std::min(cliparea2.right, textPos.X + width);
			cliparea2.bottom = std::min(cliparea2.bottom, textPos.Y + rowHeight);

			const CGUIColor& finalTextColor = (drawSelected && static_cast<size_t>(selected) == i && column.m_TextColorSelected) ? column.m_TextColorSelected : column.m_TextColor;

			// Draw list item
			DrawText(canvas, objectsCount * (i +/*Heading*/1) + colIdx, finalTextColor, textPos, cliparea2);
			xpos += width;
		}
	}

	// Draw scrollbars on top of the content
	if (m_ScrollBar)
		IGUIScrollBarOwner::Draw(canvas);

	// Draw the overlays last
	m_pGUI.DrawSprite(spriteOverlay, canvas, rect);
	if (drawSelected)
		m_pGUI.DrawSprite(spriteSelectAreaOverlay, canvas, rectSel);
}
