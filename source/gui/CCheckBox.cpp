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
/*	bool				m_Checked;
	CStr				m_Font;
	CGUISpriteInstance	m_Sprite;
	CGUISpriteInstance	m_SpriteDisabled;
	CGUISpriteInstance	m_SpriteOver;
	CGUISpriteInstance	m_SpritePressed;
	CGUISpriteInstance	m_Sprite2;
	CGUISpriteInstance	m_Sprite2Disabled;
	CGUISpriteInstance	m_Sprite2Over;
	CGUISpriteInstance	m_Sprite2Pressed;
	int					m_SquareSide;
	EAlign				m_TextAlign;
	CColor				m_TextColor;
	CColor				m_TextColorDisabled;
	CColor				m_TextColorOver;
	CColor				m_TextColorPressed;
	EVAlign				m_TextValign;
	CStr				m_ToolTip;
	CStr				m_ToolTipStyle;
*/
	AddSetting(GUIST_CGUIString,			"caption");
	AddSetting(GUIST_bool,					"checked");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite-over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite-pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite-disabled");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2-over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2-pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2-disabled");
	AddSetting(GUIST_int,					"square-side");
	AddSetting(GUIST_CStr,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip-style");

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
	// TODO Gee: research if even "default" should be hardcoded.
	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, CStr("default"), m_CachedActualSize.GetWidth()-20, 0);

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
	float middle = (m_CachedActualSize.bottom - m_CachedActualSize.top)/2;
	CRect rect;
	rect.left =		m_CachedActualSize.left + middle - square_side/2;
	rect.right =	rect.left + square_side;
	rect.top =		m_CachedActualSize.top + middle - square_side/2;
	rect.bottom =	rect.top + square_side;

	bool checked;
	GUI<bool>::GetSetting(this, "checked", checked);

	CGUISpriteInstance *sprite, *sprite_over, *sprite_pressed, *sprite_disabled;

	if (checked)
	{
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2", sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2-over", sprite_over);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2-pressed", sprite_pressed);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2-disabled", sprite_disabled);
	}
	else
	{
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite-over", sprite_over);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite-pressed", sprite_pressed);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite-disabled", sprite_disabled);
	}

	DrawButton(rect, 
			   bz,
			   *sprite,
			   *sprite_over,
			   *sprite_pressed,
			   *sprite_disabled,
			   0);

	CColor color = ChooseColor();

//	IGUITextOwner::Draw(0, color, m_TextPos, bz+0.1f);
}
