/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
GUI Object - Input [box]

--Overview--

	GUI Object representing a text field you can edit.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_CINPUT
#define INCLUDED_CINPUT

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

protected: // forwards
	struct SRow;

public:
	CInput();
	virtual ~CInput();

	virtual void ResetStates() { IGUIScrollBarOwner::ResetStates(); }

	// Check where the mouse is hovering, and get the appropriate text position.
	//  return is the text-position index.
	// const in philosophy, but I need to retrieve the caption in a non-const way.
	int GetMouseHoveringTextPosition();

	// Same as above, but only on one row in X, and a given value, not the mouse's
	//  wanted is filled with x if the row didn't extend as far as we 
	int GetXTextPosition(const std::list<SRow>::iterator &c, const float &x, float &wanted);

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
	virtual InReaction ManuallyHandleEvent(const SDL_Event_* ev);

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

	// Delete the current selection. Also places the pointer at the
	//  crack between the two segments kept.
	void DeleteCurSelection();

	// Is text selected? It can be denote two ways, m_iBufferPos_Tail
	//  being -1 or the same as m_iBufferPos. This makes for clearer
	//  code.
	bool SelectingText() const;

	// Get area of where text can be drawn.
	float GetTextAreaWidth();

	// Called every time the auto-scrolling should be checked.
	void UpdateAutoScroll();

protected:
	// Cursor position 
	//  (the second one is for selection of larger areas, -1 if not used)
	// A note on 'Tail', it was first called 'To', and the natural order
	//  of X and X_To was X then X_To. Now Tail is called so, because it
	//  can be placed both before and after, but the important things is
	//  that m_iBufferPos is ALWAYS where the edit pointer is. Yes, there
	//  is an edit pointer even though you select a larger area. For instance
	//  if you want to resize the selection with Shift+Left/Right, there
	//  are always two ways a selection can look. Check any OS GUI and you'll
	//  see.
	int m_iBufferPos,
		m_iBufferPos_Tail;

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
	// For one-liners, only one row is used.
	std::list< SRow > m_CharacterPositions;

	// *** Things for a multi-lined input control *** //

	// This is when you change row with up/down, and the row you jump
	//  to doesn't have anything at that X position, then it will
	//  keep the WantedX position in mind when switching to the next
	//  row. It will keep on being used until it reach a row which meets
	//  the requirements.
	//  0.0f means not in use.
	float m_WantedX;

	// If we are in the process of selecting a larger selection of text
	//  using the mouse click (hold) and drag, this is true.
	bool m_SelectingText;

	// *** Things for one-line input control *** //
	float m_HorizontalScroll;
};

#endif
