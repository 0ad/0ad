/*
GUI Object - Progress bar

--Overview--

	GUI Object to show progress or a value visually.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_CPROGRESSBAR
#define INCLUDED_CPROGRESSBAR

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
 * Object used to draw a value from 0 to 100 visually.
 * 
 * @see IGUIObject
 */
class CProgressBar : public IGUIObject
{
	GUI_OBJECT(CProgressBar)

public:
	CProgressBar();
	virtual ~CProgressBar();

protected:
	/**
	 * Draws the progress bar
	 */
	virtual void Draw();

	// If caption is set, make sure it's within the interval 0-100
	void HandleMessage(const SGUIMessage &Message);
};

#endif
