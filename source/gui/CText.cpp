/*
CText
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "CText.h"

#include "ogl.h"

// TODO Gee: new
#include "OverlayText.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CText::CText()
{
	AddSetting(GUIST_CGUIString,	"caption");
	AddSetting(GUIST_bool,			"scrollbar");
	AddSetting(GUIST_CStr,			"scrollbar-style");
	AddSetting(GUIST_CStr,			"sprite");
	AddSetting(GUIST_CColor,		"textcolor");

	//GUI<bool>::SetSetting(this, "ghost", true);
	GUI<bool>::SetSetting(this, "scrollbar", false);

	// Add scroll-bar
	CGUIScrollBarVertical * bar = new CGUIScrollBarVertical();
	bar->SetRightAligned(true);
	bar->SetUseEdgeButtons(true);
	AddScrollBar(bar);

	// Add text
	AddText(new SGUIText());
}

CText::~CText()
{
}

void CText::SetupText()
{
	if (!GetGUI())
		return;

	assert(m_GeneratedTexts.size()>=1);

	CColor color;
	CStr font;
	CGUIString caption;
	bool scrollbar;
	GUI<CColor>::GetSetting(this, "textcolor", color);
	GUI<CGUIString>::GetSetting(this, "caption", caption);
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	int width = m_CachedActualSize.GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, /*color,*/ CStr("palatino12"), width, 4);

	// Setup scrollbar
	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange( m_GeneratedTexts[0]->m_Size.cy );
		GetScrollBar(0).SetScrollSpace( m_CachedActualSize.GetHeight() );
	}
}

void CText::HandleMessage(const SGUIMessage &Message)
{
	// TODO Gee:
	IGUIScrollBarOwner::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		bool scrollbar;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

		// Update scroll-bar
		if (scrollbar && 
		    (Message.value == CStr("size") || Message.value == CStr("z") ||
			 Message.value == CStr("absolute")))
		{
			
			GetScrollBar(0).SetX( m_CachedActualSize.right );
			GetScrollBar(0).SetY( m_CachedActualSize.top );
			GetScrollBar(0).SetZ( GetBufferedZ() );
			GetScrollBar(0).SetLength( m_CachedActualSize.bottom - m_CachedActualSize.top );
		}

		// Update scrollbar
		if (Message.value == CStr("scrollbar-style"))
		{
			CStr scrollbar_style;
			GUI<CStr>::GetSetting(this, Message.value, scrollbar_style);

			GetScrollBar(0).SetScrollBarStyle( scrollbar_style );
		}

		break;

	case GUIM_MOUSE_WHEEL_DOWN:
		GetScrollBar(0).ScrollPlus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		HandleMessage(SGUIMessage(GUIM_MOUSE_MOTION));
		break;

	case GUIM_MOUSE_WHEEL_UP:
		GetScrollBar(0).ScrollMinus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		HandleMessage(SGUIMessage(GUIM_MOUSE_MOTION));
		break;

	default:
		break;
	}
}

void CText::Draw() 
{
	////////// Gee: janwas, this is just temp to see it
	glDisable(GL_TEXTURE_2D);
	//////////

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
		CStr sprite;
		GUI<CStr>::GetSetting(this, "sprite", sprite);

		GetGUI()->DrawSprite(sprite, bz, m_CachedActualSize);

		int scroll=0;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		CColor color;
		GUI<CColor>::GetSetting(this, "textcolor", color);

		// Draw text
		IGUITextOwner::Draw(0, color, m_CachedActualSize.TopLeft() - CPos(0,scroll), bz+0.1f);
	}
}
