/*
CCheckBox
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "CCheckBox.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CCheckBox::CCheckBox()
{
/*	bool			m_Checked;
	CStr			m_Font;
	CStr			m_Sprite;
	CStr			m_SpriteDisabled;
	CStr			m_SpriteOver;
	CStr			m_SpritePressed;
	CStr			m_Sprite2;
	CStr			m_Sprite2Disabled;
	CStr			m_Sprite2Over;
	CStr			m_Sprite2Pressed;
	int				m_SquareSide;
	EAlign			m_TextAlign;
	CColor			m_TextColor;
	CColor			m_TextColorDisabled;
	CColor			m_TextColorOver;
	CColor			m_TextColorPressed;
	EValign			m_TextValign;
	CStr			m_ToolTip;
	CStr			m_ToolTipStyle;
*/
	AddSetting(GUIST_CGUIString,		"caption");
	AddSetting(GUIST_bool,				"checked");
	AddSetting(GUIST_CStr,				"sprite");
	AddSetting(GUIST_CStr,				"sprite-over");
	AddSetting(GUIST_CStr,				"sprite-pressed");
	AddSetting(GUIST_CStr,				"sprite-disabled");
	AddSetting(GUIST_CStr,				"sprite2");
	AddSetting(GUIST_CStr,				"sprite2-over");
	AddSetting(GUIST_CStr,				"sprite2-pressed");
	AddSetting(GUIST_CStr,				"sprite2-disabled");
	AddSetting(GUIST_int,				"square-side");

	// Add text
	AddText(new SGUIText());
}

CCheckBox::~CCheckBox()
{
}

void CCheckBox::SetupText()
{
	if (!GetGUI())
		return;

	assert(m_GeneratedTexts.size()>=1);

	CStr font;
	CGUIString caption;
	//int square_side;
	GUI<CGUIString>::GetSetting(this, "caption", caption);
	//GUI<CGUIString>::GetSetting(this, "square-side", square_side);

	// TODO Gee: Establish buffer zones
	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, CStr("verdana12.fnt"), m_CachedActualSize.GetWidth()-20, 0);

	// Set position of text
	// TODO Gee: Big TODO
//	m_TextPos.x = m_CachedActualSize.left + 20;
//	m_TextPos.y = m_CachedActualSize.top;
}

void CCheckBox::HandleMessage(const SGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
	{
		bool checked;

		GUI<bool>::GetSetting(this, "checked", checked);
		checked = !checked;
		GUI<bool>::SetSetting(this, "checked", checked);
		
		//GetGUI()->TEMPmessage = "Check box " + string((const TCHAR*)m_Name) + " was " + (m_Settings.m_Checked?"checked":"unchecked");
	}	break;

	default:
		break;
	}
}

void CCheckBox::Draw() 
{
	////////// Gee: janwas, this is just temp to see it
	glDisable(GL_TEXTURE_2D);
	//////////

	int square_side;
	GUI<int>::GetSetting(this, "square-side", square_side);

	float bz = GetBufferedZ();

	// Get square
	// TODO Gee: edit below when CRect has got "height()"
	int middle = (m_CachedActualSize.bottom - m_CachedActualSize.top)/2;
	CRect rect;
	rect.left =		m_CachedActualSize.left + middle - square_side/2;
	rect.right =	rect.left + square_side;
	rect.top =		m_CachedActualSize.top + middle - square_side/2;
	rect.bottom =	rect.top + square_side;

	bool checked;
	GUI<bool>::GetSetting(this, "checked", checked);

	CStr sprite, sprite_over, sprite_pressed, sprite_disabled;

	if (checked)
	{
		GUI<CStr>::GetSetting(this, "sprite2", sprite);
		GUI<CStr>::GetSetting(this, "sprite2-over", sprite_over);
		GUI<CStr>::GetSetting(this, "sprite2-pressed", sprite_pressed);
		GUI<CStr>::GetSetting(this, "sprite2-disabled", sprite_disabled);
  	}
	else
	{
		GUI<CStr>::GetSetting(this, "sprite", sprite);
		GUI<CStr>::GetSetting(this, "sprite-over", sprite_over);
		GUI<CStr>::GetSetting(this, "sprite-pressed", sprite_pressed);
		GUI<CStr>::GetSetting(this, "sprite-disabled", sprite_disabled);
	}

	DrawButton(	rect, 
				bz,
				sprite,
				sprite_over,
				sprite_pressed,
				sprite_disabled);

	CColor color = ChooseColor();

//	IGUITextOwner::Draw(0, color, m_TextPos, bz+0.1f);
}
