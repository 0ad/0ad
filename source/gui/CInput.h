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
 * able to rapidly change the string.
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

	// Calculate m_CharacterPosition
	//  the main task for this function is to perfom word-wrapping
	// You input from which character it has been changed, because
	//  if we add a character to the very last end, we don't want
	//  process everything all over again! Also notice you can
	//  specify a 'to' also, it will only be used though if a '\n'
	//  appears, because then the word-wrapping won't change after
	//  that.
	void UpdateText(int from=0, int to_before=-1, int to_after=-1);

protected:
	// Cursor position
	int m_iBufferPos;

	// the outer vector is lines, and the inner is X positions
	//  in a row. So that we can determine where characters are
	//  placed. It's important because we need to know where the
	//  pointer should be placed when the input control is pressed.
	struct SRow
	{
		int					m_ListStart;	// Where does the Row starts
		std::vector<float>	m_ListOfX;		// List of X values for each character.
	};

	// List of rows, list because I use a lot of iterators, and change
	//  its size continuously, it's easier and safer if I know the
	//  iterators never gets invalidated.
	std::list< SRow > m_CharacterPositions;
};

#endif
