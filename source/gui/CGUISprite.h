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
#include "Overlay.h"
#include "lib/res/ogl_tex.h"

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
	SGUIImage() : m_Texture(0), m_Border(false), m_DeltaZ(0.f), m_TexturePlacementInFile() {}
	~SGUIImage() {
		if (m_Texture)
			tex_free(m_Texture);
	}

	// Copy constructor, which increments the refcount of the texture.
	// Slightly inefficient, but makes sure things get destructed
	// at the right times.
	SGUIImage(const SGUIImage& s)
	{
		#define C(x) m_##x = s.m_##x
		C(TextureName); C(Texture);
		C(Size); C(TextureSize);
		C(BackColor); C(BorderColor);
		C(Border); C(DeltaZ);
		C(TexturePlacementInFile);
		#undef C
		// 'Load' the texture (but don't do any work because it's cached)
		if (m_Texture)
			tex_load(m_TextureName);
	}

	CStr			m_TextureName;
	Handle			m_Texture;

	// Image placement
	CClientArea		m_Size;

	// Texture placement
	CClientArea		m_TextureSize;

	// Because OpenGL wants textures in squares with a power of 2 (64x64, 256x256)
	//  it's sometimes tediuos to adjust this. So this value simulates which area
	//  is the real texture
	CRect			m_TexturePlacementInFile;

	// Color
	CColor			m_BackColor;
	CColor			m_BorderColor;

	// 0 or 1 pixel border is the only option
	bool			m_Border;

	/**
	 * Z value modification of the image.
	 * Inputted in XML as x-level, although it just an easier and safer
	 * way of declaring delta-z.
	 */
	float			m_DeltaZ;
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
