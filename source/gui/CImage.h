/*
GUI Object - Image object

--Overview--

	GUI Object for just drawing a sprite.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_CIMAGE
#define INCLUDED_CIMAGE

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
 * Object just for drawing a sprite. Like CText, without the
 * possibility to draw text.
 *
 * Created, because I've seen the user being indecisive about
 * what control to use in these situations. I've seen button
 * without functionality used, and that is a lot of unnecessary
 * overhead. That's why I thought I'd go with an intuitive
 * control.
 * 
 * @see IGUIObject
 */
class CImage : public IGUIObject
{
	GUI_OBJECT(CImage)

public:
	CImage();
	virtual ~CImage();

protected:
	/**
	 * Draws the Image
	 */
	virtual void Draw();
};

#endif
