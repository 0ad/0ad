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
#include "GUIbase.h"

#include "Overlay.h"
#include "lib/res/graphics/ogl_tex.h"

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


struct SGUIImageEffects
{
	SGUIImageEffects() : m_Greyscale(false) {}
	CColor m_AddColor;
	CColor m_MultiplyColor;
	bool m_Greyscale;
};


/**
 * @author Gustav Larsson
 *
 * A CGUISprite is actually a collage of several <b>real</b>
 * sprites, this struct represents is such real sprite.
 */
struct SGUIImage
{
	SGUIImage() : m_Effects(NULL), m_Border(false), m_DeltaZ(0.f) {}

	// Filename of the texture
	CStr			m_TextureName;

	// Image placement (relative to object)
	CClientArea		m_Size;

	// Texture placement (relative to image placement)
	CClientArea		m_TextureSize;

	// Because OpenGL wants textures in squares with a power of 2 (64x64, 256x256)
	//  it's sometimes tedious to adjust this. So this value simulates which area
	//  is the real texture
	CRect			m_TexturePlacementInFile;

	// For textures that contain a collection of icons (e.g. unit portraits), this
	//  will be set to the size of one icon. An object's cell-id will determine
	//  which part of the texture is used.
	//  Equal to CSize(0,0) for non-celled textures.
	CSize			m_CellSize;

	// Visual effects (e.g. colour modulation)
	SGUIImageEffects* m_Effects;

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
public:
	CGUISprite() {}
	virtual ~CGUISprite() {}

	/**
	 * Adds an image to the sprite collage.
	 *
	 * @param image Adds this image to the sprite collage.
	 */
	void AddImage(const SGUIImage &image) { m_Images.push_back(image); }

	/// List of images
	std::vector<SGUIImage> m_Images;
};

#include "GUIRenderer.h"

// An instance of a sprite, usually stored in IGUIObjects - basically a string
// giving the sprite's name, but with some extra data to cache rendering
// calculations between draw calls.
class CGUISpriteInstance
{
public:
	CGUISpriteInstance();
	CGUISpriteInstance(CStr SpriteName);
	CGUISpriteInstance(const CGUISpriteInstance &Sprite);
	CGUISpriteInstance &operator=(CStr SpriteName);
	void Draw(CRect Size, int CellID, std::map<CStr, CGUISprite> &Sprites);
	void Invalidate();
	bool IsEmpty() const;
	CStr GetName() { return m_SpriteName; }

private:
	CStr m_SpriteName;

	// Stored drawing calls, for more efficient rendering
	GUIRenderer::DrawCalls m_DrawCallCache;
	// Relevant details of previously rendered sprite; the cache is invalidated
	// whenever any of these values changes.
	CRect m_CachedSize;
	int m_CachedCellID;
};

#endif
