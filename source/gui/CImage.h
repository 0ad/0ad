/*
GUI Object - Image object
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object for just drawing a sprite.

--More info--

	Check GUI.h

*/

#ifndef CImage_H
#define CImage_H

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
