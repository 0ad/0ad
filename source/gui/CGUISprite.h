/*
A GUI Sprite
by Gustav Larsson
gee@pyro.nu

--Overview--

	A GUI Sprite, which is actually a collage of several
	sprites.

--Usage--

	Used internally and declared in XML files, read documentations
	on how.

--More info--

	Check GUI.h

*/

#ifndef CGUISprite_H
#define CGUISprite_H

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
//  Error declarations
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * @author Gustav Larsson
 *
 * A CGUISprite is actually a collage of several <b>real</b>
 * sprites, this struct represents is such real sprite.
 */
struct SGUIImage
{
	CStr			m_Texture;

	// Image placement
	CClientArea		m_Size;

	// Texture placement
	CClientArea		m_TextureSize;

	// Color
	CColor			m_BackColor;
	CColor			m_BorderColor;

	// 0 or 1 pixel border is the only option
	bool			m_Border;
};

/**
 * @author Gustav Larsson
 *
 * The GUI sprite, is actually several real sprites (images)
 * like a collage. View the section <sprites> in the GUI
 * TDD for more information.
 *
 * Drawing routine is located in CGUI
 *
 * @see CGUI#DrawSprite
 */
class CGUISprite
{
	// For CGUI::DrawSprite()
	friend class CGUI;

public:
	CGUISprite() {}
	virtual ~CGUISprite() {}

	/**
	 * Execute a drawing request for this sprite
	 *
	 * @param z				Draw in what depth.
	 * @param rect			Outer rectangle to draw the collage.
	 * @param clipping		The clipping rectangle, things should only 
	 *						be drawn within these perimeters.
	 */
	//void Draw(const float &z, const CRect &rect, const CRect &clipping);

	/**
	 * Adds an image to the sprite collage.
	 *
	 * @param image Adds this image to the sprite collage.
	 */
	void AddImage(const SGUIImage &image) { m_Images.push_back(image); }

private:
	/// List of images
	std::vector<SGUIImage> m_Images;
};

#endif
