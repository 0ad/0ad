/*
GUI Object - Button
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object representing a simple button

--More info--

	Check GUI.h

*/

#ifndef CButton_H
#define CButton_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

// Settings
struct SButtonSettings
{
	bool			m_Disabled;
	CStr			m_Font;
	CStr			m_Sprite;
	CStr			m_SpriteDisabled;
	CStr			m_SpriteOver;
	CStr			m_SpritePressed;
	EAlign			m_TextAlign;
	CColor			m_TextColor;
	CColor			m_TextColorDisabled;
	CColor			m_TextColorOver;
	CColor			m_TextColorPressed;
	EValign			m_TextValign;
	CStr			m_ToolTip;
	CStr			m_ToolTipStyle;
};

///////////////////////////////////////////////////////////////////////////////

class CButton : public CGUISettingsObject<SButtonSettings>, public CGUIButtonBehavior
{
	GUI_OBJECT(CButton)

public:
	CButton();
	virtual ~CButton();


	// Since we're doing multiple inheritance, this is to avoid error message
	virtual map_Settings GetSettingsInfo() const { return CGUISettingsObject<SButtonSettings>::m_SettingsInfo; }

	// Handle Messages
	virtual void HandleMessage(const EGUIMessage &Message);

	// Draw
	virtual void Draw();
};

#endif