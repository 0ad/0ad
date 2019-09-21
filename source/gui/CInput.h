/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_CINPUT
#define INCLUDED_CINPUT

#include "gui/IGUIScrollBarOwner.h"
#include "lib/external_libraries/libsdl.h"

#include <vector>

/**
 * Text field where you can input and edit the text.
 *
 * It doesn't use IGUITextOwner, because we don't need
 * any other features than word-wrapping, and we need to be
 * able to rapidly change the string.
 */
class CInput : public IGUIScrollBarOwner
{
	GUI_OBJECT(CInput)

protected: // forwards
	struct SRow;

public:
	CInput(CGUI& pGUI);
	virtual ~CInput();

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates() { IGUIScrollBarOwner::ResetStates(); }

	// Check where the mouse is hovering, and get the appropriate text position.
	//  return is the text-position index.
	int GetMouseHoveringTextPosition() const;

	// Same as above, but only on one row in X, and a given value, not the mouse's.
	//  wanted is filled with x if the row didn't extend as far as the mouse pos.
	int GetXTextPosition(const std::list<SRow>::const_iterator& c, const float& x, float& wanted) const;

protected:
	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Handle events manually to catch keyboard inputting.
	 */
	virtual InReaction ManuallyHandleEvent(const SDL_Event_* ev);

	/**
	* Handle events manually to catch keys which change the text.
	*/
	virtual void ManuallyMutableHandleKeyDownEvent(const SDL_Keycode keyCode, CStrW& pCaption);

	/**
	* Handle events manually to catch keys which don't change the text.
	*/
	virtual void ManuallyImmutableHandleKeyDownEvent(const SDL_Keycode keyCode, CStrW& pCaption);

	/**
	 * Handle hotkey events (called by ManuallyHandleEvent)
	 */
	virtual InReaction ManuallyHandleHotkeyEvent(const SDL_Event_* ev);

	/**
	 * @see IGUIObject#UpdateCachedSize()
	 */
	virtual void UpdateCachedSize();

	/**
	 * Draws the Text
	 */
	virtual void Draw();

	/**
	 * Calculate m_CharacterPosition
	 * the main task for this function is to perfom word-wrapping
	 * You input from which character it has been changed, because
	 * if we add a character to the very last end, we don't want
	 * process everything all over again! Also notice you can
	 * specify a 'to' also, it will only be used though if a '\n'
	 * appears, because then the word-wrapping won't change after
	 * that.
	 */
	void UpdateText(int from = 0, int to_before = -1, int to_after = -1);

	/**
	 * Delete the current selection. Also places the pointer at the
	 * crack between the two segments kept.
	 */
	void DeleteCurSelection();

	/**
	 * Is text selected? It can be denote two ways, m_iBufferPos_Tail
	 * being -1 or the same as m_iBufferPos. This makes for clearer
	 * code.
	 */
	bool SelectingText() const;

	/// Get area of where text can be drawn.
	float GetTextAreaWidth();

	/// Called every time the auto-scrolling should be checked.
	void UpdateAutoScroll();

	/// Clear composed IME input when supported (SDL2 only).
	void ClearComposedText();

	/// Updates the buffer (cursor) position exposed to JS.
	void UpdateBufferPositionSetting();
protected:
	/// Cursor position
	int m_iBufferPos;
	/// Cursor position we started to select from. (-1 if not selecting)
	/// (NB: Can be larger than m_iBufferPos if selecting from back to front.)
	int m_iBufferPos_Tail;

	/// If we're composing text with an IME
	bool m_ComposingText;
	/// The length and position of the current IME composition
	int m_iComposedLength, m_iComposedPos;
	/// The position to insert committed text
	int m_iInsertPos;

	// the outer vector is lines, and the inner is X positions
	//  in a row. So that we can determine where characters are
	//  placed. It's important because we need to know where the
	//  pointer should be placed when the input control is pressed.
	struct SRow
	{
		int					m_ListStart;	/// Where does the Row starts
		std::vector<float>	m_ListOfX;		/// List of X values for each character.
	};

	/**
	 * List of rows to ease changing its size, so iterators stay valid.
	 * For one-liners only one row is used.
	 */
	std::list<SRow> m_CharacterPositions;

	// *** Things for a multi-lined input control *** //

	/**
	 * When you change row with up/down, and the row you jump to does
	 * not have anything at that X position, then it will keep the
	 * m_WantedX position in mind when switching to the next row.
	 * It will keep on being used until it reach a row which meets the
	 * requirements.
	 * 0.0f means not in use.
	 */
	float m_WantedX;

	/**
	 * If we are in the process of selecting a larger selection of text
	 * using the mouse click (hold) and drag, this is true.
	 */
	bool m_SelectingText;

	// *** Things for one-line input control *** //
	float m_HorizontalScroll;

	/// Used to store the previous time for flashing the cursor.
	double m_PrevTime;

	/// Cursor blink rate in seconds, if greater than 0.0.
	double m_CursorBlinkRate;

	/// If the cursor should be drawn or not.
	bool m_CursorVisState;

	/// If enabled, it is only allowed to select and copy text.
	bool m_Readonly;
};

#endif // INCLUDED_CINPUT
