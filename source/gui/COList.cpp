/* Copyright (C) 2016 Wildfire Games.
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

#include "i18n/L10n.h"
#include "ps/CLogger.h"
#include "soundmanager/ISoundManager.h"

COList::COList()
	: CList(), m_HeadingHeight(30.f), m_SelectedDef(-1), m_SelectedColumnOrder(0)
{
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_heading");
	AddSetting(GUIST_bool,					"sortable"); // The actual sorting is done in JS for more versatility
	AddSetting(GUIST_CStr,					"selected_column");
	AddSetting(GUIST_int,					"selected_column_order");
	AddSetting(GUIST_CStr,					"default_column");
	AddSetting(GUIST_int,					"default_column_order");
	AddSetting(GUIST_int,					"selected_def");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_asc");  // Show the order of sorting
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_desc");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_not_sorted");

	GUI<CStr>::SetSetting(this, "selected_column", "");
	GUI<int>::SetSetting(this, "selected_column_order", 0);
	GUI<int>::SetSetting(this, "selected_def", -1);
}

void COList::SetupText()
{
	if (!GetGUI())
		return;

	CGUIList* pList;
	GUI<CGUIList>::GetSettingPointer(this, "list_name", pList);

	m_ItemsYPositions.resize(pList->m_Items.size() + 1);

	// Delete all generated texts. Some could probably be saved,
	//  but this is easier, and this function will never be called
	//  continuously, or even often, so it'll probably be okay.
	for (SGUIText* const& t : m_GeneratedTexts)
		delete t;
	m_GeneratedTexts.clear();

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Don't define standard like this. Do it with the default style.
		font = L"default";

	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	float width = GetListRect().GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	m_TotalAvalibleColumnWidth = width;

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	CStr defaultColumn;
	GUI<CStr>::GetSetting(this, "default_column", defaultColumn);
	defaultColumn = "list_" + defaultColumn;

	for (size_t c = 0; c < m_ObjectsDefs.size(); ++c)
	{
		SGUIText* text = new SGUIText();
		CGUIString gui_string;
		gui_string.SetValue(m_ObjectsDefs[c].m_Heading);
		*text = GetGUI()->GenerateText(gui_string, font, width, buffer_zone, this);
		AddText(text);

		if (m_SelectedDef == (size_t)-1 && defaultColumn == m_ObjectsDefs[c].m_Id)
			m_SelectedDef = c;
	}

	if (m_SelectedDef != (size_t)-1)
		GUI<CStr>::SetSetting(this, "selected_column", m_ObjectsDefs[m_SelectedDef].m_Id.substr(5));

	if (m_SelectedColumnOrder == 0)
	{
		GUI<int>::GetSetting(this, "default_column_order", m_SelectedColumnOrder);
		GUI<int>::SetSetting(this, "selected_column_order", m_SelectedColumnOrder);
	}

	// Generate texts
	float buffered_y = 0.f;

	for (size_t i = 0; i < pList->m_Items.size(); ++i)
	{
		m_ItemsYPositions[i] = buffered_y;
		for (size_t c = 0; c < m_ObjectsDefs.size(); ++c)
		{
			CGUIList* pList_c;
			GUI<CGUIList>::GetSettingPointer(this, m_ObjectsDefs[c].m_Id, pList_c);
			SGUIText* text = new SGUIText();
			*text = GetGUI()->GenerateText(pList_c->m_Items[i], font, width, buffer_zone, this);
			if (c == 0)
				buffered_y += text->m_Size.cy;
			AddText(text);
		}
	}

	m_ItemsYPositions[pList->m_Items.size()] = buffered_y;

	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange(m_ItemsYPositions.back());
		GetScrollBar(0).SetScrollSpace(GetListRect().GetHeight());

		CRect rect = GetListRect();
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
	// If somebody clicks on the column heading
	case GUIM_MOUSE_PRESS_LEFT:
	{
		bool sortable;
		GUI<bool>::GetSetting(this, "sortable", sortable);
		if (!sortable)
			return;

		CPos mouse = GetMousePos();
		if (!m_CachedActualSize.PointInside(mouse))
			return;

		float xpos = 0;
		for (size_t def = 0; def < m_ObjectsDefs.size(); ++def)
		{
			float width = m_ObjectsDefs[def].m_Width;
			// Check if it's a decimal value, and if so, assume relative positioning.
			if (m_ObjectsDefs[def].m_Width < 1 && m_ObjectsDefs[def].m_Width > 0)
				width *= m_TotalAvalibleColumnWidth;
			CPos leftTopCorner = m_CachedActualSize.TopLeft() + CPos(xpos, 0);
			if (mouse.x >= leftTopCorner.x &&
				mouse.x < leftTopCorner.x + width &&
				mouse.y < leftTopCorner.y + m_HeadingHeight)
			{
				if (def != m_SelectedDef)
				{
					m_SelectedColumnOrder = 1;
					m_SelectedDef = def;
				}
				else
					m_SelectedColumnOrder = -m_SelectedColumnOrder;
				GUI<CStr>::SetSetting(this, "selected_column", m_ObjectsDefs[def].m_Id.substr(5));
				GUI<int>::SetSetting(this, "selected_column_order", m_SelectedColumnOrder);
				GUI<int>::SetSetting(this, "selected_def", def);
				ScriptEvent("selectioncolumnchange");

				CStrW soundPath;
				if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_selected", soundPath) == PSRETURN_OK && !soundPath.empty())
					g_SoundManager->PlayAsUI(soundPath.c_str(), false);

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

bool COList::HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile)
{
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(item);
	ELMT(def);
	ELMT(translatableAttribute);
	ATTR(id);
	ATTR(context);

	if (child.GetNodeName() == elmt_item)
	{
		AddItem(child.GetText().FromUTF8(), child.GetText().FromUTF8());
		return true;
	}
	else if (child.GetNodeName() == elmt_def)
	{
		ObjectDef oDef;

		for (XMBAttribute attr : child.GetAttributes())
		{
			CStr attr_name(pFile->GetAttributeString(attr.Name));
			CStr attr_value(attr.Value);

			if (attr_name == "color")
			{
				CColor color;
				if (!GUI<CColor>::ParseString(attr_value.FromUTF8(), color))
					LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name.c_str(), attr_value.c_str());
				else oDef.m_TextColor = color;
			}
			else if (attr_name == "id")
			{
				oDef.m_Id = "list_"+attr_value;
			}
			else if (attr_name == "width")
			{
				float width;
				if (!GUI<float>::ParseString(attr_value.FromUTF8(), width))
					LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name.c_str(), attr_value.c_str());
				else
				{
					// Check if it's a relative value, and save as decimal if so.
					if (attr_value.find("%") != std::string::npos)
						width = width / 100.f;
					oDef.m_Width = width;
				}
			}
			else if (attr_name == "heading")
			{
				oDef.m_Heading = attr_value.FromUTF8();
			}
		}

		for (XMBElement grandchild : child.GetChildNodes())
		{
			if (grandchild.GetNodeName() != elmt_translatableAttribute)
				continue;

			CStr attributeName(grandchild.GetAttributes().GetNamedItem(attr_id));
			// only the heading is translatable for list defs
			if (attributeName.empty() || attributeName != "heading")
			{
				LOGERROR("GUI: translatable attribute in olist def that isn't a heading. (object: %s)", this->GetPresentableName().c_str());
				continue;
			}

			CStr value(grandchild.GetText());
			if (value.empty())
				continue;

			CStr context(grandchild.GetAttributes().GetNamedItem(attr_context)); // Read the context if any.
			if (!context.empty())
			{
				CStr translatedValue(g_L10n.TranslateWithContext(context, value));
				oDef.m_Heading = translatedValue.FromUTF8();
			}
			else
			{
				CStr translatedValue(g_L10n.Translate(value));
				oDef.m_Heading = translatedValue.FromUTF8();
			}
		}

		m_ObjectsDefs.push_back(oDef);

		AddSetting(GUIST_CGUIList, oDef.m_Id);
		SetupText();

		return true;
	}
	else
	{
		return false;
	}
}

void COList::DrawList(const int& selected, const CStr& _sprite, const CStr& _sprite_selected, const CStr& _textcolor)
{
	float bz = GetBufferedZ();

	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	if (scrollbar)
		IGUIScrollBarOwner::Draw();

	if (!GetGUI())
		return;

	CRect rect = GetListRect();

	CGUISpriteInstance* sprite = NULL;
	CGUISpriteInstance* sprite_selectarea = NULL;
	int cell_id;
	GUI<CGUISpriteInstance>::GetSettingPointer(this, _sprite, sprite);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, _sprite_selected, sprite_selectarea);
	GUI<int>::GetSetting(this, "cell_id", cell_id);

	CGUIList* pList;
	GUI<CGUIList>::GetSettingPointer(this, "list_name", pList);

	GetGUI()->DrawSprite(*sprite, cell_id, bz, rect);

	float scroll = 0.f;
	if (scrollbar)
		scroll = GetScrollBar(0).GetPos();

	// Draw item selection
	if (selected != -1)
	{
		ENSURE(selected >= 0 && selected+1 < (int)m_ItemsYPositions.size());

		// Get rectangle of selection:
		CRect rect_sel(rect.left, rect.top + m_ItemsYPositions[selected] - scroll,
		               rect.right, rect.top + m_ItemsYPositions[selected+1] - scroll);

		if (rect_sel.top <= rect.bottom &&
			rect_sel.bottom >= rect.top)
		{
			if (rect_sel.bottom > rect.bottom)
				rect_sel.bottom = rect.bottom;
			if (rect_sel.top < rect.top)
				rect_sel.top = rect.top;

			if (scrollbar)
			{
				// Remove any overlapping area of the scrollbar.
				if (rect_sel.right > GetScrollBar(0).GetOuterRect().left &&
					rect_sel.right <= GetScrollBar(0).GetOuterRect().right)
					rect_sel.right = GetScrollBar(0).GetOuterRect().left;

				if (rect_sel.left >= GetScrollBar(0).GetOuterRect().left &&
					rect_sel.left < GetScrollBar(0).GetOuterRect().right)
					rect_sel.left = GetScrollBar(0).GetOuterRect().right;
			}

			// Draw item selection
			GetGUI()->DrawSprite(*sprite_selectarea, cell_id, bz+0.05f, rect_sel);
		}
	}

	CColor color;
	GUI<CColor>::GetSetting(this, _textcolor, color);

	// Draw line above column header
	CGUISpriteInstance* sprite_heading = NULL;
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_heading", sprite_heading);
	CRect rect_head(m_CachedActualSize.left, m_CachedActualSize.top, m_CachedActualSize.right,
									m_CachedActualSize.top + m_HeadingHeight);
	GetGUI()->DrawSprite(*sprite_heading, cell_id, bz, rect_head);

	CGUISpriteInstance* sprite_order;
	CGUISpriteInstance* sprite_not_sorted;
	if (m_SelectedColumnOrder != -1)
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_asc", sprite_order);
	else
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_desc", sprite_order);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_not_sorted", sprite_not_sorted);

	// Draw column headers
	float xpos = 0;
	for (size_t def = 0; def < m_ObjectsDefs.size(); ++def)
	{
		// Check if it's a decimal value, and if so, assume relative positioning.
		float width = m_ObjectsDefs[def].m_Width;
		if (m_ObjectsDefs[def].m_Width < 1 && m_ObjectsDefs[def].m_Width > 0)
			width *= m_TotalAvalibleColumnWidth;

		CPos leftTopCorner = m_CachedActualSize.TopLeft() + CPos(xpos, 0);

		CGUISpriteInstance* sprite;
		// If the list sorted by current column
		if (m_SelectedDef == def)
			sprite = sprite_order;
		else
			sprite = sprite_not_sorted;

		// Draw sort arrows in colum header
		GetGUI()->DrawSprite(*sprite, cell_id, bz + 0.1f, CRect(leftTopCorner + CPos(width - 16, 0), leftTopCorner + CPos(width, 16)));

		// Draw column header text
		DrawText(def, color, leftTopCorner + CPos(0, 4), bz + 0.1f, rect_head);
		xpos += width;
	}

	// Draw list items for each column
	const size_t objectsCount = m_ObjectsDefs.size();
	for (size_t i = 0; i < pList->m_Items.size(); ++i)
	{
		if (m_ItemsYPositions[i+1] - scroll < 0 ||
			m_ItemsYPositions[i] - scroll > rect.GetHeight())
			continue;

		const float rowHeight = m_ItemsYPositions[i+1] - m_ItemsYPositions[i];

		// Clipping area (we'll have to substract the scrollbar)
		CRect cliparea = GetListRect();

		if (scrollbar)
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
		for (size_t def = 0; def < objectsCount; ++def)
		{
			// Determine text position and width
			const CPos textPos = rect.TopLeft() + CPos(xpos, -scroll + m_ItemsYPositions[i]);

			float width = m_ObjectsDefs[def].m_Width;;
			// Check if it's a decimal value, and if so, assume relative positioning.
			if (m_ObjectsDefs[def].m_Width < 1 && m_ObjectsDefs[def].m_Width > 0)
				width *= m_TotalAvalibleColumnWidth;

			// Clip text to the column (to prevent drawing text into the neighboring column)
			CRect cliparea2 = cliparea;
			cliparea2.right = std::min(cliparea2.right, textPos.x + width);
			cliparea2.bottom = std::min(cliparea2.bottom, textPos.y + rowHeight);

			// Draw list item
			DrawText(objectsCount * (i+/*Heading*/1) + def, m_ObjectsDefs[def].m_TextColor, textPos, bz+0.1f, cliparea2);
			xpos += width;
		}
	}
}
