/*
GUI Object Base - Text Owner
by Gustav Larsson
gee@pyro.nu

--Overview--

	Interface class that enhance the IGUIObject with
	 cached CGUIStrings. This class is not at all needed,
	 and many controls that will use CGUIStrings might
	 not use this, but does help for regular usage such
	 as a text-box, a button, a radio button etc.

--More info--

	Check GUI.h

*/

#ifndef IGUITextOwner_H
#define IGUITextOwner_H

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

/**
 * @author Gustav Larsson
 *
 * Framework for handling Output text.
 *
 * @see IGUIObject
 */
class IGUITextOwner : virtual public IGUIObject
{
public:
	IGUITextOwner();
	virtual ~IGUITextOwner();

	/**
	 * Adds a text object.
	 */
	void AddText(SGUIText * text);

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Draws the Text.
	 *
	 * @param index Index value of text. Mostly this will be 0
	 * @param pos Position
	 * @param z Z value
	 * @param clipping Clipping rectangle, don't even add a paramter
	 *		  to get no clipping.
	 */
	virtual void Draw(const int &index, const CColor &color, const CPos &pos, 
					  const float &z, const CRect &clipping = CRect());

protected:

	/**
	 * Setup texts. Functions that sets up all texts when changes have been made.
	 */
	virtual void SetupText()=0;

	/**
	 * Texts that are generated and ready to be rendred.
	 */
	std::vector<SGUIText*> m_GeneratedTexts;
};

#endif
