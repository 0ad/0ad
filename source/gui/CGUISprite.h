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

// Actual sprite
struct SGUIImage
{
	CStr			m_Texture;

	// Placement modifiers
	int				m_Pixel[4];
	float			m_Percent[4];

	// Texture modifiers
	int				m_TexturePixel[4];
	float			m_TexturePercent[4];

	//CColor		m_BackColor;
	//CColor		m_BorderColor;
	int				m_BorderSize;
};

// The GUI sprite, is actually several real sprites (images).
class CGUISprite
{
public:
	CGUISprite() {}
	virtual ~CGUISprite() {}

	// Execute a drawing request for this sprite
	void Draw(const float &z, const CRect &rect, const CRect &clipping);

	void AddImage(const SGUIImage &image) { m_Images.push_back(image); }

private:
	std::vector<SGUIImage>			m_Images;
};

#endif