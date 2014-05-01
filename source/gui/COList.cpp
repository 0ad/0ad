#include "precompiled.h"
#include "COList.h"
#include "i18n/L10n.h"

#include "ps/CLogger.h"

COList::COList() : CList(),m_HeadingHeight(30.f)
{
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_heading");
}

void COList::SetupText()
{
	if (!GetGUI())
		return;

	CGUIList *pList;
	GUI<CGUIList>::GetSettingPointer(this, "list_name", pList);

	//ENSURE(m_GeneratedTexts.size()>=1);

	m_ItemsYPositions.resize( pList->m_Items.size()+1 );

	// Delete all generated texts. Some could probably be saved,
	//  but this is easier, and this function will never be called
	//  continuously, or even often, so it'll probably be okay.
	std::vector<SGUIText*>::iterator it;
	for (it=m_GeneratedTexts.begin(); it!=m_GeneratedTexts.end(); ++it)
	{
		if (*it)
			delete *it;
	}
	m_GeneratedTexts.clear();

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Don't define standard like this. Do it with the default style.
		font = L"default";

	//CGUIString caption;
	bool scrollbar;
	//GUI<CGUIString>::GetSetting(this, "caption", caption);
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	float width = GetListRect().GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	// Cache width for other use
	m_TotalAvalibleColumnWidth = width;

	float buffer_zone=0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);


	for (unsigned int c=0; c<m_ObjectsDefs.size(); ++c)
	{
		SGUIText *text = new SGUIText();
		CGUIString gui_string;
		gui_string.SetValue(m_ObjectsDefs[c].m_Heading);
		*text = GetGUI()->GenerateText(gui_string, font, width, buffer_zone, this);
		AddText(text);
	}


	// Generate texts
	float buffered_y = 0.f;

	for (int i=0; i<(int)pList->m_Items.size(); ++i)
	{
		m_ItemsYPositions[i] = buffered_y;
		for (unsigned int c=0; c<m_ObjectsDefs.size(); ++c)
		{
			CGUIList * pList_c;
			GUI<CGUIList>::GetSettingPointer(this, m_ObjectsDefs[c].m_Id, pList_c);
			SGUIText *text = new SGUIText();
			*text = GetGUI()->GenerateText(pList_c->m_Items[i], font, width, buffer_zone, this);
			if (c==0)
				buffered_y += text->m_Size.cy;
			AddText(text);
		}
	}

	m_ItemsYPositions[pList->m_Items.size()] = buffered_y;

	//if (! scrollbar)
	//	CalculateTextPosition(m_CachedActualSize, m_TextPos, *m_GeneratedTexts[0]);

	// Setup scrollbar
	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange( m_ItemsYPositions.back() );
		GetScrollBar(0).SetScrollSpace( GetListRect().GetHeight() );

		CRect rect = GetListRect();
		GetScrollBar(0).SetX( rect.right );
		GetScrollBar(0).SetY( rect.top );
		GetScrollBar(0).SetZ( GetBufferedZ() );
		GetScrollBar(0).SetLength( rect.bottom - rect.top );
	}
}

CRect COList::GetListRect() const
{
	return m_CachedActualSize + CRect(0, m_HeadingHeight, 0, 0);
}

void COList::HandleMessage(SGUIMessage &Message)
{
	CList::HandleMessage(Message);
//	switch (Message.type)
//	{
//	case GUIM_SETTINGS_UPDATED:
//		if (Message.value.Find("list_") != -1)
//		{
//			SetupText();
//		}
//	}
}

bool COList::HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile)
{
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(item);
	ELMT(heading);
	ELMT(def);
	ELMT(translatableAttribute);
	ATTR(id);
	ATTR(context);

	if (child.GetNodeName() == elmt_item)
	{
		AddItem(child.GetText().FromUTF8(), child.GetText().FromUTF8());
		return true;
	}
	else if (child.GetNodeName() == elmt_heading)
	{
		CStrW text (child.GetText().FromUTF8());

		return true;
	}
	else if (child.GetNodeName() == elmt_def)
	{
		ObjectDef oDef;

		XMBAttributeList attributes = child.GetAttributes();
		for (int i=0; i<attributes.Count; ++i)
		{
			XMBAttribute attr = attributes.Item(i);
			CStr attr_name (pFile->GetAttributeString(attr.Name));
			CStr attr_value (attr.Value);

			if (attr_name == "color")
			{
				CColor color;
				if (!GUI<CColor>::ParseString(attr_value.FromUTF8(), color))
					LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
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
					LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
				else
				{
					// Check if it's a relative value, and save as decimal if so.
					if (attr_value.find("%") != std::string::npos)
					{
						width = width / 100.f;
					}
					oDef.m_Width = width;
				}
			}
			else if (attr_name == "heading")
			{
				oDef.m_Heading = attr_value.FromUTF8();
			}

		}

		XMBElementList grandchildren = child.GetChildNodes();
		for (int i = 0; i < grandchildren.Count; ++i)
		{
			XMBElement grandchild = grandchildren.Item(i);
			if (grandchild.GetNodeName() == elmt_translatableAttribute)
			{
				CStr attributeName(grandchild.GetAttributes().GetNamedItem(attr_id));
				// only the heading is translatable for list defs
				if (!attributeName.empty() && attributeName == "heading")
				{
					CStr value(grandchild.GetText());
					if (!value.empty())
					{
						CStr context(grandchild.GetAttributes().GetNamedItem(attr_context)); // Read the context if any.
						if (!context.empty())
						{
							CStr translatedValue(L10n::Instance().TranslateWithContext(context, value));
							oDef.m_Heading = translatedValue.FromUTF8();
						}
						else
						{
							CStr translatedValue(L10n::Instance().Translate(value));
							oDef.m_Heading = translatedValue.FromUTF8();
						}
					}
				}
				else // Ignore.
				{
					LOGERROR(L"GUI: translatable attribute in olist def that isn't a heading. (object: %hs)", this->GetPresentableName().c_str());
				}
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

void COList::DrawList(const int &selected,
					 const CStr& _sprite,
					 const CStr& _sprite_selected,
					 const CStr& _textcolor)
{
	float bz = GetBufferedZ();

	// First call draw on ScrollBarOwner
	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	if (scrollbar)
	{
		// Draw scrollbar
		IGUIScrollBarOwner::Draw();
	}

	if (GetGUI())
	{
		CRect rect = GetListRect();

		CGUISpriteInstance *sprite=NULL, *sprite_selectarea=NULL;
		int cell_id;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, _sprite, sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, _sprite_selected, sprite_selectarea);
		GUI<int>::GetSetting(this, "cell_id", cell_id);

		CGUIList *pList;
		GUI<CGUIList>::GetSettingPointer(this, "list_name", pList);

		GetGUI()->DrawSprite(*sprite, cell_id, bz, rect);

		float scroll=0.f;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

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

				GetGUI()->DrawSprite(*sprite_selectarea, cell_id, bz+0.05f, rect_sel);
			}
		}

		CColor color;
		GUI<CColor>::GetSetting(this, _textcolor, color);

		CGUISpriteInstance *sprite_heading=NULL;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_heading", sprite_heading);
		CRect rect_head(m_CachedActualSize.left, m_CachedActualSize.top, m_CachedActualSize.right,
										m_CachedActualSize.top + m_HeadingHeight);
		GetGUI()->DrawSprite(*sprite_heading, cell_id, bz, rect_head);

		float xpos = 0;
		for (unsigned int def=0; def< m_ObjectsDefs.size(); ++def)
		{
			DrawText(def, color, m_CachedActualSize.TopLeft() + CPos(xpos, 4), bz+0.1f, rect_head);
			// Check if it's a decimal value, and if so, assume relative positioning.
			if (m_ObjectsDefs[def].m_Width < 1 && m_ObjectsDefs[def].m_Width > 0)
				xpos += m_ObjectsDefs[def].m_Width * m_TotalAvalibleColumnWidth;
			else
				xpos += m_ObjectsDefs[def].m_Width;
		}

		for (int i=0; i<(int)pList->m_Items.size(); ++i)
		{
			if (m_ItemsYPositions[i+1] - scroll < 0 ||
				m_ItemsYPositions[i] - scroll > rect.GetHeight())
				continue;

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

			xpos = 0;
			for (unsigned int def=0; def< m_ObjectsDefs.size(); ++def)
			{
				DrawText(m_ObjectsDefs.size() * (i+/*Heading*/1) + def, m_ObjectsDefs[def].m_TextColor, rect.TopLeft() + CPos(xpos, -scroll + m_ItemsYPositions[i]), bz+0.1f, cliparea);
				// Check if it's a decimal value, and if so, assume relative positioning.
				if (m_ObjectsDefs[def].m_Width < 1 && m_ObjectsDefs[def].m_Width > 0)
					xpos += m_ObjectsDefs[def].m_Width * m_TotalAvalibleColumnWidth;
				else
					xpos += m_ObjectsDefs[def].m_Width;
			}
		}
	}
}
