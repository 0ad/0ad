/*
CDropDown
*/

#include "precompiled.h"
#include "GUI.h"
#include "CDropDown.h"

#include "lib/ogl.h"
#include "lib/external_libraries/sdl.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CDropDown::CDropDown() : m_Open(false), m_HideScrollBar(false), m_ElementHighlight(-1)
{
	AddSetting(GUIST_float,					"button_width");
	AddSetting(GUIST_float,					"dropdown_size");
	AddSetting(GUIST_float,					"dropdown_buffer");
//	AddSetting(GUIST_CStr,					"font");
//	AddSetting(GUIST_CGUISpriteInstance,	"sprite");				// Background that sits around the size
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_list");			// Background of the drop down list
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2");				// Button that sits to the right
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_disabled");
	AddSetting(GUIST_EVAlign,				"text_valign");
	
	// Add these in CList! And implement TODO
	//AddSetting(GUIST_CColor,				"textcolor_over");
	//AddSetting(GUIST_CColor,				"textcolor_pressed");
	//AddSetting(GUIST_CColor,				"textcolor_disabled");

	// Scrollbar is forced to be true.
	GUI<bool>::SetSetting(this, "scrollbar", true);
}

CDropDown::~CDropDown()
{
}

void CDropDown::SetupText()
{
	CList::SetupText();

	SetupListRect();
}

void CDropDown::HandleMessage(const SGUIMessage &Message)
{
	// Important

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		bool scrollbar;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

		// Update scroll-bar
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
		if (scrollbar && 
		    (Message.value == CStr("dropdown_size")/* || 
			 Message.value == CStr("")*/))
		{
			CRect rect = GetListRect();
			GetScrollBar(0).SetX( rect.right );
			GetScrollBar(0).SetY( rect.top );
			GetScrollBar(0).SetZ( GetBufferedZ() );
			GetScrollBar(0).SetLength( rect.bottom - rect.top );
		}

		// Update cached list rect
		if (Message.value == CStr("size") ||
			Message.value == CStr("absolute") ||
			Message.value == CStr("dropdown_size") ||
			Message.value == CStr("dropdown_buffer") ||
			Message.value == CStr("scrollbar_style") ||
			Message.value == CStr("button_width"))
		{
			SetupListRect();
		}		

		break;

	case GUIM_MOUSE_MOTION:{
		if (m_Open)
		{
			CPos mouse = GetMousePos();

			if (GetListRect().PointInside(mouse))
			{
				bool scrollbar;
				CGUIList *pList;
				GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
				GUI<CGUIList>::GetSettingPointer(this, "list", pList);
				float scroll=0.f;
				if (scrollbar)
				{
					scroll = GetScrollBar(0).GetPos();
				}

				CRect rect = GetListRect();
				mouse.y += scroll;
				int set=-1;
				for (int i=0; i<(int)pList->m_Items.size(); ++i)
				{
					if (mouse.y >= rect.top + m_ItemsYPositions[i] &&
						mouse.y < rect.top + m_ItemsYPositions[i+1] &&
						// mouse is not over scroll-bar
						!(mouse.x >= GetScrollBar(0).GetOuterRect().left &&
						mouse.x <= GetScrollBar(0).GetOuterRect().right))
					{
						set = i;
					}
				}
				
				if (set != -1)
				{
					//GUI<int>::SetSetting(this, "selected", set);
					m_ElementHighlight = set;
					//UpdateAutoScroll();
				}
			}
		}

		}break;

	case GUIM_MOUSE_LEAVE:{
		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

		}break;

		// We can't inherent this routine from CList, because we need to include
		//  a mouse click to open the dropdown, also the coordinates are changed.
	case GUIM_MOUSE_PRESS_LEFT:
	{
		if (!m_Open)
		{
			m_Open = true;
			GetScrollBar(0).SetPos(0.f);
			GetScrollBar(0).SetZ( GetBufferedZ() );
			GUI<int>::GetSetting(this, "selected", m_ElementHighlight);
			return; // overshadow
		}
		else
		{
			CPos mouse = GetMousePos();

			// If the regular area is pressed, then abort, and close.
			if (m_CachedActualSize.PointInside(mouse))
			{
				m_Open = false;
				GetScrollBar(0).SetZ( GetBufferedZ() );
				return; // overshadow
			}

			if (!(mouse.x >= GetScrollBar(0).GetOuterRect().left &&
				mouse.x <= GetScrollBar(0).GetOuterRect().right) &&
				mouse.y >= GetListRect().top)
			{
				m_Open = false;
				GetScrollBar(0).SetZ( GetBufferedZ() );
			}
		}
	}	break;

	case GUIM_LOST_FOCUS:
		m_Open=false;
		break;

	case GUIM_LOAD:
		SetupListRect();
		break;

	default:
		break;
	}

	// Important that this is after, so that overshadowed implementations aren't processed
	CList::HandleMessage(Message);
}

InReaction CDropDown::ManuallyHandleEvent(const SDL_Event_* ev)
{
	int szChar = ev->ev.key.keysym.sym;
	bool update_highlight = false;

	switch (szChar)
	{
	case '\r':
		m_Open=false;
		break;

	case SDLK_HOME:
	case SDLK_END:
	case SDLK_UP:
	case SDLK_DOWN:
	case SDLK_PAGEUP:
	case SDLK_PAGEDOWN:
		if (!m_Open)
			return IN_PASS;
		// Set current selected item to highlighted, before
		//  then really processing these in CList::ManuallyHandleEvent()
		GUI<int>::SetSetting(this, "selected", m_ElementHighlight);
		update_highlight = true;
		break;

	default:
		break;
	}

	CList::ManuallyHandleEvent(ev);

	if (update_highlight)
		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

	return IN_HANDLED;
}

void CDropDown::SetupListRect()
{
	float size, buffer, button_width;
	GUI<float>::GetSetting(this, "dropdown_size", size);
	GUI<float>::GetSetting(this, "dropdown_buffer", buffer);
	GUI<float>::GetSetting(this, "button_width", button_width);

	if (m_ItemsYPositions.empty() || m_ItemsYPositions.back() >= size)
	{
		m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom+buffer,
								m_CachedActualSize.right, m_CachedActualSize.bottom+buffer + size);

		m_HideScrollBar = false;
	}
	else
	{
		m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom+buffer,
								 m_CachedActualSize.right - GetScrollBar(0).GetStyle()->m_Width, m_CachedActualSize.bottom+buffer + m_ItemsYPositions.back());

		// We also need to hide the scrollbar
		m_HideScrollBar = true;
	}
}

CRect CDropDown::GetListRect() const
{
	return m_CachedListRect;
}

bool CDropDown::MouseOver()
{
	if(!GetGUI())
		throw PS_NEEDS_PGUI;

	if (m_Open)
	{
		CRect rect(m_CachedActualSize.left, m_CachedActualSize.top,
				   m_CachedActualSize.right, GetListRect().bottom);


		return rect.PointInside(GetMousePos());
	}
	else
		return m_CachedActualSize.PointInside(GetMousePos());
}

void CDropDown::Draw() 
{
	if (!GetGUI())
		return;

	float bz = GetBufferedZ();

	float dropdown_size, button_width;
	GUI<float>::GetSetting(this, "dropdown_size", dropdown_size);
	GUI<float>::GetSetting(this, "button_width", button_width);

	CGUISpriteInstance *sprite, *sprite2, *sprite2_second;
	int cell_id, selected=0;
	CColor color;

	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2", sprite2);
	GUI<int>::GetSetting(this, "cell_id", cell_id);
	GUI<int>::GetSetting(this, "selected", selected);
	GUI<CColor>::GetSetting(this, "textcolor", color);


	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);

	GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);

	if (button_width > 0.f)
	{
		CRect rect(m_CachedActualSize.right-button_width, m_CachedActualSize.top,
				   m_CachedActualSize.right, m_CachedActualSize.bottom);

		if (!enabled)
		{
			GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_disabled", sprite2_second);
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(*sprite2_second, *sprite2), cell_id, bz+0.05f, rect);
		}
		else
		if (m_Open)
		{
			GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_pressed", sprite2_second);
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(*sprite2_second, *sprite2), cell_id, bz+0.05f, rect);
		}
		else
		if (m_MouseHovering)
		{
			GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_over", sprite2_second);
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(*sprite2_second, *sprite2), cell_id, bz+0.05f, rect);
		}
		else 
			GetGUI()->DrawSprite(*sprite2, cell_id, bz+0.05f, rect);
	}

	if (selected != -1) // TODO: Maybe check validity completely?
	{
		// figure out clipping rectangle
		CRect cliparea(m_CachedActualSize.left, m_CachedActualSize.top,
					   m_CachedActualSize.right-button_width, m_CachedActualSize.bottom);

		CPos pos(m_CachedActualSize.left, m_CachedActualSize.top);
		IGUITextOwner::Draw(selected, color, pos, bz+0.1f, cliparea);
	}

	bool *scrollbar=NULL, old;
	GUI<bool>::GetSettingPointer(this, "scrollbar", scrollbar);

	old = *scrollbar;

	if (m_Open)
	{
		if (m_HideScrollBar)
            *scrollbar = false;

		DrawList(m_ElementHighlight, "sprite_list", "sprite_selectarea", "textcolor");
		
		if (m_HideScrollBar)
			*scrollbar = old;
	}
}

// When a dropdown list is opened, it needs to be visible above all the other
// controls on the page. The only way I can think of to do this is to increase
// its z value when opened, so that it's probably on top.
float CDropDown::GetBufferedZ() const
{
	float bz = CList::GetBufferedZ();
	if (m_Open)
		return std::min(bz + 500.f, 1000.f); // TODO - don't use magic number for max z value
	else
		return bz;
}
