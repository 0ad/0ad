/*
CButton
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "CButton.h"

#include "ogl.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CButton::CButton()
{
	AddSetting(GUIST_float,					"buffer-zone");
	AddSetting(GUIST_CGUIString,			"caption");
	AddSetting(GUIST_CStr,					"font");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite-over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite-pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite-disabled");
	AddSetting(GUIST_int,					"cell-id");
	AddSetting(GUIST_EAlign,				"text-align");
	AddSetting(GUIST_EVAlign,				"text-valign");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_CColor,				"textcolor-over");
	AddSetting(GUIST_CColor,				"textcolor-pressed");
	AddSetting(GUIST_CColor,				"textcolor-disabled");
	AddSetting(GUIST_CStr,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip-style");

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
	if (GUI<CStr>::GetSetting(this, "font", font) != PS_OK || font.Length()==0)
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Default should not be hard-coded, but be in styles!
		font = "default";

	CGUIString caption;
	GUI<CGUIString>::GetSetting(this, "caption", caption);

	float buffer_zone=0.f;
	GUI<float>::GetSetting(this, "buffer-zone", buffer_zone);
	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, font, m_CachedActualSize.GetWidth(), buffer_zone, this);

	CalculateTextPosition(m_CachedActualSize, m_TextPos, *m_GeneratedTexts[0]);
}

void CButton::HandleMessage(const SGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
//		GetGUI()->TEMPmessage = "Button " + string((const TCHAR*)m_Name) + " was pressed!";
		break;

	default:
		break;
	}
}

void CButton::Draw() 
{
	float bz = GetBufferedZ();

	CGUISpriteInstance *sprite, *sprite_over, *sprite_pressed, *sprite_disabled;
	int cell_id;

	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite-over", sprite_over);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite-pressed", sprite_pressed);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite-disabled", sprite_disabled);
	GUI<int>::GetSetting(this, "cell-id", cell_id);

	DrawButton(m_CachedActualSize,
			   bz,
			   *sprite,
			   *sprite_over,
			   *sprite_pressed,
			   *sprite_disabled,
			   cell_id);

	CColor color = ChooseColor();
	IGUITextOwner::Draw(0, color, m_TextPos, bz+0.1f);
}
