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
	AddSetting(GUIST_float,					"buffer-zone");
	AddSetting(GUIST_CGUIString,			"caption");
	AddSetting(GUIST_CStr,					"font");
	AddSetting(GUIST_bool,					"scrollbar");
	AddSetting(GUIST_CStr,					"scrollbar-style");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_int,					"cell-id");
	AddSetting(GUIST_CColor,				"textcolor");
	// TODO Gee: (2004-08-14)
	//  Add a setting for buffer zone
	//AddSetting(GUIST_int,			"

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
	if (GUI<CStr>::GetSetting(this, "font", font) != PS_OK || font.Length()==0)
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Don't define standard like this. Do it with the default style.
		font = "default";

	CGUIString caption;
	bool scrollbar;
	GUI<CColor>::GetSetting(this, "textcolor", color);
	GUI<CGUIString>::GetSetting(this, "caption", caption);
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	float width = m_CachedActualSize.GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;


    float buffer_zone=0.f;
	GUI<float>::GetSetting(this, "buffer-zone", buffer_zone);
	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, font, width, buffer_zone, this);

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
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
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

			// Update text
			SetupText();
		}

		break;

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

	default:
		break;
	}
}

void CText::Draw() 
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
		CGUISpriteInstance *sprite;
		int cell_id;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
		GUI<int>::GetSetting(this, "cell-id", cell_id);

		GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);

		float scroll=0.f;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		CColor color;
		GUI<CColor>::GetSetting(this, "textcolor", color);

		// Draw text
		IGUITextOwner::Draw(0, color, m_CachedActualSize.TopLeft() - CPos(0.f, scroll), bz+0.1f);
	}
}
