/*
GUI Object - Input [box]
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object representing a text field you can edit.

--More info--

	Check GUI.h

*/

#ifndef CInput_H
#define CInput_H

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
 * @author Gustav Larsson
 *
 * Text field where you can input and edit the text.
 *
 * It doesn't use IGUITextOwner, because we don't need
 * any other features than word-wrapping, and we need to be
 * able to rapidly 
 * 
 * @see IGUIObject
 */
class CInput : public IGUIScrollBarOwner
{
	GUI_OBJECT(CInput)

public:
	CInput();
	virtual ~CInput();

	virtual void ResetStates() { IGUIScrollBarOwner::ResetStates(); }

protected:
	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Handle events manually to catch keyboard inputting.
	 */
	virtual int ManuallyHandleEvent(const SDL_Event* ev);

//    void InsertMessage(const wchar_t* szMessage, ...);
//	void InsertChar(const int szChar, const wchar_t cooked);

	/**
	 * Draws the Text
	 */
	virtual void Draw();

protected:
	// Cursor position
	int m_iBufferPos;
};

#endif
