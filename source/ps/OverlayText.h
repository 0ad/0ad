/*
COverlayText
by Rich Cross, rich@0ad.wildfiregames.com


--Overview--

	Class representing 2D screen text overlay
*/

#ifndef COVERLAYTEXT_H
#define COVERLAYTEXT_H


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "terrain/Texture.h"
#include "Overlay.h"		// just for CColor at the mo


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
class NPFont;

/**
 * @author Rich Cross
 *
 * OverlayText class definition.
 */
class COverlayText
{
public:
	/**
	 * Default constructor; creates an overlay text object that won't actually be renderable
	 */
	COverlayText();
	/**
	 * Constructor with setup for more common parameters
	 */
	COverlayText(float x,float y,int z,const char* fontname,const char* string,const CColor& color);
	/**
	 * Destructor 
	 */	
	~COverlayText();

	/**
	 * Get position
	 */	
	void GetPosition(float& x,float& y) {
		x=m_X;
		y=m_Y;
	}

	/**
	 * Get depth
	 */	
	int GetZ() const { return m_Z; }

	/**
	 * Get font (not const as Renderer need to modify texture to store handle)
	 */	
	NPFont* GetFont() { return m_Font; }

	/**
	 * Get string to render
	 */	
	const CStr& GetString() const { return m_String; }

	/**
	 * Get color
	 */	
	const CColor& GetColor() const { return m_Color; }

	/**
	 * Get size of this string when rendered; return false if font is invalid and string size cannot
	 * actually by determined, or true on success
	 */	
	bool GetOutputStringSize(int& sx,int& sy);

private:
	/// coordinates to start rendering string
	float m_X,m_Y;
	/// depth of overlay text, for correctly overlapping overlays; higher z implies in-front-of behaviour
	int m_Z;
	/// text color
	CColor m_Color;
	/// pointer to the font to use in rendering out text; not owned by the overlay, never attempt to delete
	NPFont* m_Font;
	/// actual text string
	CStr m_String;
};


#endif
