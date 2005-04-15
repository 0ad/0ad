/*
CList
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "CList.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CList::CList()
{
	AddSetting(GUIST_float,					"buffer_zone");
	//AddSetting(GUIST_CGUIString,			"caption"); will it break removing this? If I know my system, then no, but test just in case TODO (Gee).
	AddSetting(GUIST_CStr,					"font");
	AddSetting(GUIST_bool,					"scrollbar");
	AddSetting(GUIST_CStr,					"scrollbar_style");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_selectarea");
	AddSetting(GUIST_int,					"cell_id");
	AddSetting(GUIST_EAlign,				"text_align");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_int,					"selected");	// Index selected. -1 is none.
	//AddSetting(GUIST_CStr,					"tooltip");
	//AddSetting(GUIST_CStr,					"tooltip_style");

	GUI<bool>::SetSetting(this, "scrollbar", false);

	GUI<int>::SetSetting(this, "selected", 1);

	// Add scroll-bar
	CGUIScrollBarVertical * bar = new CGUIScrollBarVertical();
	bar->SetRightAligned(true);
	bar->SetUseEdgeButtons(true);
	AddScrollBar(bar);
}

CList::~CList()
{
}

void CList::SetupText()
{
	if (!GetGUI())
		return;

	//assert(m_GeneratedTexts.size()>=1);

	m_ItemsYPositions.resize( m_Items.size()+1 );

	// Delete all generated texts. Some could probably be saved,
	//  but this is easier, and this function will never be called
	//  continuously, or even often, so it'll probably be okay.
	vector<SGUIText*>::iterator it;
	for (it=m_GeneratedTexts.begin(); it!=m_GeneratedTexts.end(); ++it)
	{
		if (*it)
			delete *it;
	}
	m_GeneratedTexts.clear();

	CStr font;
	if (GUI<CStr>::GetSetting(this, "font", font) != PS_OK || font.Length()==0)
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Don't define standard like this. Do it with the default style.
		font = "default";

	//CGUIString caption;
	bool scrollbar;
	//GUI<CGUIString>::GetSetting(this, "caption", caption);
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	float width = m_CachedActualSize.GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	float buffer_zone=0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	// Generate texts
	float buffered_y = 0.f;

	for (size_t i=0; i<m_Items.size(); ++i)
	{
		// Create a new SGUIText. Later on, input it using AddText()
		SGUIText *text = new SGUIText();

		*text = GetGUI()->GenerateText(m_Items[i], font, width, buffer_zone, this);

		m_ItemsYPositions[i] = buffered_y;
		buffered_y += text->m_Size.cy;

		AddText(text);
	}

	m_ItemsYPositions[i] = buffered_y;
	
	//if (! scrollbar)
	//	CalculateTextPosition(m_CachedActualSize, m_TextPos, *m_GeneratedTexts[0]);

	// Setup scrollbar
	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange( m_ItemsYPositions.back() );
		GetScrollBar(0).SetScrollSpace( m_CachedActualSize.GetHeight() );
	}
}

void CList::HandleMessage(const SGUIMessage &Message)
{
	IGUIScrollBarOwner::HandleMessage(Message);
	//IGUITextOwner::HandleMessage(Message); <== placed it after the switch instead!

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		bool scrollbar;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

		// Update scroll-bar
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
		if (scrollbar && 
		    (Message.value == CStr("size") || 
			 Message.value == CStr("z") ||
			 Message.value == CStr("absolute")))
		{
			GetScrollBar(0).SetX( m_CachedActualSize.right );
			GetScrollBar(0).SetY( m_CachedActualSize.top );
			GetScrollBar(0).SetZ( GetBufferedZ() );
			GetScrollBar(0).SetLength( m_CachedActualSize.bottom - m_CachedActualSize.top );
		}

		if (Message.value == CStr("scrollbar"))
		{
			SetupText();
		}

		// Update scrollbar
		if (Message.value == CStr("scrollbar_style"))
		{
			CStr scrollbar_style;
			GUI<CStr>::GetSetting(this, Message.value, scrollbar_style);

			GetScrollBar(0).SetScrollBarStyle( scrollbar_style );

			SetupText();
		}

		break;

	case GUIM_MOUSE_PRESS_LEFT:
	{
		bool scrollbar;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		float scroll=0.f;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		CPos mouse = GetMousePos();
		mouse.y += scroll;
		int set=-1;
		for (int i=0; i<(int)m_Items.size(); ++i)
		{
			if (mouse.y >= m_CachedActualSize.top + m_ItemsYPositions[i] &&
				mouse.y < m_CachedActualSize.top + m_ItemsYPositions[i+1] &&
				// mouse is not over scroll-bar
				!(mouse.x >= GetScrollBar(0).GetOuterRect().left &&
				mouse.x <= GetScrollBar(0).GetOuterRect().right))
			{
				set = i;
			}
		}
		
		if (set != -1)
		{
			GUI<int>::SetSetting(this, "selected", set);
			UpdateAutoScroll();
		}
	}	break;

	case GUIM_MOUSE_WHEEL_DOWN:
		GetScrollBar(0).ScrollMinus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		HandleMessage(SGUIMessage(GUIM_MOUSE_MOTION));
		break;

	case GUIM_MOUSE_WHEEL_UP:
		GetScrollBar(0).ScrollPlus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		HandleMessage(SGUIMessage(GUIM_MOUSE_MOTION));
		break;

	case GUIM_LOAD:
		{
		GetScrollBar(0).SetX( m_CachedActualSize.right );
		GetScrollBar(0).SetY( m_CachedActualSize.top );
		GetScrollBar(0).SetZ( GetBufferedZ() );
		GetScrollBar(0).SetLength( m_CachedActualSize.bottom - m_CachedActualSize.top );

		CStr scrollbar_style;
		GUI<CStr>::GetSetting(this, "scrollbar_style", scrollbar_style);
		GetScrollBar(0).SetScrollBarStyle( scrollbar_style );
		}
		break;

	default:
		break;
	}

	IGUITextOwner::HandleMessage(Message);
}

int CList::ManuallyHandleEvent(const SDL_Event* ev)
{
	int szChar = ev->key.keysym.sym;

	switch (szChar)
	{
		case SDLK_HOME:
			SelectFirstElement();
			UpdateAutoScroll();
			break;

		case SDLK_END:
			SelectLastElement();
			UpdateAutoScroll();
			break;

		case SDLK_UP:
			SelectPrevElement();
			UpdateAutoScroll();
			break;

		case SDLK_DOWN:
			SelectNextElement();
			UpdateAutoScroll();
			break;

		case SDLK_PAGEUP:
			GetScrollBar(0).ScrollMinusPlenty();
			break;

		case SDLK_PAGEDOWN:
			GetScrollBar(0).ScrollPlusPlenty();
			break;

		default: // Do nothing
			break;
	}

	return EV_HANDLED;
}

void CList::Draw() 
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
		CGUISpriteInstance *sprite=NULL, *sprite_selectarea=NULL;
		int cell_id, selected;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_selectarea", sprite_selectarea);
		GUI<int>::GetSetting(this, "cell_id", cell_id);
		GUI<int>::GetSetting(this, "selected", selected);

		GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);

		float scroll=0.f;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		if (selected != -1)
		{
			assert(selected >= 0 && selected+1 < m_ItemsYPositions.size());

			// Get rectangle of selection:
			CRect rect(m_CachedActualSize.left, m_CachedActualSize.top + m_ItemsYPositions[selected] - scroll,
					   m_CachedActualSize.right, m_CachedActualSize.top + m_ItemsYPositions[selected+1] - scroll);

			if (rect.top <= m_CachedActualSize.bottom &&
				rect.bottom >= m_CachedActualSize.top)
			{
				if (rect.bottom > m_CachedActualSize.bottom)
					rect.bottom = m_CachedActualSize.bottom;
				if (rect.top < m_CachedActualSize.top)
					rect.top = m_CachedActualSize.top;

				if (scrollbar)
				{
					// Remove any overlapping area of the scrollbar.

					if (rect.right > GetScrollBar(0).GetOuterRect().left &&
						rect.right <= GetScrollBar(0).GetOuterRect().right)
						rect.right = GetScrollBar(0).GetOuterRect().left;

					if (rect.left >= GetScrollBar(0).GetOuterRect().left &&
						rect.left < GetScrollBar(0).GetOuterRect().right)
						rect.left = GetScrollBar(0).GetOuterRect().right;
				}

				GetGUI()->DrawSprite(*sprite_selectarea, cell_id, bz+0.05f, rect);
			}
		}

		CColor color;
		GUI<CColor>::GetSetting(this, "textcolor", color);

		for (int i=0; i<(int)m_Items.size(); ++i)
		{
			if (m_ItemsYPositions[i+1] - scroll < 0 ||
				m_ItemsYPositions[i] - scroll > m_CachedActualSize.GetHeight())
				continue;

			IGUITextOwner::Draw(i, color, m_CachedActualSize.TopLeft() - CPos(0.f, scroll - m_ItemsYPositions[i]), bz+0.1f);
		}
	}
}

void CList::AddItem(const CStr& str) 
{
	CGUIString gui_string; 
	gui_string.SetValue(str); 
	m_Items.push_back( gui_string ); 
}

bool CList::HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile)
{
	int elmt_item = pFile->getElementID("item");

	if (child.getNodeName() == elmt_item)
	{
		AddItem((CStr)child.getText());
		
		return true;
	}
	else
	{
		return false;
	}
}

void CList::SelectNextElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	if (selected != m_Items.size()-1)
	{
		++selected;
		GUI<int>::SetSetting(this, "selected", selected);
	}
}
	
void CList::SelectPrevElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	if (selected != 0)
	{
		--selected;
		GUI<int>::SetSetting(this, "selected", selected);
	}
}

void CList::SelectFirstElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	if (selected != 0)
	{
		GUI<int>::SetSetting(this, "selected", 0);
	}
}
	
void CList::SelectLastElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	if (selected != m_Items.size()-1)
	{
		GUI<int>::SetSetting(this, "selected", (int)m_Items.size()-1);
	}
}

void CList::UpdateAutoScroll()
{
	int selected;
	bool scrollbar;
	float scroll;
	GUI<int>::GetSetting(this, "selected", selected);
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	// No scrollbar, no scrolling (at least it's not made to work properly).
	if (!scrollbar)
		return;

	scroll = GetScrollBar(0).GetPos();

	// Check upper boundary
	if (m_ItemsYPositions[selected] < scroll)
	{
		GetScrollBar(0).SetPos(m_ItemsYPositions[selected]);
		return; // this means, if it wants to align both up and down at the same time
				//  this will have precedence.
	}

	// Check lower boundary
	if (m_ItemsYPositions[selected+1]-m_CachedActualSize.GetHeight() > scroll)
	{
		GetScrollBar(0).SetPos(m_ItemsYPositions[selected+1]-m_CachedActualSize.GetHeight());
	}

/*	float buffer_zone;
	bool multiline;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);

	// Autoscrolling up and down
	if (multiline)
	{
		CStr font_name;
		bool scrollbar;
		GUI<CStr>::GetSetting(this, "font", font_name);
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		
		float scroll=0.f;
		if (!scrollbar)
			return;

		scroll = GetScrollBar(0).GetPos();
		
		// Now get the height of the font.
						// TODO: Get the real font
		CFont font(font_name);
		float spacing = (float)font.GetLineSpacing();
		//float height = font.GetHeight();

		// TODO Gee (2004-11-21): Okay, I need a 'list' for some reasons, but I would really like to
		//  be able to get the specific element here. This is hopefully a temporary hack.

		list<SRow>::iterator current = m_CharacterPositions.begin();
		int row=0;
		while (current != m_CharacterPositions.end())
		{
			if (m_iBufferPos >= current->m_ListStart &&
				m_iBufferPos <= current->m_ListStart+(int)current->m_ListOfX.size())
				break;
			
			++current;
			++row;
		}

		// If scrolling down
		if (-scroll + (float)(row+1) * spacing + buffer_zone*2.f > m_CachedActualSize.GetHeight())
		{
			// Scroll so the selected row is shown completely, also with buffer_zone length to the edge.
			GetScrollBar(0).SetPos((float)(row+1) * spacing - m_CachedActualSize.GetHeight() + buffer_zone*2.f);
		}
		else
		// If scrolling up
		if (-scroll + (float)row * spacing < 0.f)
		{
			// Scroll so the selected row is shown completely, also with buffer_zone length to the edge.
			GetScrollBar(0).SetPos((float)row * spacing);
		}
	}
	else // autoscrolling left and right
	{
		// Get X position of position:
		if (m_CharacterPositions.empty())
			return;

		float x_position = 0.f;
		float x_total = 0.f;
		if (!m_CharacterPositions.begin()->m_ListOfX.empty())
		{

			// Get position of m_iBufferPos
			if ((int)m_CharacterPositions.begin()->m_ListOfX.size() >= m_iBufferPos &&
				m_iBufferPos != 0)
				x_position = m_CharacterPositions.begin()->m_ListOfX[m_iBufferPos-1];

			// Get complete length:
			x_total = m_CharacterPositions.begin()->m_ListOfX[ m_CharacterPositions.begin()->m_ListOfX.size()-1 ];
		}

		// Check if outside to the right
		if (x_position - m_HorizontalScroll + buffer_zone*2.f > m_CachedActualSize.GetWidth())
			m_HorizontalScroll = x_position - m_CachedActualSize.GetWidth() + buffer_zone*2.f;

		// Check if outside to the left
		if (x_position - m_HorizontalScroll < 0.f)
			m_HorizontalScroll = x_position;

		// Check if the text doesn't even fill up to the right edge even though scrolling is done.
		if (m_HorizontalScroll != 0.f &&
			x_total - m_HorizontalScroll + buffer_zone*2.f < m_CachedActualSize.GetWidth())
			m_HorizontalScroll = x_total - m_CachedActualSize.GetWidth() + buffer_zone*2.f;

		// Now this is the fail-safe, if x_total isn't even the length of the control,
		//  remove all scrolling
		if (x_total + buffer_zone*2.f < m_CachedActualSize.GetWidth())
			m_HorizontalScroll = 0.f;
	}*/
}
