/*
GUI Object - Text [field]
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object representing a text field

--More info--

	Check GUI.h

*/

#ifndef CText_H
#define CText_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

// TODO Gee: Remove
class IGUIScrollBar;

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * Text Settings
 */
struct STextSettings
{
	CStr			m_Font;
	CStr			m_Sprite;
	EAlign			m_TextAlign;
	CColor			m_TextColor;
	EValign			m_TextValign;
	CStr			m_ToolTip;
	CStr			m_ToolTipStyle;
	bool			m_ScrollBar;
	CStr			m_ScrollBarStyle;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * @author Gustav Larsson
 *
 * Text field that just displays static text.
 * 
 * @see IGUIObject
 * @see IGUISettingsObject
 * @see STextSettings
 */
class CText : public IGUISettingsObject<STextSettings>, public IGUIScrollBarOwner
{
	GUI_OBJECT(CText)

public:
	CText();
	virtual ~CText();

	/**
	 * Since we're doing multiple inheritance, this is to avoid error message
	 *
	 * @return Settings infos
	 */
	virtual map_Settings GetSettingsInfo() const { return IGUISettingsObject<STextSettings>::m_SettingsInfo; }

	virtual void ResetStates() { IGUIScrollBarOwner::ResetStates(); }

	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Draws the Text
	 */
	virtual void Draw();

	// TODO Gee: Temp!
	//CGUIScrollBar m_ScrollBar;
};

#endif
