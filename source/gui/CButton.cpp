/*
CButton
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx."
#include "GUI.h"
#include "CButton.h"

// TODO Gee: font.h is temporary.
#include "res/font.h"
#include "res/res.h"
#include "ogl.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CButton::CButton()
{
	AddSetting(GUIST_CGUIString,	"caption");
	AddSetting(GUIST_CStr,			"font");
	AddSetting(GUIST_CStr,			"sprite");
	AddSetting(GUIST_CStr,			"sprite-over");
	AddSetting(GUIST_CStr,			"sprite-pressed");
	AddSetting(GUIST_CStr,			"sprite-disabled");
	AddSetting(GUIST_CColor,		"textcolor");
	AddSetting(GUIST_CColor,		"textcolor-over");
	AddSetting(GUIST_CColor,		"textcolor-pressed");
	AddSetting(GUIST_CColor,		"textcolor-disabled");

	// Add text
	AddText(new SGUIText());
}

CButton::~CButton()
{
}

void CButton::SetupText()
{
	if (!GetGUI())
		return;

	assert(m_GeneratedTexts.size()>=1);

	CStr font;
	CGUIString caption;
	GUI<CGUIString>::GetSetting(this, "caption", caption);

	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, CStr("verdana12.fnt"), 0, 0);

	// Set position of text
	m_TextPos = m_CachedActualSize.CenterPoint() - m_GeneratedTexts[0]->m_Size/2;
}

void CButton::HandleMessage(const SGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PREPROCESS:
		break;

	case GUIM_POSTPROCESS:
		break;

	case GUIM_MOUSE_OVER:
		break;

	case GUIM_MOUSE_ENTER:
		break;

	case GUIM_MOUSE_LEAVE:
		break;

	case GUIM_MOUSE_PRESS_LEFT:
		break;

	case GUIM_MOUSE_RELEASE_LEFT:
		break;

	case GUIM_PRESSED:
		GetGUI()->TEMPmessage = "Button " + string((const TCHAR*)m_Name) + " was pressed!";
		break;

	default:
		break;
	}
}

void CButton::Draw() 
{
	////////// Gee: janwas, this is just temp to see it
	glDisable(GL_TEXTURE_2D);
	//////////

	float bz = GetBufferedZ();

	CStr sprite, sprite_over, sprite_pressed, sprite_disabled;

	GUI<CStr>::GetSetting(this, "sprite", sprite);
	GUI<CStr>::GetSetting(this, "sprite-over", sprite_over);
	GUI<CStr>::GetSetting(this, "sprite-pressed", sprite_pressed);
	GUI<CStr>::GetSetting(this, "sprite-disabled", sprite_disabled);
  
	DrawButton(m_CachedActualSize, 
			   bz, 
			   sprite,
			   sprite_over, 
			   sprite_pressed, 
			   sprite_disabled);


	CColor color = ChooseColor();

	IGUITextOwner::Draw(0, color, m_TextPos, bz+0.1f);
}
