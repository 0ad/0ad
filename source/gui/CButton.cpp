/*
CButton
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx."
#include "GUI.h"

// temp GeeTODO
#include "font.h"
#include "ogl.h"

using namespace std;

// Offsets
DECLARE_SETTINGS_INFO(SButtonSettings)

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CButton::CButton()
{
	// Settings defaults !
	m_Settings.m_Disabled =				false;
	m_Settings.m_Font =					"null";
	m_Settings.m_Sprite =				"null";
	m_Settings.m_SpriteDisabled =		"null";
	m_Settings.m_SpriteOver =			"null";
	m_Settings.m_SpritePressed =		"null";
	m_Settings.m_TextAlign =			EAlign_Center;
//	m_Settings.m_TextColor =			CColor();
//	m_Settings.m_TextColorDisabled;
//	m_Settings.m_TextColorOver;
//	m_Settings.m_TextColorPressed;
	m_Settings.m_TextValign =			EValign_Center;
	m_Settings.m_ToolTip =				"null";
	m_Settings.m_ToolTipStyle =			"null";


	// Static! Only done once
	if (m_SettingsInfo.empty())
	{
		// Setup the base ones too
		SetupBaseSettingsInfo(m_SettingsInfo);

		GUI_ADD_OFFSET_EXT(SButtonSettings, m_Sprite,		"string",		"sprite")
		GUI_ADD_OFFSET_EXT(SButtonSettings, m_SpriteOver,	"string",		"sprite-over")
		GUI_ADD_OFFSET_EXT(SButtonSettings, m_SpritePressed,"string",		"sprite-pressed")
	}
}

CButton::~CButton()
{
}

void CButton::HandleMessage(const EGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message)
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

	if (GetGUI())
	{
		if (m_Pressed && m_Settings.m_SpritePressed != CStr("null"))
			GetGUI()->DrawSprite(m_Settings.m_SpritePressed, GetBaseSettings().m_Z, m_CachedActualSize);
		else
		if (m_MouseHovering && !m_Pressed && m_Settings.m_SpriteOver != CStr("null"))
			GetGUI()->DrawSprite(m_Settings.m_SpriteOver, GetBaseSettings().m_Z, m_CachedActualSize);
		else
			GetGUI()->DrawSprite(m_Settings.m_Sprite, GetBaseSettings().m_Z, m_CachedActualSize);
	}
}
