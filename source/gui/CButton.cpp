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
	AddSetting(GUIST_float,			"buffer-zone");
	AddSetting(GUIST_CGUIString,	"caption");
	AddSetting(GUIST_CStr,			"font");
	AddSetting(GUIST_CStr,			"sprite");
	AddSetting(GUIST_CStr,			"sprite-over");
	AddSetting(GUIST_CStr,			"sprite-pressed");
	AddSetting(GUIST_CStr,			"sprite-disabled");
	AddSetting(GUIST_EAlign,		"text-align");
	AddSetting(GUIST_EVAlign,		"text-valign");
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
	if (GUI<CStr>::GetSetting(this, "font", font) != PS_OK || font.Length()==0)
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Default should not be hard-coded, but be in styles!
		font = "default";

	CGUIString caption;
	GUI<CGUIString>::GetSetting(this, "caption", caption);

	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, font, m_CachedActualSize.GetWidth(), 0, this);

	// Set position of text

	// Check which alignment to use!
	EAlign align;
	EVAlign valign;
	float bz;
	GUI<EAlign>::GetSetting(this, "text-align", align);
	GUI<EVAlign>::GetSetting(this, "text-valign", valign);
	GUI<float>::GetSetting(this, "buffer-zone", bz);

	switch (align)
	{
	case EAlign_Left:
		m_TextPos.x = m_CachedActualSize.left + bz;
		break;
	case EAlign_Center:
		// Round to integer pixel values, else the fonts look awful
		m_TextPos.x = floorf(m_CachedActualSize.CenterPoint().x - m_GeneratedTexts[0]->m_Size.cx/2.f);
		break;
	case EAlign_Right:
		m_TextPos.x = m_CachedActualSize.right - m_GeneratedTexts[0]->m_Size.cx - bz;
		break;
	default:
		debug_warn("Broken EAlign in CButton::SetupText()");
		break;
	}

	switch (valign)
	{
	case EVAlign_Top:
		m_TextPos.y = m_CachedActualSize.top + bz;
		break;
	case EVAlign_Center:
		// Round to integer pixel values, else the fonts look awful
		m_TextPos.y = floorf(m_CachedActualSize.CenterPoint().y - m_GeneratedTexts[0]->m_Size.cy/2.f);
		break;
	case EVAlign_Bottom:
		m_TextPos.y = m_CachedActualSize.bottom - m_GeneratedTexts[0]->m_Size.cy - bz;
		break;
	default:
		debug_warn("Broken EVAlign in CButton::SetupText()");
		break;
	}

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
