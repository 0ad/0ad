/*
CText
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx."
#include "GUI.h"

// TODO Gee: font.h is temporary.
#include "font.h"
#include "ogl.h"

using namespace std;

// Offsets
DECLARE_SETTINGS_INFO(STextSettings)

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CText::CText()
{
	// Static! Only done once
	if (m_SettingsInfo.empty())
	{
		// Setup the base ones too
		SetupBaseSettingsInfo(m_SettingsInfo);

		GUI_ADD_OFFSET_EXT(STextSettings, m_Sprite,			"string",		"sprite")
		GUI_ADD_OFFSET_EXT(STextSettings, m_ScrollBar,		"bool",			"scrollbar")
		GUI_ADD_OFFSET_EXT(STextSettings, m_ScrollBarStyle,	"string",		"scrollbar-style")
	}

	// Add scroll-bar
	CGUIScrollBarVertical * bar = new CGUIScrollBarVertical();
	bar->SetRightAligned(true);
	AddScrollBar(bar);
}

CText::~CText()
{
}

void CText::HandleMessage(const SGUIMessage &Message)
{
	// TODO Gee:
	IGUIScrollBarOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		if (Message.value == CStr("size") || Message.value == CStr("z") ||
			Message.value == CStr("absolute"))
		{
			GetScrollBar(0).SetX( m_CachedActualSize.right );
			GetScrollBar(0).SetY( m_CachedActualSize.top );
			GetScrollBar(0).SetZ( GetBufferedZ() );
			GetScrollBar(0).SetLength( m_CachedActualSize.bottom - m_CachedActualSize.top );
		}
		
		if (Message.value == CStr("scrollbar-style"))
		{
			GetScrollBar(0).SetScrollBarStyle( GetSettings().m_ScrollBarStyle );
		}
		break;

	case GUIM_MOUSE_WHEEL_DOWN:
		GetScrollBar(0).ScrollPlus();
		break;

	case GUIM_MOUSE_WHEEL_UP:
		GetScrollBar(0).ScrollMinus();
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

	// First call draw on ScrollBarOwner
	IGUIScrollBarOwner::Draw();

	if (GetGUI())
	{
		GetGUI()->DrawSprite(m_Settings.m_Sprite, GetBufferedZ(), m_CachedActualSize);
	}

}
