/*
GUI Object - List [box]
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object for creating lists of information, wherein one
	 of the elements can be selected. A scroll-bar will aid
	 when there's too much information to be displayed at once.

--More info--

	Check GUI.h

*/

#ifndef CList_H
#define CList_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
//#include "GUI.h"

// TODO Gee: Remove
//class IGUIScrollBar;

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
 * Create a list of elements, where one can be selected
 * by the user. The control will use a pre-processed
 * text-object for each element, which will be managed
 * by the IGUITextOwner structure.
 *
 * A scroll-bar will appear when needed. This will be
 * achieve with the IGUIScrollBarOwner structure.
 *
 */

class CList : public IGUIScrollBarOwner, public IGUITextOwner
{
	GUI_OBJECT(CList)

public:
	CList();
	virtual ~CList();

	virtual void ResetStates() { IGUIScrollBarOwner::ResetStates(); }

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();

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

	/**
	 * Draws the List box
	 */
	virtual void Draw();

	/**
	 * Adds an item last to the list.
	 */
	virtual void AddItem(const CStr& str);

	/**
	 * Easy select elements functions
	 */
	virtual void SelectNextElement();
	virtual void SelectPrevElement();
	virtual void SelectFirstElement();
	virtual void SelectLastElement();

	/**
	 * Handle the <item> tag.
	 */
	virtual bool HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile);

	// Called every time the auto-scrolling should be checked.
	void UpdateAutoScroll();

	/**
	 * List of items (as text), the post-processed result is stored in
	 *  the IGUITextOwner structure of this class.
	 */
	std::vector<CGUIString> m_Items;

	/**
	 * List of each element's relative y position. Will be
	 * one larger than m_Items, because it will end with the
	 * bottom of the last element. First element will always
	 * be zero, but still stored for easy handling.
	 */
	std::vector<float> m_ItemsYPositions;
};

#endif
